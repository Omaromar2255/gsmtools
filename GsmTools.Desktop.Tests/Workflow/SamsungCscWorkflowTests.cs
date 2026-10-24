using GsmTools.Desktop.Core;
using GsmTools.Desktop.Tests.Mocks;
using GsmTools.Desktop.Workflows;
using GsmTools.Desktop.Workflows.Models;
using Xunit;

namespace GsmTools.Desktop.Tests.Workflow;

public class SamsungCscWorkflowTests : IAsyncLifetime
{
    private readonly MockAdbServer _server = new();
    private readonly AdbRawSocketClient _adbClient = new();
    private PersistentAtBridgeSession? _atSession;
    private SamsungCscWorkflow? _workflow;

    public async Task InitializeAsync()
    {
        _server.Start();
        await _adbClient.ConnectAsync(_server.Host, _server.Port);
        _atSession = new PersistentAtBridgeSession(_adbClient);
        _workflow = new SamsungCscWorkflow(_adbClient, _atSession);
    }

    public async Task DisposeAsync()
    {
        if (_atSession != null) await _atSession.DisposeAsync();
        await _adbClient.DisposeAsync();
        await _server.DisposeAsync();
    }

    [Fact]
    public async Task ReadDeviceInfoAsync_ExecutesAll8PreChecksAndReturnsDeviceInfo()
    {
        // Features F12-F19: Device Pre-checks 1 to 8
        DeviceInfo? eventInfo = null;
        _workflow!.OnDeviceInfoUpdated += info => eventInfo = info;

        var info = await _workflow.ReadDeviceInfoAsync();

        Assert.NotNull(info);
        Assert.Equal("SM-S918U", info.Model);
        Assert.Equal("RFCX90GR21X", info.Serial);
        Assert.Equal("355752920542633", info.Imei);
        Assert.Equal("ATT", info.CurrentCsc);
        Assert.Equal("SM-S918UZKFXAA", info.ProductCode);
        Assert.Equal("15", info.AndroidVersion);
        Assert.Equal("2025-07-01", info.SecurityPatch);
        Assert.True(info.IsSelinuxEnforcing);
        Assert.False(info.HasRsuApp);
        Assert.False(info.HasTsdaemon);
        Assert.True(info.HasKeyTool);
        Assert.True(info.Port2600Available);
        Assert.Equal("5.59.6-main", info.RetailAgentVersion);

        Assert.NotNull(eventInfo);
        Assert.Equal(info.Model, eventInfo.Model);
    }

    [Fact]
    public async Task ExecuteCscChangeAsync_InvalidCscFormat_ThrowsArgumentException()
    {
        // Feature F20: CscValidation
        // Too short, too long, lowercase, non-alpha
        var res1 = await _workflow!.ExecuteCscChangeAsync("XA");
        Assert.False(res1.Success);
        Assert.Contains("Invalid CSC", res1.Message);

        var res2 = await _workflow.ExecuteCscChangeAsync("XAAA");
        Assert.False(res2.Success);

        var res3 = await _workflow.ExecuteCscChangeAsync("123");
        Assert.False(res3.Success);
    }

    [Fact]
    public async Task ExecuteCscChangeAsync_ValidCsc_Executes5FramesAndCleanupAndReset()
    {
        // Features F20-F26
        var steps = new List<WorkflowStepProgress>();
        _workflow!.OnStepChanged += s => steps.Add(s);

        var result = await _workflow.ExecuteCscChangeAsync("XAA", useEncryptedAtSession: false);

        Assert.True(result.Success);
        Assert.Equal("ATT", result.OldCsc);
        Assert.Equal("XAA", result.NewCsc);
        Assert.NotEmpty(result.ExecutionLog);

        // Verify Radio logcat flush executed before injection (F23)
        Assert.Contains(_server.ReceivedShellCommands, c => c == "logcat -b radio -c");

        // Verify 5-command CSC injection (F24)
        // For XAA -> 584141
        Assert.Contains("0601000E00005841410000584141", _server.AtBridge.ReceivedIpcCommands);
        Assert.Contains("060100090000584141", _server.AtBridge.ReceivedIpcCommands);
        Assert.Contains("0601001200005841410000584141584141", _server.AtBridge.ReceivedIpcCommands);
        Assert.Contains("060100120000", _server.AtBridge.ReceivedIpcCommands);

        // Verify post-cleanup commands executed (F25)
        Assert.Contains(_server.ReceivedShellCommands, c => c.Contains("AutoPreconfig"));

        // Verify direct USB reset executed (F26)
        Assert.Contains(_server.ReceivedRequests, r => r.Contains("usb:"));

        // Verify progress reporting (8 steps total)
        Assert.True(steps.Count >= 8);
        Assert.Equal(8, steps.Last().TotalSteps);
        Assert.Equal(100.0, steps.Last().ProgressPercentage);
    }

    [Fact]
    public async Task ExecuteCscChangeAsync_WithEncryptedSession_CompletesSuccessfully()
    {
        // Encrypted AT session mode
        var result = await _workflow!.ExecuteCscChangeAsync("USC", useEncryptedAtSession: true);

        Assert.True(result.Success);
        Assert.Equal("USC", result.NewCsc);

        // Bridge received session seed > 0
        Assert.True(_server.AtBridge.SessionSeed > 0);

        // 5 commands decrypted and verified by mock bridge
        // USC -> 555343
        Assert.Contains("0601000E00005553430000555343", _server.AtBridge.ReceivedIpcCommands);
        Assert.Contains("060100090000555343", _server.AtBridge.ReceivedIpcCommands);
    }

    [Fact]
    public async Task ExecuteMethod2Async_InvalidCsc_FailsValidation()
    {
        var res = await _workflow!.ExecuteMethod2Async("INVALID");
        Assert.False(res.Success);
        Assert.Contains("Invalid CSC", res.Message);
    }

    [Fact]
    public async Task ExecuteMethod2Async_ValidCsc_ExecutesFull14StepsMatching33Txt()
    {
        var steps = new List<WorkflowStepProgress>();
        _workflow!.OnStepChanged += s => steps.Add(s);

        // Test pushing SRV4 by simulating KEYTool.SRV4 not initially installed
        _server.ShellResponses["pm path KEYTool.SRV4"] = "";
        // Simulate port 2600 not initially open, requiring exploit to run
        _server.Port2600FailAttempts = 2;

        string artifactsDir = @"C:\Users\pc5\Documents\GitHub\gsmtools\2";
        var result = await _workflow.ExecuteMethod2Async("TMB", artifactsDir, useEncryptedAtSession: false);

        Assert.True(result.Success, $"Workflow failed with message: {result.Message}");
        Assert.Equal("ATT", result.OldCsc);
        Assert.Equal("TMB", result.NewCsc);
        Assert.NotEmpty(result.ExecutionLog);

        // 1. Verify SRV4 push and install
        Assert.True(_server.PushedFiles.ContainsKey("/data/local/tmp/SRV4"));
        Assert.Contains(_server.ReceivedShellCommands, c => c == "pm install -r /data/local/tmp/SRV4");

        // 2. Verify Port 2600 check & stayon
        Assert.Contains(_server.ReceivedShellCommands, c => c == "echo 'id' |toybox nc localhost 2600");
        Assert.Contains(_server.ReceivedShellCommands, c => c == "svc power stayon true");

        // 3. Verify diagnostic dumpsys queries
        Assert.Contains(_server.ReceivedShellCommands, c => c.Contains("repaircal"));
        Assert.Contains(_server.ReceivedShellCommands, c => c.Contains("mobiledoctor"));
        Assert.Contains(_server.ReceivedShellCommands, c => c.Contains("retailagent"));

        // 4. Verify retail exploit files pushed
        Assert.True(_server.PushedFiles.ContainsKey("/data/local/tmp/retail/ADP.xml"));
        Assert.True(_server.PushedFiles.ContainsKey("/data/local/tmp/retail/ASKSW.xml"));
        Assert.True(_server.PushedFiles.ContainsKey("/data/local/tmp/retail/classes.dex"));
        Assert.True(_server.PushedFiles.ContainsKey("/data/local/tmp/retail/trustedTime"));

        // 5. Verify WallpaperExploit transaction command
        Assert.Contains(_server.ReceivedShellCommands, c => c.Contains("WallpaperExploit") && c.Contains("app_process"));
        Assert.Contains(_server.ReceivedShellCommands, c => c == "rm -rf /data/local/tmp/retail");

        // 6. Verify R2.zip push, extraction, and install
        Assert.True(_server.PushedFiles.ContainsKey("/data/local/tmp/R2.zip"));
        Assert.Contains(_server.ReceivedShellCommands, c => c.Contains("unzip /data/local/tmp/R2.zip"));
        Assert.Contains(_server.ReceivedShellCommands, c => c == "pm install -r /data/local/tmp/R2");

        // 7. Verify Port 2600 UID 1000 polling
        Assert.Contains(_server.ReceivedShellCommands, c => c == "echo 'id' |toybox nc localhost 2600");

        // 8. Verify ssud stopped via port 2600
        Assert.Contains(_server.ReceivedShellCommands, c => c == "echo 'setprop ctl.stop ssud'|toybox nc localhost 2600");

        // 9. Verify AT pushed and chmod 777
        Assert.True(_server.PushedFiles.ContainsKey("/data/local/tmp/AT"));
        Assert.Contains(_server.ReceivedShellCommands, c => c == "chmod 777 /data/local/tmp/AT");

        // 10. Verify radio logcat flush
        Assert.Contains(_server.ReceivedShellCommands, c => c == "logcat -b radio -c");

        // 11. Verify 5 CSC injection packets for TMB (544D42)
        Assert.Contains("0601000E0000544D420000544D42", _server.AtBridge.ReceivedIpcCommands);
        Assert.Contains("060100090000544D42", _server.AtBridge.ReceivedIpcCommands);
        Assert.Contains("060100120000544D420000544D42544D42", _server.AtBridge.ReceivedIpcCommands);
        Assert.Contains("060100120000", _server.AtBridge.ReceivedIpcCommands);
        Assert.Contains("060100090000544D42", _server.AtBridge.ReceivedIpcCommands);

        // 12. Verify post-cleanup & USB reset
        Assert.Contains(_server.ReceivedShellCommands, c => c.Contains("AutoPreconfig"));
        Assert.Contains(_server.ReceivedRequests, r => r.Contains("usb:"));

        // 13. Verify progress steps
        Assert.True(steps.Count >= 14);
        Assert.Equal(14, steps.Last().TotalSteps);
        Assert.Equal(100.0, steps.Last().ProgressPercentage);
    }

    [Fact]
    public async Task ExecuteMethod2Async_Port2600AlreadyActive_BypassesExploitAndInjectsSuccessfully()
    {
        // Port 2600 already returns uid=1000 (0 fail attempts)
        _server.Port2600FailAttempts = 0;

        string artifactsDir = @"C:\Users\pc5\Documents\GitHub\gsmtools\2";
        var result = await _workflow!.ExecuteMethod2Async("VZW", artifactsDir, useEncryptedAtSession: false);

        Assert.True(result.Success, $"Workflow failed with message: {result.Message}");
        Assert.Equal("VZW", result.NewCsc);

        // Verify exploit staging was skipped to preserve running privilege session
        Assert.False(_server.PushedFiles.ContainsKey("/data/local/tmp/retail/ADP.xml"));
        Assert.DoesNotContain(_server.ReceivedShellCommands, c => c.Contains("WallpaperExploit"));

        // Verify 5 CSC packets for VZW (565A57)
        Assert.Contains("0601000E0000565A570000565A57", _server.AtBridge.ReceivedIpcCommands);
        Assert.Contains("060100090000565A57", _server.AtBridge.ReceivedIpcCommands);
    }
}
