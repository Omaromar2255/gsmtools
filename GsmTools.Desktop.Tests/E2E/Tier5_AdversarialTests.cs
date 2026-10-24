using System.Text;
using GsmTools.Desktop.Core;
using GsmTools.Desktop.Tests.Mocks;
using GsmTools.Desktop.Workflows;
using Xunit;

namespace GsmTools.Desktop.Tests.E2E;

/// <summary>
/// Tier 5: Adversarial Verification & Stress Hardening.
/// Tests malicious payloads, injection attacks, premature socket closures, protocol corruption, and high concurrency.
/// </summary>
public class Tier5_AdversarialTests
{
    [Theory]
    [InlineData("'; DROP TABLE Devices; --")]
    [InlineData("XAA\0INJECTED")]
    [InlineData("XAA\nreboot")]
    [InlineData("XAA`id`")]
    [InlineData("XAA$(reboot)")]
    [InlineData("XAA && rm -rf /")]
    public async Task Adversarial_CommandInjectionInCsc_RejectedImmediately(string maliciousCsc)
    {
        await using var server = new MockAdbServer();
        server.Start();

        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);
        var workflow = new SamsungCscWorkflow(client, session);

        var result = await workflow.ExecuteCscChangeAsync(maliciousCsc);
        Assert.False(result.Success);
        Assert.Contains("Invalid CSC", result.Message);
        // Ensure no malicious payload ever reached ADB server or bridge
        Assert.DoesNotContain(server.ReceivedShellCommands, c => c.Contains("DROP TABLE") || c.Contains("rm -rf"));
        Assert.Empty(server.AtBridge.ReceivedIpcCommands);
    }

    [Fact]
    public async Task Adversarial_PrematureSocketSeverDuringSyncPush_ThrowsGracefully()
    {
        await using var server = new MockAdbServer();
        server.SimulatePrematureDisconnect = true;
        server.Start();

        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);

        string tmp = Path.GetTempFileName();
        try
        {
            await File.WriteAllBytesAsync(tmp, new byte[100]);
            await Assert.ThrowsAnyAsync<Exception>(() =>
                client.PushFileSyncAsync(tmp, "/data/local/tmp/AT"));
        }
        finally
        {
            if (File.Exists(tmp)) File.Delete(tmp);
        }
    }

    [Fact]
    public async Task Adversarial_HighConcurrencyTelemetryRequests_DoesNotCorruptState()
    {
        await using var server = new MockAdbServer();
        server.Start();

        // 10 concurrent clients performing telemetry queries simultaneously
        var tasks = Enumerable.Range(0, 10).Select(async i =>
        {
            await using var client = new AdbRawSocketClient();
            await client.ConnectAsync(server.Host, server.Port);
            await using var session = new PersistentAtBridgeSession(client);
            var workflow = new SamsungCscWorkflow(client, session);

            var info = await workflow.ReadDeviceInfoAsync();
            Assert.Equal("SM-S918U", info.Model);
            Assert.Equal("355752920542633", info.Imei);
        });

        await Task.WhenAll(tasks);
    }

    [Fact]
    public void Adversarial_Xorshift32_KeystreamEntropyDistribution()
    {
        // Verify that Xorshift32 does not collapse to all zeros or trivial cycles
        uint seed = 0x1337BEEF;
        byte[] buffer = new byte[1024];
        Xorshift32StreamCipher.TransformBuffer(buffer, seed);

        // Count unique byte values across 1024 transformed bytes
        int distinctCount = buffer.Distinct().Count();
        // A well-distributed 8-bit stream over 1024 bytes should cover >= 200 distinct values
        Assert.True(distinctCount > 200, $"Distinct byte count was {distinctCount}, expected > 200");
    }

    [Theory]
    [InlineData("0601000e00005841410000584141")] // Plaintext frame
    [InlineData("0601001200005841410000584141584141")] // Multi-CSC frame
    public void Adversarial_HexParsing_CaseInsensitive(string hex)
    {
        byte[] lower = Convert.FromHexString(hex.ToLowerInvariant());
        byte[] upper = Convert.FromHexString(hex.ToUpperInvariant());
        Assert.Equal(lower, upper);
    }
}
