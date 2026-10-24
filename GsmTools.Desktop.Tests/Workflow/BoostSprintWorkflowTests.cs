using GsmTools.Desktop.Core;
using GsmTools.Desktop.Tests.Mocks;
using GsmTools.Desktop.Workflows;
using Xunit;

namespace GsmTools.Desktop.Tests.Workflow;

public class BoostSprintWorkflowTests : IAsyncLifetime
{
    private readonly MockAdbServer _server = new();
    private readonly AdbRawSocketClient _adbClient = new();
    private PersistentAtBridgeSession? _atSession;
    private BoostSprintWorkflow? _workflow;

    public async Task InitializeAsync()
    {
        _server.Start();
        await _adbClient.ConnectAsync(_server.Host, _server.Port);
        _atSession = new PersistentAtBridgeSession(_adbClient);
        _workflow = new BoostSprintWorkflow(_adbClient, _atSession);
    }

    public async Task DisposeAsync()
    {
        if (_atSession != null) await _atSession.DisposeAsync();
        await _adbClient.DisposeAsync();
        await _server.DisposeAsync();
    }

    [Fact]
    public async Task ExecuteBoostUnlockAsync_ExecutesAll5StepsSuccessfully()
    {
        var logs = new List<string>();
        _workflow!.OnLog += msg => logs.Add(msg);

        var result = await _workflow.ExecuteBoostUnlockAsync();

        Assert.True(result.Success);
        Assert.Equal(5, result.StepsCompleted);
        Assert.Contains(logs, l => l.Contains("AT+UNLOCKSPR=OLD"));
        Assert.Contains(logs, l => l.Contains("AT+SIMLOCKU=0,0,0"));
        Assert.Contains(logs, l => l.Contains("com.sec.omadmspr"));
        Assert.Contains(logs, l => l.Contains("AT+CFUN=1,1"));
    }

    [Fact]
    public async Task ExecuteCarrierRelockAsync_GeneratesBcdAndDispatchesRelock()
    {
        var logs = new List<string>();
        _workflow!.OnLog += msg => logs.Add(msg);

        // Relock to Sabafon Yemen (421-01 -> 421F0100)
        var result = await _workflow.ExecuteCarrierRelockAsync("421", "01", "Sabafon Yemen");

        Assert.True(result.Success);
        Assert.Equal("421", result.TargetMcc);
        Assert.Equal("01", result.TargetMnc);
        Assert.Equal("421F0100", result.BcdPayload);
        Assert.Contains(logs, l => l.Contains("421F0100"));
    }
}
