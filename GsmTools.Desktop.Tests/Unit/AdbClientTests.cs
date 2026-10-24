using System.Text;
using GsmTools.Desktop.Core;
using GsmTools.Desktop.Tests.Mocks;
using Xunit;

namespace GsmTools.Desktop.Tests.Unit;

public class AdbClientTests : IAsyncLifetime
{
    private readonly MockAdbServer _server = new();
    private readonly AdbRawSocketClient _client = new();

    public async Task InitializeAsync()
    {
        _server.Start();
        await _client.ConnectAsync(_server.Host, _server.Port);
    }

    public async Task DisposeAsync()
    {
        await _client.DisposeAsync();
        await _server.DisposeAsync();
    }

    [Fact]
    public async Task SendAdbRequestAsync_ValidRequest_ReturnsOkay()
    {
        // F1: AdbTcpFraming
        string resp = await _client.SendAdbRequestAsync("host:version");
        Assert.Equal("OKAY", resp);
        Assert.Contains("host:version", _server.ReceivedRequests);
    }

    [Fact]
    public async Task SendAdbRequestAsync_ServerReturnsFail_ThrowsAdbExceptionWithMessage()
    {
        // F1: AdbTcpFraming - Error Handling
        _server.SimulateFailResponse = true;
        _server.FailErrorMessage = "device unauthorized";

        var ex = await Assert.ThrowsAsync<AdbException>(() => _client.SendAdbRequestAsync("host:version"));
        Assert.Contains("device unauthorized", ex.Message);
    }

    [Fact]
    public async Task SelectDeviceAsync_WithSerial_SendsHostTransportWithSerial()
    {
        // F2: AdbTransportSwitch
        await _client.SelectDeviceAsync("RFCX90GR21X");

        Assert.Equal("RFCX90GR21X", _client.CurrentSerial);
        Assert.Contains("host:transport:RFCX90GR21X", _server.ReceivedRequests);
    }

    [Fact]
    public async Task SelectDeviceAsync_NullSerial_SendsHostTransportAny()
    {
        // F2: AdbTransportSwitch
        await _client.SelectDeviceAsync(null);

        Assert.Null(_client.CurrentSerial);
        Assert.Contains("host:transport-any", _server.ReceivedRequests);
    }

    [Fact]
    public async Task ExecuteShellCommandAsync_StandardShell_ReturnsCommandOutput()
    {
        // F3: AdbShellExecution
        _server.ShellResponses["getprop ro.product.model"] = "SM-S918U\n";

        string output = await _client.ExecuteShellCommandAsync("getprop ro.product.model", useV2: false);

        Assert.Equal("SM-S918U\n", output);
        Assert.Contains("getprop ro.product.model", _server.ReceivedShellCommands);
    }

    [Fact]
    public async Task ExecuteShellCommandAsync_ShellV2_DecodesStreamFrames()
    {
        // F3: AdbShellExecution (v2 framing)
        _server.ShellResponses["uname -a"] = "Linux localhost 5.15.0\n";

        string output = await _client.ExecuteShellCommandAsync("uname -a", useV2: true);

        Assert.Equal("Linux localhost 5.15.0\n", output);
    }

    [Fact]
    public async Task PushFileSyncAsync_PushesBinaryPayloadInChunks()
    {
        // F4: AdbSyncPush
        string tempFile = Path.GetTempFileName();
        try
        {
            // Create 150KB dummy binary file (will be split into 64KB + 64KB + 22KB chunks)
            byte[] dummyData = new byte[150 * 1024];
            Random.Shared.NextBytes(dummyData);
            await File.WriteAllBytesAsync(tempFile, dummyData);

            long lastSent = 0;
            await _client.PushFileSyncAsync(tempFile, "/data/local/tmp/AT", 33188, (sent, total) =>
            {
                lastSent = sent;
            });

            Assert.Equal(dummyData.Length, lastSent);
            Assert.True(_server.PushedFiles.ContainsKey("/data/local/tmp/AT"));
            Assert.Equal(dummyData, _server.PushedFiles["data/local/tmp/AT".StartsWith("/") ? "/data/local/tmp/AT" : "/data/local/tmp/AT"]);
        }
        finally
        {
            if (File.Exists(tempFile)) File.Delete(tempFile);
        }
    }

    [Fact]
    public async Task RestartUsbModeAsync_SendsDirectUsbRequest_ReturnsRestartingInUsbMode()
    {
        // F5: AdbUsbReset
        string result = await _client.RestartUsbModeAsync();

        Assert.Contains("restarting in USB mode", result);
        Assert.Contains(_server.ReceivedRequests, r => r.Contains("usb:"));
    }
}
