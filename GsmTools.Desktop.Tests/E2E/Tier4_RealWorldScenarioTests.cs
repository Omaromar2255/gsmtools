using GsmTools.Desktop.Core;
using GsmTools.Desktop.Tests.Mocks;
using GsmTools.Desktop.Workflows;
using GsmTools.Desktop.Workflows.Models;
using Xunit;

namespace GsmTools.Desktop.Tests.E2E;

/// <summary>
/// Tier 4: Real-World Application Workload Scenarios.
/// Authoritative replays of csc 15.txt Run 1 (XAA) and Run 2 (USC with port 2600 refusal/recovery),
/// full Galaxy S23 Ultra device telemetry audit, encrypted session playback, and post-reset re-enumeration.
/// </summary>
public class Tier4_RealWorldScenarioTests
{
    [Fact]
    public async Task Scenario1_FullDeviceTelemetryAudit_GalaxyS23Ultra()
    {
        // Replays initial audit in csc 15.txt lines 7-57
        await using var server = new MockAdbServer();
        server.Start();

        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);
        var workflow = new SamsungCscWorkflow(client, session);

        var devInfo = await workflow.ReadDeviceInfoAsync();

        Assert.Equal("SM-S918U", devInfo.Model);
        Assert.Equal("RFCX90GR21X", devInfo.Serial);
        Assert.Equal("355752920542633", devInfo.Imei);
        Assert.Equal("ATT", devInfo.CurrentCsc);
        Assert.Equal("SM-S918UZKFXAA", devInfo.ProductCode);
        Assert.Equal("15", devInfo.AndroidVersion);
        Assert.Equal("2025-07-01", devInfo.SecurityPatch);
        Assert.True(devInfo.IsSelinuxEnforcing);
        Assert.False(devInfo.HasRsuApp);
        Assert.False(devInfo.HasTsdaemon);
        Assert.True(devInfo.HasKeyTool);
        Assert.True(devInfo.Port2600Available);
        Assert.Equal("5.59.6-main", devInfo.RetailAgentVersion);
    }

    [Fact]
    public async Task Scenario2_EndToEndCscChange_Run1_Playback_XAA()
    {
        // Full replay of Run 1 in csc 15.txt (17:40:08 - 17:40:25)
        await using var server = new MockAdbServer();
        server.Start();

        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);
        var workflow = new SamsungCscWorkflow(client, session);

        var result = await workflow.ExecuteCscChangeAsync("XAA", useEncryptedAtSession: false);

        Assert.True(result.Success);
        Assert.Equal("ATT", result.OldCsc);
        Assert.Equal("XAA", result.NewCsc);

        // Verify exact 5-frame sequence for XAA (584141)
        var cmds = server.AtBridge.ReceivedIpcCommands;
        Assert.True(cmds.Count >= 5);
        Assert.True(string.Equals("0601000E00005841410000584141", cmds[0], StringComparison.OrdinalIgnoreCase));
        Assert.True(string.Equals("060100090000584141", cmds[1], StringComparison.OrdinalIgnoreCase));
        Assert.True(string.Equals("0601001200005841410000584141584141", cmds[2], StringComparison.OrdinalIgnoreCase));
        Assert.True(string.Equals("060100120000", cmds[3], StringComparison.OrdinalIgnoreCase));
        Assert.True(string.Equals("060100090000584141", cmds[4], StringComparison.OrdinalIgnoreCase));

        // Verify post-cleanup and USB mode reset
        Assert.Contains(server.ReceivedShellCommands, c => c.Contains("com.sec.android.AutoPreconfig"));
        Assert.Contains(server.ReceivedRequests, r => r.Contains("usb:"));
    }

    [Fact]
    public async Task Scenario3_EndToEndCscChange_Run2_Playback_USC_Port2600RefusalRecovery()
    {
        // Full replay of Run 2 in csc 15.txt (17:41:46 - 17:42:26)
        // Includes: port 2600 refused, svc stayon, diagnostic checks, polling recovery, ssud stop, injection
        await using var server = new MockAdbServer();
        server.Port2600FailAttempts = 2; // Refused twice, recovers on attempt 3
        server.Start();

        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);
        var workflow = new SamsungCscWorkflow(client, session);

        var result = await workflow.ExecuteCscChangeAsync("USC", useEncryptedAtSession: false);

        Assert.True(result.Success);
        Assert.Equal("USC", result.NewCsc);

        // Verify stayon and ssud stoppage occurred
        Assert.Contains("svc power stayon true", server.ReceivedShellCommands);
        Assert.Contains(server.ReceivedShellCommands, c => c.Contains("ctl.stop ssud"));

        // Verify exact 5-frame sequence for USC (555343)
        var cmds = server.AtBridge.ReceivedIpcCommands;
        Assert.True(cmds.Count >= 5);
        Assert.True(string.Equals("0601000E00005553430000555343", cmds[0], StringComparison.OrdinalIgnoreCase));
        Assert.True(string.Equals("060100090000555343", cmds[1], StringComparison.OrdinalIgnoreCase));
        Assert.True(string.Equals("0601001200005553430000555343555343", cmds[2], StringComparison.OrdinalIgnoreCase));
        Assert.True(string.Equals("060100120000", cmds[3], StringComparison.OrdinalIgnoreCase));
        Assert.True(string.Equals("060100090000555343", cmds[4], StringComparison.OrdinalIgnoreCase));
    }

    [Fact]
    public async Task Scenario4_EncryptedAtBridgeSession_WithDynamicSeed()
    {
        // Verifies encrypted session with dynamic non-zero seed
        await using var server = new MockAdbServer();
        server.Start();

        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);
        var workflow = new SamsungCscWorkflow(client, session);

        var result = await workflow.ExecuteCscChangeAsync("TMB", useEncryptedAtSession: true);

        Assert.True(result.Success);
        Assert.True(server.AtBridge.SessionSeed > 0);

        // Verify mock bridge successfully decrypted all 5 frames
        // TMB -> 544D42
        var cmds = server.AtBridge.ReceivedIpcCommands.TakeLast(5).ToList();
        Assert.Equal(5, cmds.Count);
        Assert.True(string.Equals("0601000E0000544D420000544D42", cmds[0], StringComparison.OrdinalIgnoreCase));
        Assert.True(string.Equals("060100090000544D42", cmds[1], StringComparison.OrdinalIgnoreCase));
    }

    [Fact]
    public async Task Scenario5_UsbDisconnection_AndPostRebootVerificationAudit()
    {
        // Replays the interlude and post-reboot verification audit in csc 15.txt lines 92-119
        await using var server = new MockAdbServer();
        server.Start();

        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);
        var workflow = new SamsungCscWorkflow(client, session);

        // 1. Perform CSC change to XAA
        var cscResult = await workflow.ExecuteCscChangeAsync("XAA");
        Assert.True(cscResult.Success);

        // 2. Simulate device rebooting into new CSC
        server.ShellResponses["getprop"] =
            "[ro.product.model]: [SM-S918U]\r\n" +
            "[ro.boot.serialno]: [RFCX90GR21X]\r\n" +
            "[ro.csc.sales_code]: [XAA]\r\n" +
            "[ril.product_code]: [SM-S918UZKFXAA]\r\n" +
            "[ro.build.version.release]: [15]\r\n" +
            "[ro.build.version.security_patch]: [2025-07-01]\r\n" +
            "[sys.usb.config]: [mtp,conn_gadget,adb]\r\n";

        // 3. Post-reboot audit
        var postAudit = await workflow.ReadDeviceInfoAsync();
        Assert.Equal("XAA", postAudit.CurrentCsc);
        Assert.Equal("SM-S918UZKFXAA", postAudit.ProductCode);
    }
}
