using GsmTools.Desktop.Core;
using GsmTools.Desktop.Tests.Mocks;
using GsmTools.Desktop.Workflows;
using Xunit;

namespace GsmTools.Desktop.Tests.Workflow;

public class RadioControlWorkflowTests : IAsyncLifetime
{
    private readonly MockAdbServer _server = new();
    private readonly AdbRawSocketClient _adbClient = new();
    private PersistentAtBridgeSession? _atSession;
    private RadioControlWorkflow? _workflow;

    public async Task InitializeAsync()
    {
        _server.Start();
        await _adbClient.ConnectAsync(_server.Host, _server.Port);
        _atSession = new PersistentAtBridgeSession(_adbClient);
        _workflow = new RadioControlWorkflow(_adbClient, _atSession);
    }

    public async Task DisposeAsync()
    {
        if (_atSession != null) await _atSession.DisposeAsync();
        await _adbClient.DisposeAsync();
        await _server.DisposeAsync();
    }

    [Theory]
    [InlineData("REBOOT", "CFUN=1,1")]
    [InlineData("OFF", "CFUN=0")]
    [InlineData("ON", "CFUN=1")]
    [InlineData("FACTORST", "FACTORST=0,0")]
    [InlineData("SWRESET", "SWRESET")]
    public async Task ExecuteRadioActionAsync_ValidAction_Succeeds(string action, string expectedSnippet)
    {
        var result = await _workflow!.ExecuteRadioActionAsync(action);
        Assert.True(result.Success);
        Assert.Equal(action, result.Action);
    }

    [Fact]
    public async Task ExecuteRadioActionAsync_UnknownAction_ThrowsArgumentException()
    {
        await Assert.ThrowsAsync<ArgumentException>(() => _workflow!.ExecuteRadioActionAsync("INVALID_ACTION"));
    }

    [Fact]
    public async Task QuerySecuritySeedAsync_ReturnsResult()
    {
        var result = await _workflow!.QuerySecuritySeedAsync();
        Assert.True(result.Success);
        Assert.NotNull(result.ChallengeSeed);
    }
}
