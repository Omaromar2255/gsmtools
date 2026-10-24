using System.IO;
using System.Text;
using GsmTools.Desktop.Core;
using GsmTools.Desktop.Tests.Mocks;
using Xunit;

namespace GsmTools.Desktop.Tests;

public class WorkflowE2ETests
{
    [Fact]
    public async Task AdbRawSocketClient_WithMockServer_ExecutesShellAndSyncPush()
    {
        await using var mockServer = new MockAdbServer();
        mockServer.Start();
        var client = new AdbRawSocketClient();

        await client.ConnectAsync("127.0.0.1", mockServer.Port);
        Assert.True(client.IsConnected);

        await client.SelectDeviceAsync("RFCX90GR21X");

        // Shell getprop test
        string getprop = await client.ExecuteShellCommandAsync("getprop");
        Assert.Contains("SM-S918U", getprop);
        Assert.Contains("RFCX90GR21X", getprop);

        // Shell iphonesubinfo test
        string imeiDump = await client.ExecuteShellCommandAsync("service call iphonesubinfo");
        string imei = ImeiParcelParser.ParseImeiFromParcelDump(imeiDump);
        Assert.Equal("355752920542633", imei);

        // Sync push test
        string tempFile = Path.GetTempFileName();
        try
        {
            await File.WriteAllBytesAsync(tempFile, new byte[128 * 1024]); // 128KB file to test chunking
            await client.PushFileSyncAsync(tempFile, "/data/local/tmp/AT", 33188);
        }
        finally
        {
            if (File.Exists(tempFile))
                File.Delete(tempFile);
        }

        // USB mode reset test
        string usbResp = await client.RestartUsbModeAsync();
        Assert.Contains("restarting in USB mode", usbResp);
    }
}
