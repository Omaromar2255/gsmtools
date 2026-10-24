using System.Text;
using GsmTools.Desktop.Core;
using GsmTools.Desktop.Tests.Mocks;
using GsmTools.Desktop.Workflows;
using Xunit;

namespace GsmTools.Desktop.Tests.E2E;

/// <summary>
/// Tier 2: Boundary Value Analysis (BVA) & Corner Case Stress Tests.
/// Covers payload size extremes, zero lengths, buffer fragmentation, corrupt framing, and boundary seeds.
/// </summary>
public class Tier2_BoundaryTests
{
    [Theory]
    [InlineData(0)]           // 0 bytes (empty file)
    [InlineData(1)]           // 1 byte (minimal payload)
    [InlineData(64 * 1024)]   // exactly 64KB (chunk boundary)
    [InlineData(64 * 1024 + 1)]// 64KB + 1 byte (chunk boundary + 1)
    [InlineData(128 * 1024)]  // 2 full 64KB chunks
    public async Task PushFileSync_ChunkSizeBoundaries_TransfersCorrectByteCount(int size)
    {
        await using var server = new MockAdbServer();
        server.Start();
        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);

        string tmp = Path.GetTempFileName();
        try
        {
            byte[] buffer = new byte[size];
            if (size > 0)
            {
                Random.Shared.NextBytes(buffer);
            }
            await File.WriteAllBytesAsync(tmp, buffer);

            string target = $"/data/local/tmp/chunk_test_{size}.bin";
            await client.PushFileSyncAsync(tmp, target);

            Assert.True(server.PushedFiles.ContainsKey(target));
            Assert.Equal(size, server.PushedFiles[target].Length);
            Assert.Equal(buffer, server.PushedFiles[target]);
        }
        finally
        {
            if (File.Exists(tmp)) File.Delete(tmp);
        }
    }

    [Theory]
    [InlineData(0u)]
    [InlineData(1u)]
    [InlineData(0x7FFFFFFFu)]
    [InlineData(0x80000000u)]
    [InlineData(0xFFFFFFFFu)]
    public void Xorshift32_SeedBoundaries_ProduceConsistentReversibleKeystream(uint seed)
    {
        byte[] payload = new byte[256];
        for (int i = 0; i < payload.Length; i++) payload[i] = (byte)i;

        byte[] encrypted = Xorshift32StreamCipher.Transform(payload, seed);
        Assert.Equal(payload.Length, encrypted.Length);

        byte[] decrypted = Xorshift32StreamCipher.Transform(encrypted, seed);
        Assert.Equal(payload, decrypted);
    }

    [Fact]
    public void Xorshift32_ExtremeLengths_ZeroAndSingleByte()
    {
        // 0-byte buffer
        byte[] zeroLen = Array.Empty<byte>();
        Xorshift32StreamCipher.TransformBuffer(zeroLen, 0x12345678);
        Assert.Empty(zeroLen);

        // 1-byte buffer
        byte[] singleByte = new byte[] { 0x55 };
        Xorshift32StreamCipher.TransformBuffer(singleByte, 0x12345678);
        // seed 0x12345678 first byte is 0xA5 -> 0x55 ^ 0xA5 = 0xF0
        Assert.Equal(0xF0, singleByte[0]);
    }

    [Theory]
    [InlineData("0x00000000: 00000000 00000000")] // 0 length
    [InlineData("0x00000000: 00000000 00000003 00310032 '...1.2'")] // 3 length (< 14 digits)
    [InlineData("0x00000000: 00000000 00000018 00310032 '...1.2'")] // 24 length (> 16 digits)
    [InlineData("Result: Parcel()")] // Empty parcel
    [InlineData("0x00000000: 00000001 0000000f 'Error status'")] // Non-zero status
    public void ImeiParcelParser_MalformedOrBoundaryParcels_ReturnEmpty(string malformed)
    {
        string imei = ImeiParcelParser.ParseImeiFromParcelDump(malformed);
        Assert.Equal("", imei);
    }

    [Fact]
    public async Task PersistentAtBridgeSession_ServerDelimiterError_ThrowsInvalidOperation()
    {
        await using var server = new MockAdbServer();
        server.AtBridge.SimulateIpcError = true;
        server.Start();

        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);

        await session.StartSessionAsync();
        var ex = await Assert.ThrowsAsync<InvalidOperationException>(() =>
            session.SendIpcHexCommandAsync("060100090000584141"));
        Assert.Contains(".ERR_IPC..", ex.Message);
    }

    [Fact]
    public async Task PersistentAtBridgeSession_PromptAtBufferBoundary_ParsedCorrectly()
    {
        // Tests that reading until prompt handles multiple chunk transmissions cleanly
        await using var server = new MockAdbServer();
        server.Start();

        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);

        await session.StartSessionAsync();

        // Send 10 consecutive commands in rapid succession
        for (int i = 0; i < 10; i++)
        {
            string resp = await session.SendIpcHexCommandAsync("060100120000");
            Assert.Equal("", resp);
        }
    }

    [Fact]
    public async Task SamsungCscWorkflow_Port2600Exhaustion_DoesNotCrashAndContinues()
    {
        await using var server = new MockAdbServer();
        server.Port2600FailAttempts = 100; // Will never succeed within retry limit
        server.Start();

        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);
        var workflow = new SamsungCscWorkflow(client, session);

        // When port 2600 fails to confirm, workflow logs notice and proceeds to attempt bridge
        var result = await workflow.ExecuteCscChangeAsync("XAA");
        Assert.True(result.Success);
    }
}
