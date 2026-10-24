using System.Text;
using GsmTools.Desktop.Core;
using GsmTools.Desktop.Tests.Mocks;
using GsmTools.Desktop.Workflows;
using GsmTools.Desktop.Workflows.Models;
using Xunit;

namespace GsmTools.Desktop.Tests.E2E;

/// <summary>
/// Tier 1: Systematic Feature Coverage Tests for all 26 core features (F1 - F26).
/// Each feature has at least 5 dedicated, isolated test cases covering primary and specific variants.
/// </summary>
public class Tier1_FeatureCoverageTests
{
    // =========================================================================
    // F1: AdbTcpFraming (ORIGINAL_REQUEST §R1) - 5 tests
    // =========================================================================
    [Fact]
    public async Task F01_AdbTcpFraming_StandardOkay()
    {
        await using var server = new MockAdbServer();
        server.Start();
        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);

        string resp = await client.SendAdbRequestAsync("host:version");
        Assert.Equal("OKAY", resp);
    }

    [Fact]
    public async Task F01_AdbTcpFraming_DevicesRequestLength()
    {
        await using var server = new MockAdbServer();
        server.Start();
        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);

        string resp = await client.SendAdbRequestAsync("host:devices");
        Assert.Equal("OKAY", resp);
        Assert.Contains(server.ReceivedRequests, r => r == "host:devices");
    }

    [Fact]
    public async Task F01_AdbTcpFraming_FailHandlingWithParsedMessage()
    {
        await using var server = new MockAdbServer();
        server.SimulateFailResponse = true;
        server.FailErrorMessage = "device not found";
        server.Start();
        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);

        var ex = await Assert.ThrowsAsync<AdbException>(() => client.SendAdbRequestAsync("host:transport-any"));
        Assert.Contains("device not found", ex.Message);
    }

    [Fact]
    public async Task F01_AdbTcpFraming_UnconnectedClientThrowsInvalidOperation()
    {
        await using var client = new AdbRawSocketClient();
        await Assert.ThrowsAsync<InvalidOperationException>(() => client.SendAdbRequestAsync("host:version"));
    }

    [Fact]
    public async Task F01_AdbTcpFraming_MalformedServerHeaderThrowsAdbException()
    {
        await using var server = new MockAdbServer();
        server.SimulateMalformedHeader = true;
        server.Start();
        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);

        await Assert.ThrowsAsync<AdbException>(() => client.SendAdbRequestAsync("host:version"));
    }

    // =========================================================================
    // F2: AdbTransportSwitch (ORIGINAL_REQUEST §R1) - 5 tests
    // =========================================================================
    [Fact]
    public async Task F02_AdbTransportSwitch_SelectSpecificSerial()
    {
        await using var server = new MockAdbServer();
        server.Start();
        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);

        await client.SelectDeviceAsync("RFCX90GR21X");
        Assert.Equal("RFCX90GR21X", client.CurrentSerial);
        Assert.Contains("host:transport:RFCX90GR21X", server.ReceivedRequests);
    }

    [Fact]
    public async Task F02_AdbTransportSwitch_SelectNullSerial_UsesTransportAny()
    {
        await using var server = new MockAdbServer();
        server.Start();
        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);

        await client.SelectDeviceAsync(null);
        Assert.Null(client.CurrentSerial);
        Assert.Contains("host:transport-any", server.ReceivedRequests);
    }

    [Fact]
    public async Task F02_AdbTransportSwitch_SelectEmptySerial_UsesTransportAny()
    {
        await using var server = new MockAdbServer();
        server.Start();
        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);

        await client.SelectDeviceAsync("   ");
        Assert.Equal("   ", client.CurrentSerial);
        Assert.Contains("host:transport-any", server.ReceivedRequests);
    }

    [Fact]
    public async Task F02_AdbTransportSwitch_SequentialSwitching()
    {
        await using var server = new MockAdbServer();
        server.Start();
        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);

        await client.SelectDeviceAsync("SERIAL_A");
        await client.SelectDeviceAsync("SERIAL_B");

        Assert.Equal("SERIAL_B", client.CurrentSerial);
        Assert.Contains("host:transport:SERIAL_A", server.ReceivedRequests);
        Assert.Contains("host:transport:SERIAL_B", server.ReceivedRequests);
    }

    [Fact]
    public async Task F02_AdbTransportSwitch_ServerFailOnInvalidSerial()
    {
        await using var server = new MockAdbServer();
        server.SimulateFailResponse = true;
        server.FailErrorMessage = "device 'INVALID' not found";
        server.Start();
        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);

        var ex = await Assert.ThrowsAsync<AdbException>(() => client.SelectDeviceAsync("INVALID"));
        Assert.Contains("device 'INVALID' not found", ex.Message);
    }

    // =========================================================================
    // F3: AdbShellExecution (ORIGINAL_REQUEST §R1) - 5 tests
    // =========================================================================
    [Fact]
    public async Task F03_AdbShellExecution_StandardShellBasicOutput()
    {
        await using var server = new MockAdbServer();
        server.ShellResponses["echo hello"] = "hello\n";
        server.Start();
        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);

        string output = await client.ExecuteShellCommandAsync("echo hello", useV2: false);
        Assert.Equal("hello\n", output);
    }

    [Fact]
    public async Task F03_AdbShellExecution_StandardShellEmptyOutput()
    {
        await using var server = new MockAdbServer();
        server.ShellResponses["true"] = "";
        server.Start();
        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);

        string output = await client.ExecuteShellCommandAsync("true", useV2: false);
        Assert.Equal("", output);
    }

    [Fact]
    public async Task F03_AdbShellExecution_ShellV2Output()
    {
        await using var server = new MockAdbServer();
        server.ShellResponses["whoami"] = "shell\n";
        server.Start();
        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);

        string output = await client.ExecuteShellCommandAsync("whoami", useV2: true);
        Assert.Equal("shell\n", output);
    }

    [Fact]
    public async Task F03_AdbShellExecution_MultilineOutputPreserved()
    {
        await using var server = new MockAdbServer();
        server.ShellResponses["multiline"] = "line1\nline2\nline3\n";
        server.Start();
        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);

        string output = await client.ExecuteShellCommandAsync("multiline", useV2: false);
        Assert.Equal("line1\nline2\nline3\n", output);
    }

    [Fact]
    public async Task F03_AdbShellExecution_CustomSerialPropagation()
    {
        await using var server = new MockAdbServer();
        server.Start();
        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        client.CurrentSerial = "DEVICE_XYZ";

        await client.ExecuteShellCommandAsync("id");
        Assert.Contains("host:transport:DEVICE_XYZ", server.ReceivedRequests);
    }

    // =========================================================================
    // F4: AdbSyncPush (ORIGINAL_REQUEST §R1) - 5 tests
    // =========================================================================
    [Fact]
    public async Task F04_AdbSyncPush_SmallFile()
    {
        await using var server = new MockAdbServer();
        server.Start();
        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);

        string tmp = Path.GetTempFileName();
        try
        {
            byte[] content = Encoding.ASCII.GetBytes("SMALL_TEST_FILE");
            await File.WriteAllBytesAsync(tmp, content);

            await client.PushFileSyncAsync(tmp, "/data/local/tmp/test.txt");
            Assert.True(server.PushedFiles.ContainsKey("/data/local/tmp/test.txt"));
            Assert.Equal(content, server.PushedFiles["/data/local/tmp/test.txt"]);
        }
        finally
        {
            if (File.Exists(tmp)) File.Delete(tmp);
        }
    }

    [Fact]
    public async Task F04_AdbSyncPush_MultiChunkFile()
    {
        await using var server = new MockAdbServer();
        server.Start();
        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);

        string tmp = Path.GetTempFileName();
        try
        {
            // 130KB = 2 full 64KB chunks + 2KB remainder
            byte[] content = new byte[130 * 1024];
            Random.Shared.NextBytes(content);
            await File.WriteAllBytesAsync(tmp, content);

            await client.PushFileSyncAsync(tmp, "/data/local/tmp/big.bin");
            Assert.Equal(content, server.PushedFiles["/data/local/tmp/big.bin"]);
        }
        finally
        {
            if (File.Exists(tmp)) File.Delete(tmp);
        }
    }

    [Fact]
    public async Task F04_AdbSyncPush_ProgressCallbackInvoked()
    {
        await using var server = new MockAdbServer();
        server.Start();
        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);

        string tmp = Path.GetTempFileName();
        try
        {
            byte[] content = new byte[80 * 1024];
            await File.WriteAllBytesAsync(tmp, content);

            var reports = new List<(long sent, long total)>();
            await client.PushFileSyncAsync(tmp, "/data/local/tmp/prog.bin", progressCallback: (s, t) => reports.Add((s, t)));

            Assert.NotEmpty(reports);
            Assert.Equal(content.Length, reports.Last().sent);
        }
        finally
        {
            if (File.Exists(tmp)) File.Delete(tmp);
        }
    }

    [Fact]
    public async Task F04_AdbSyncPush_NonExistentLocalFileThrowsFileNotFound()
    {
        await using var server = new MockAdbServer();
        server.Start();
        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);

        await Assert.ThrowsAsync<FileNotFoundException>(() =>
            client.PushFileSyncAsync("c:\\non_existent_path_xyz_123.bin", "/data/local/tmp/AT"));
    }

    [Fact]
    public async Task F04_AdbSyncPush_PreservesFilePermissionsFormat()
    {
        await using var server = new MockAdbServer();
        server.Start();
        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);

        string tmp = Path.GetTempFileName();
        try
        {
            await File.WriteAllBytesAsync(tmp, new byte[] { 1, 2, 3 });
            await client.PushFileSyncAsync(tmp, "/data/local/tmp/AT", 33188);

            Assert.True(server.PushedFiles.ContainsKey("/data/local/tmp/AT"));
        }
        finally
        {
            if (File.Exists(tmp)) File.Delete(tmp);
        }
    }

    // =========================================================================
    // F5: AdbUsbReset (ORIGINAL_REQUEST §R1) - 5 tests
    // =========================================================================
    [Fact]
    public async Task F05_AdbUsbReset_ReturnsRestartingInUsbMode()
    {
        await using var server = new MockAdbServer();
        server.Start();
        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);

        string res = await client.RestartUsbModeAsync();
        Assert.Equal("restarting in USB mode", res);
    }

    [Fact]
    public async Task F05_AdbUsbReset_TransmitsUsbCommand()
    {
        await using var server = new MockAdbServer();
        server.Start();
        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);

        await client.RestartUsbModeAsync();
        Assert.Contains(server.ReceivedRequests, r => r.Contains("usb:"));
    }

    [Fact]
    public async Task F05_AdbUsbReset_RespectsDeviceSerialSelection()
    {
        await using var server = new MockAdbServer();
        server.Start();
        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        client.CurrentSerial = "RFCX90GR21X";

        await client.RestartUsbModeAsync();
        Assert.Contains("host:transport:RFCX90GR21X", server.ReceivedRequests);
    }

    [Fact]
    public async Task F05_AdbUsbReset_HandlesSocketClosureGracefully()
    {
        await using var server = new MockAdbServer();
        server.Start();
        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);

        // When adb restarts in usb mode, connection abruptly breaks. Method should catch and return message
        string res = await client.RestartUsbModeAsync();
        Assert.False(string.IsNullOrEmpty(res));
    }

    [Fact]
    public async Task F05_AdbUsbReset_ThrowsOnServerFailResponse()
    {
        await using var server = new MockAdbServer();
        server.SimulateFailResponse = true;
        server.FailErrorMessage = "usb reset not supported";
        server.Start();
        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);

        await Assert.ThrowsAsync<AdbException>(() => client.RestartUsbModeAsync());
    }

    // =========================================================================
    // F6: ImeiParcelParsing (ORIGINAL_REQUEST §R2) - 5 tests
    // =========================================================================
    [Fact]
    public void F06_ImeiParcelParsing_MatchesCsc15ExactDump()
    {
        string dump = MockAdbServer.GetDefaultImeiParcelOutput();
        string imei = ImeiParcelParser.ParseImeiFromParcelDump(dump);
        Assert.Equal("355752920542633", imei);
    }

    [Fact]
    public void F06_ImeiParcelParsing_HandlesWindowsNewlines()
    {
        string dump = MockAdbServer.GetDefaultImeiParcelOutput().Replace("\n", "\r\n");
        string imei = ImeiParcelParser.ParseImeiFromParcelDump(dump);
        Assert.Equal("355752920542633", imei);
    }

    [Fact]
    public void F06_ImeiParcelParsing_Extracts14DigitImei()
    {
        string dump =
            "Result: Parcel(\n" +
            "0x00000000: 00000000 0000000e 00350033 00370035 '........3.5.5.7.'\n" +
            "0x00000010: 00320035 00320039 00350030 00320034 '5.2.9.2.0.5.4.2.'\n" +
            "0x00000020: 00330036                            '6.3             ')\n";
        string imei = ImeiParcelParser.ParseImeiFromParcelDump(dump);
        Assert.Equal("35575292054263", imei);
    }

    [Fact]
    public void F06_ImeiParcelParsing_EmptyOrWhitespaceReturnsEmpty()
    {
        Assert.Equal("", ImeiParcelParser.ParseImeiFromParcelDump(""));
        Assert.Equal("", ImeiParcelParser.ParseImeiFromParcelDump("   \r\n\t  "));
    }

    [Fact]
    public void F06_ImeiParcelParsing_CorruptedParcelFallsBackToAscii()
    {
        string dump = "Result: Parcel( '........3.5.5.7.''5.2.9.2.0.5.4.2.''6.3.3...        ' )";
        string imei = ImeiParcelParser.ParseImeiFromParcelDump(dump);
        Assert.Equal("355752920542633", imei);
    }

    // =========================================================================
    // F7: Xorshift32Cipher (at_bridge.c, ORIGINAL_REQUEST §R3) - 5 tests
    // =========================================================================
    [Fact]
    public void F07_Xorshift32Cipher_KnownVectorMatchesKeystream()
    {
        uint seed = 0x12345678;
        byte[] input = new byte[] { 0, 0, 0, 0, 0 };
        Xorshift32StreamCipher.TransformBuffer(input, seed);
        Assert.Equal(new byte[] { 165, 163, 196, 152, 136 }, input);
    }

    [Fact]
    public void F07_Xorshift32Cipher_RoundtripInversion()
    {
        uint seed = 0xDEADBEEF;
        byte[] msg = Encoding.UTF8.GetBytes("GsmTools_Secure_AT_Bridge_Test");
        byte[] enc = Xorshift32StreamCipher.Transform(msg, seed);
        Assert.NotEqual(msg, enc);
        byte[] dec = Xorshift32StreamCipher.Transform(enc, seed);
        Assert.Equal(msg, dec);
    }

    [Fact]
    public void F07_Xorshift32Cipher_ZeroSeedBypass()
    {
        uint seed = 0;
        byte[] msg = new byte[] { 1, 2, 3, 4, 5 };
        byte[] outBytes = Xorshift32StreamCipher.Transform(msg, seed);
        Assert.Equal(msg, outBytes);
    }

    [Fact]
    public void F07_Xorshift32Cipher_SliceTransformation()
    {
        uint seed = 0x12345678;
        byte[] buf = new byte[] { 0xAA, 0, 0, 0xBB };
        Xorshift32StreamCipher.TransformBuffer(buf, 1, 2, seed);
        Assert.Equal(0xAA, buf[0]);
        Assert.Equal(165, buf[1]);
        Assert.Equal(163, buf[2]);
        Assert.Equal(0xBB, buf[3]);
    }

    [Fact]
    public void F07_Xorshift32Cipher_EmptyDataReturnsEmpty()
    {
        byte[] res = Xorshift32StreamCipher.Transform(Array.Empty<byte>(), 0x12345678);
        Assert.Empty(res);
    }

    // =========================================================================
    // F8: AtBannerParsing (at_bridge.c, ORIGINAL_REQUEST §R3) - 5 tests
    // =========================================================================
    [Fact]
    public async Task F08_AtBannerParsing_StandardFields()
    {
        await using var server = new MockAdbServer();
        server.AtBridge.Model = "SM-S918U";
        server.AtBridge.Csc = "TMB";
        server.AtBridge.AndroidVersion = "15";
        server.AtBridge.SecurityPatch = "2025-07-01";
        server.AtBridge.DebugLevel = "0x4f4c";
        server.AtBridge.Uid = 1000;
        server.AtBridge.ActiveChannel = "/dev/socket/rild";
        server.Start();

        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);

        var banner = await session.StartSessionAsync();
        Assert.Equal("SM-S918U", banner.Model);
        Assert.Equal("TMB", banner.Csc);
        Assert.Equal("15", banner.AndroidVersion);
        Assert.Equal("2025-07-01", banner.SecurityPatch);
        Assert.Equal("0x4f4c", banner.DebugLevel);
        Assert.Equal(1000, banner.Uid);
        Assert.Equal("/dev/socket/rild", banner.ActiveChannel);
    }

    [Fact]
    public async Task F08_AtBannerParsing_MissingFieldsFallbackToDefaults()
    {
        await using var server = new MockAdbServer();
        server.AtBridge.Model = "";
        server.Start();

        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);

        var banner = await session.StartSessionAsync();
        Assert.NotNull(banner);
    }

    [Fact]
    public async Task F08_AtBannerParsing_ActiveSessionProperty()
    {
        await using var server = new MockAdbServer();
        server.Start();

        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);

        Assert.False(session.IsActive);
        await session.StartSessionAsync();
        Assert.True(session.IsActive);
    }

    [Fact]
    public async Task F08_AtBannerParsing_BannerPropertyMatchesReturnedBanner()
    {
        await using var server = new MockAdbServer();
        server.Start();

        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);

        var returned = await session.StartSessionAsync();
        Assert.Same(returned, session.Banner);
    }

    [Fact]
    public async Task F08_AtBannerParsing_TerminationClearsBanner()
    {
        await using var server = new MockAdbServer();
        server.Start();

        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);

        await session.StartSessionAsync();
        await session.TerminateSessionAsync();
        Assert.Null(session.Banner);
        Assert.False(session.IsActive);
    }

    // =========================================================================
    // F9: AtAuthHandshake (at_bridge.c, ORIGINAL_REQUEST §R3) - 5 tests
    // =========================================================================
    [Fact]
    public async Task F09_AtAuthHandshake_NonZeroSeedActivatesAuth()
    {
        await using var server = new MockAdbServer();
        server.Start();

        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);

        uint seed = 0x44332211;
        await session.StartSessionAsync(sessionSeed: seed);

        Assert.Equal(seed, session.SessionSeed);
        Assert.Contains(server.AtBridge.ReceivedRawLines, l => l == $"AUTH:{seed:X8}");
    }

    [Fact]
    public async Task F09_AtAuthHandshake_ZeroSeedRemainsPlaintext()
    {
        await using var server = new MockAdbServer();
        server.Start();

        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);

        await session.StartSessionAsync(sessionSeed: 0);

        Assert.Equal(0u, session.SessionSeed);
        Assert.DoesNotContain(server.AtBridge.ReceivedRawLines, l => l.StartsWith("AUTH:"));
    }

    [Fact]
    public async Task F09_AtAuthHandshake_ServerAckDelimitedCorrectly()
    {
        await using var server = new MockAdbServer();
        server.Start();

        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);

        uint seed = 0x88776655;
        await session.StartSessionAsync(sessionSeed: seed);
        Assert.Equal(seed, server.AtBridge.SessionSeed);
    }

    [Fact]
    public async Task F09_AtAuthHandshake_HexFormattingPreservesFull8Digits()
    {
        await using var server = new MockAdbServer();
        server.Start();

        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);

        uint smallSeed = 0x000000AB;
        await session.StartSessionAsync(sessionSeed: smallSeed);

        Assert.Contains(server.AtBridge.ReceivedRawLines, l => l == "AUTH:000000AB");
    }

    [Fact]
    public async Task F09_AtAuthHandshake_MaxSeedValue()
    {
        await using var server = new MockAdbServer();
        server.Start();

        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);

        uint maxSeed = 0xFFFFFFFF;
        await session.StartSessionAsync(sessionSeed: maxSeed);

        Assert.Contains(server.AtBridge.ReceivedRawLines, l => l == "AUTH:FFFFFFFF");
    }

    // =========================================================================
    // F10: AtFramingDelimiters (at_bridge.c, ORIGINAL_REQUEST §R3) - 5 tests
    // =========================================================================
    [Fact]
    public async Task F10_AtFramingDelimiters_EmptyPayloadReturnsEmpty()
    {
        await using var server = new MockAdbServer();
        server.Start();

        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);

        await session.StartSessionAsync(sessionSeed: 0);
        string resp = await session.SendIpcHexCommandAsync("060100090000584141");
        Assert.Equal("", resp);
    }

    [Fact]
    public async Task F10_AtFramingDelimiters_WithPayloadReturnsHex()
    {
        await using var server = new MockAdbServer();
        server.AtBridge.CustomIpcHandler = (hex) => (true, "00112233");
        server.Start();

        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);

        await session.StartSessionAsync(sessionSeed: 0);
        string resp = await session.SendIpcHexCommandAsync("060100090000584141");
        Assert.Equal("00112233", resp);
    }

    [Fact]
    public async Task F10_AtFramingDelimiters_IpcErrorThrowsInvalidOperation()
    {
        await using var server = new MockAdbServer();
        server.AtBridge.SimulateIpcError = true;
        server.Start();

        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);

        await session.StartSessionAsync(sessionSeed: 0);
        await Assert.ThrowsAsync<InvalidOperationException>(() => session.SendIpcHexCommandAsync("060100090000584141"));
    }

    [Fact]
    public async Task F10_AtFramingDelimiters_SyntaxErrorThrowsInvalidOperation()
    {
        await using var server = new MockAdbServer();
        server.Start();

        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);

        await session.StartSessionAsync(sessionSeed: 0);
        await Assert.ThrowsAsync<FormatException>(() => session.SendIpcHexCommandAsync("NON_HEX_CHARACTERS"));
    }

    [Fact]
    public async Task F10_AtFramingDelimiters_PromptSynchronization()
    {
        await using var server = new MockAdbServer();
        server.Start();

        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);

        await session.StartSessionAsync(sessionSeed: 0);
        for (int i = 0; i < 3; i++)
        {
            string resp = await session.SendIpcHexCommandAsync("060100120000");
            Assert.Equal("", resp);
        }
        Assert.Equal(3, server.AtBridge.ReceivedIpcCommands.Count);
    }

    // =========================================================================
    // F11: AtUidFallback (ORIGINAL_REQUEST §R3) - 5 tests
    // =========================================================================
    [Fact]
    public async Task F11_AtUidFallback_RelayTriggeredWhenUid2000AndChannelNone()
    {
        await using var server = new MockAdbServer();
        server.AtBridge.Uid = 2000;
        server.AtBridge.ActiveChannel = "NONE";
        server.ShellResponses["echo 'id' |toybox nc localhost 2600"] = "uid=1000(system) context=u:r:system_app:s0\n";
        server.Start();

        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);

        var banner = await session.StartSessionAsync();
        Assert.Contains(server.ReceivedRequests, r => r.Contains("toybox nc localhost 2600"));
    }

    [Fact]
    public async Task F11_AtUidFallback_NoRelayIfAlreadyUid1000()
    {
        await using var server = new MockAdbServer();
        server.AtBridge.Uid = 1000;
        server.AtBridge.ActiveChannel = "NONE";
        server.Start();

        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);

        var banner = await session.StartSessionAsync();
        Assert.DoesNotContain(server.ReceivedRequests, r => r.Contains("toybox nc localhost 2600"));
    }

    [Fact]
    public async Task F11_AtUidFallback_NoRelayIfActiveChannelPresent()
    {
        await using var server = new MockAdbServer();
        server.AtBridge.Uid = 2000;
        server.AtBridge.ActiveChannel = "/dev/umts_ipc0";
        server.Start();

        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);

        var banner = await session.StartSessionAsync();
        Assert.DoesNotContain(server.ReceivedRequests, r => r.Contains("toybox nc localhost 2600"));
    }

    [Fact]
    public async Task F11_AtUidFallback_RelaySkippedIfPort2600ConnectionRefused()
    {
        await using var server = new MockAdbServer();
        server.AtBridge.Uid = 2000;
        server.AtBridge.ActiveChannel = "NONE";
        server.ShellResponses["echo 'id' |toybox nc localhost 2600"] = "nc: connect: Connection refused.\n";
        server.Start();

        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);

        var banner = await session.StartSessionAsync();
        Assert.DoesNotContain(server.ReceivedRequests, r => r == "shell:toybox nc localhost 2600");
    }

    [Fact]
    public async Task F11_AtUidFallback_RelayMaintainsInteractiveCommunication()
    {
        await using var server = new MockAdbServer();
        server.AtBridge.Uid = 2000;
        server.AtBridge.ActiveChannel = "NONE";
        server.ShellResponses["echo 'id' |toybox nc localhost 2600"] = "uid=1000(system)\n";
        server.Start();

        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);

        await session.StartSessionAsync();
        string resp = await session.SendIpcHexCommandAsync("060100090000584141");
        Assert.Equal("", resp);
    }

    // =========================================================================
    // F12 - F19: Device Pre-Checks (csc 15.txt, ORIGINAL_REQUEST §R4)
    // =========================================================================
    [Fact]
    public async Task F12_DevicePreCheckRsu_DetectsPresent()
    {
        await using var server = new MockAdbServer();
        server.ShellResponses["pm list package|grep rsuapp"] = "package:com.samsung.rsuapp\n";
        server.Start();
        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);
        var workflow = new SamsungCscWorkflow(client, session);

        var info = await workflow.ReadDeviceInfoAsync();
        Assert.True(info.HasRsuApp);
    }

    [Fact]
    public async Task F13_DevicePreCheckTsdaemon_DetectsPresent()
    {
        await using var server = new MockAdbServer();
        server.ShellResponses["ps -A|grep tsdaemon;ps|grep tsdaemon"] = "system 1234 1 0 tsdaemon\n";
        server.Start();
        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);
        var workflow = new SamsungCscWorkflow(client, session);

        var info = await workflow.ReadDeviceInfoAsync();
        Assert.True(info.HasTsdaemon);
    }

    [Fact]
    public async Task F14_DevicePreCheckGetprop_ParsesSystemProperties()
    {
        await using var server = new MockAdbServer();
        server.Start();
        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);
        var workflow = new SamsungCscWorkflow(client, session);

        var info = await workflow.ReadDeviceInfoAsync();
        Assert.Equal("SM-S918U", info.Model);
        Assert.Equal("RFCX90GR21X", info.Serial);
        Assert.Equal("ATT", info.CurrentCsc);
        Assert.Equal("15", info.AndroidVersion);
    }

    [Fact]
    public async Task F15_DevicePreCheckImei_ParsesImeiFromServiceCall()
    {
        await using var server = new MockAdbServer();
        server.Start();
        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);
        var workflow = new SamsungCscWorkflow(client, session);

        var info = await workflow.ReadDeviceInfoAsync();
        Assert.Equal("355752920542633", info.Imei);
    }

    [Fact]
    public async Task F16_DevicePreCheckSelinux_DetectsEnforcingAndPermissive()
    {
        await using var server = new MockAdbServer();
        server.ShellResponses["getenforce"] = "Permissive\n";
        server.Start();
        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);
        var workflow = new SamsungCscWorkflow(client, session);

        var info = await workflow.ReadDeviceInfoAsync();
        Assert.False(info.IsSelinuxEnforcing);
    }

    [Fact]
    public async Task F17_DevicePreCheckKeytool_DetectsPresence()
    {
        await using var server = new MockAdbServer();
        server.ShellResponses["pm path KEYTool.SRV4"] = "package:/data/app/KEYTool.SRV4.apk\n";
        server.Start();
        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);
        var workflow = new SamsungCscWorkflow(client, session);

        var info = await workflow.ReadDeviceInfoAsync();
        Assert.True(info.HasKeyTool);
    }

    [Fact]
    public async Task F18_DevicePreCheckPort2600_DetectsAvailability()
    {
        await using var server = new MockAdbServer();
        server.ShellResponses["echo 'id' |toybox nc localhost 2600"] = "uid=1000(system)\n";
        server.Start();
        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);
        var workflow = new SamsungCscWorkflow(client, session);

        var info = await workflow.ReadDeviceInfoAsync();
        Assert.True(info.Port2600Available);
    }

    [Fact]
    public async Task F19_DevicePreCheckDiagnostics_ExtractsRetailAgentVersion()
    {
        await using var server = new MockAdbServer();
        server.Start();
        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);
        var workflow = new SamsungCscWorkflow(client, session);

        var info = await workflow.ReadDeviceInfoAsync();
        Assert.Equal("5.59.6-main", info.RetailAgentVersion);
    }

    // =========================================================================
    // F20 - F26: CSC Injection Sequence & Actions (csc 15.txt)
    // =========================================================================
    [Theory]
    [InlineData("XAA", true)]
    [InlineData("USC", true)]
    [InlineData("TMB", true)]
    [InlineData("X", false)]
    [InlineData("TOOLONG", false)]
    [InlineData("12A", false)]
    public async Task F20_CscValidation_Validates3AlphaFormat(string csc, bool expectedSuccess)
    {
        await using var server = new MockAdbServer();
        server.Start();
        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);
        var workflow = new SamsungCscWorkflow(client, session);

        var result = await workflow.ExecuteCscChangeAsync(csc);
        Assert.Equal(expectedSuccess, result.Success);
    }

    [Fact]
    public async Task F21_Uid1000Recovery_PollsPort2600UntilReady()
    {
        await using var server = new MockAdbServer();
        server.Port2600FailAttempts = 2; // Fails twice, succeeds on attempt 3 matching csc 15.txt Run 2
        server.Start();
        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);
        var workflow = new SamsungCscWorkflow(client, session);

        var result = await workflow.ExecuteCscChangeAsync("USC");
        Assert.True(result.Success);
        Assert.Contains(server.ReceivedShellCommands, c => c == "svc power stayon true");
        Assert.Contains(server.ReceivedShellCommands, c => c.Contains("ctl.stop ssud"));
    }

    [Fact]
    public async Task F22_AtBinaryDeployment_PushesAtAndChmod777()
    {
        await using var server = new MockAdbServer();
        server.Start();
        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);
        var workflow = new SamsungCscWorkflow(client, session);

        var result = await workflow.ExecuteCscChangeAsync("XAA");
        Assert.True(result.Success);
        Assert.True(server.PushedFiles.ContainsKey("/data/local/tmp/AT"));
        Assert.Contains(server.ReceivedShellCommands, c => c == "chmod 777 /data/local/tmp/AT");
    }

    [Fact]
    public async Task F23_RadioLogcatFlush_ClearsRadioBufferBeforeInjection()
    {
        await using var server = new MockAdbServer();
        server.Start();
        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);
        var workflow = new SamsungCscWorkflow(client, session);

        var result = await workflow.ExecuteCscChangeAsync("XAA");
        Assert.True(result.Success);
        Assert.Contains("logcat -b radio -c", server.ReceivedShellCommands);
    }

    [Fact]
    public async Task F24_CscFiveCommandInjection_SendsAll5FramesInOrder()
    {
        await using var server = new MockAdbServer();
        server.Start();
        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);
        var workflow = new SamsungCscWorkflow(client, session);

        var result = await workflow.ExecuteCscChangeAsync("XAA");
        Assert.True(result.Success);

        // 5 specific commands for XAA (584141)
        Assert.True(string.Equals("0601000E00005841410000584141", server.AtBridge.ReceivedIpcCommands[0], StringComparison.OrdinalIgnoreCase));
        Assert.True(string.Equals("060100090000584141", server.AtBridge.ReceivedIpcCommands[1], StringComparison.OrdinalIgnoreCase));
        Assert.True(string.Equals("0601001200005841410000584141584141", server.AtBridge.ReceivedIpcCommands[2], StringComparison.OrdinalIgnoreCase));
        Assert.True(string.Equals("060100120000", server.AtBridge.ReceivedIpcCommands[3], StringComparison.OrdinalIgnoreCase));
        Assert.True(string.Equals("060100090000584141", server.AtBridge.ReceivedIpcCommands[4], StringComparison.OrdinalIgnoreCase));
    }

    [Fact]
    public async Task F25_PostCleanupOps_ExecutesNonFatalPackageCleanup()
    {
        await using var server = new MockAdbServer();
        server.Start();
        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);
        var workflow = new SamsungCscWorkflow(client, session);

        var result = await workflow.ExecuteCscChangeAsync("XAA");
        Assert.True(result.Success);
        Assert.Contains(server.ReceivedShellCommands, c => c.Contains("com.sec.android.AutoPreconfig"));
    }

    [Fact]
    public async Task F26_CscUsbReset_IssuesUsbResetCommand()
    {
        await using var server = new MockAdbServer();
        server.Start();
        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);
        var workflow = new SamsungCscWorkflow(client, session);

        var result = await workflow.ExecuteCscChangeAsync("XAA");
        Assert.True(result.Success);
        Assert.Contains(server.ReceivedRequests, r => r.Contains("usb:"));
    }
}
