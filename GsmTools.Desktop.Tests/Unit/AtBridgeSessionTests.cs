using GsmTools.Desktop.Core;
using GsmTools.Desktop.Tests.Mocks;
using Xunit;

namespace GsmTools.Desktop.Tests.Unit;

public class AtBridgeSessionTests : IAsyncLifetime
{
    private readonly MockAdbServer _server = new();
    private readonly AdbRawSocketClient _adbClient = new();
    private PersistentAtBridgeSession? _session;

    public async Task InitializeAsync()
    {
        _server.Start();
        await _adbClient.ConnectAsync(_server.Host, _server.Port);
        _session = new PersistentAtBridgeSession(_adbClient);
    }

    public async Task DisposeAsync()
    {
        if (_session != null)
        {
            await _session.DisposeAsync();
        }
        await _adbClient.DisposeAsync();
        await _server.DisposeAsync();
    }

    [Fact]
    public async Task StartSessionAsync_ParsesCompactBannerAndSetsBannerProperties()
    {
        // F8: AtBannerParsing
        _server.AtBridge.Model = "SM-S918U";
        _server.AtBridge.Csc = "ATT";
        _server.AtBridge.AndroidVersion = "15";
        _server.AtBridge.SecurityPatch = "2025-07-01";
        _server.AtBridge.Uid = 1000;
        _server.AtBridge.ActiveChannel = "/dev/socket/rild";

        var banner = await _session!.StartSessionAsync(sessionSeed: 0);

        Assert.NotNull(banner);
        Assert.Equal("SM-S918U", banner.Model);
        Assert.Equal("ATT", banner.Csc);
        Assert.Equal("15", banner.AndroidVersion);
        Assert.Equal("2025-07-01", banner.SecurityPatch);
        Assert.Equal(1000, banner.Uid);
        Assert.Equal("/dev/socket/rild", banner.ActiveChannel);
        Assert.True(_session.IsActive);
    }

    [Fact]
    public async Task StartSessionAsync_WithSeed_PerformsAuthHandshakeAndActivatesEncryption()
    {
        // F9: AtAuthHandshake
        uint seed = 0x12345678;
        var banner = await _session!.StartSessionAsync(sessionSeed: seed);

        Assert.Equal(seed, _session.SessionSeed);
        Assert.Contains(_server.AtBridge.ReceivedRawLines, line => line == $"AUTH:{seed:X8}");
    }

    [Fact]
    public async Task SendIpcHexCommandAsync_RawHexMode_SendsAndReceivesPlaintextDelimiters()
    {
        // F10: AtFramingDelimiters
        await _session!.StartSessionAsync(sessionSeed: 0);

        string cmd = "0601000e00005841410000584141";
        string response = await _session.SendIpcHexCommandAsync(cmd);

        Assert.Equal(string.Empty, response); // .OK..# returns empty hex response
        Assert.Contains(_server.AtBridge.ReceivedIpcCommands, c => string.Equals(c, cmd, StringComparison.OrdinalIgnoreCase));
    }

    [Fact]
    public async Task SendIpcHexCommandAsync_EncryptedMode_TransmitsCiphertextAndDecryptsResponse()
    {
        // F7, F9, F10: Encrypted session roundtrip
        uint seed = 0x9ABCDEF0;
        await _session!.StartSessionAsync(sessionSeed: seed);

        _server.AtBridge.CustomIpcHandler = (decryptedHex) =>
        {
            // Return an IPC hex payload "AABBCCDDEE"
            return (true, "AABBCCDDEE");
        };

        string cmd = "060100090000584141";
        string response = await _session.SendIpcHexCommandAsync(cmd);

        // Client decrypts the response returned by server
        Assert.Equal("AABBCCDDEE", response);
        // Server decrypted the received command correctly
        Assert.Contains(cmd, _server.AtBridge.ReceivedIpcCommands);
    }

    [Fact]
    public async Task StartSessionAsync_Uid2000AndChannelNone_TriggersSmartPort2600Relay()
    {
        // F11: AtUidFallback
        _server.AtBridge.Uid = 2000;
        _server.AtBridge.ActiveChannel = "NONE";
        _server.ShellResponses["echo 'id' |toybox nc localhost 2600"] = "uid=1000(system) gid=1000(system) context=u:r:system_app:s0\n";

        var banner = await _session!.StartSessionAsync(sessionSeed: 0);

        Assert.NotNull(banner);
        // Relay via toybox nc localhost 2600 should have been executed
        Assert.Contains(_server.ReceivedRequests, r => r.Contains("toybox nc localhost 2600"));
    }

    [Fact]
    public async Task TerminateSessionAsync_SendsExitCommandAndClosesSession()
    {
        await _session!.StartSessionAsync(sessionSeed: 0);
        Assert.True(_session.IsActive);

        await _session.TerminateSessionAsync();

        Assert.False(_session.IsActive);
        Assert.Contains(_server.AtBridge.ReceivedRawLines, line => line == "EXIT");
    }
}
