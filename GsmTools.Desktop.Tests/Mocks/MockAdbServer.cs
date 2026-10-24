using System.Buffers.Binary;
using System.Net;
using System.Net.Sockets;
using System.Text;
using GsmTools.Desktop.Core;

namespace GsmTools.Desktop.Tests.Mocks;

/// <summary>
/// In-memory TCP server listening on loopback (ephemeral port 127.0.0.1:0),
/// fully mimicking the ADB server daemon protocol, handshake, device transport,
/// shell commands, binary sync push protocol, and direct usb mode reset.
/// </summary>
public class MockAdbServer : IAsyncDisposable
{
    private readonly TcpListener _listener;
    private readonly CancellationTokenSource _cts = new();
    private Task? _listenTask;

    public int Port => ((IPEndPoint)_listener.LocalEndpoint).Port;
    public string Host => "127.0.0.1";
    private bool _isStarted = false;

    public MockAtBridge AtBridge { get; } = new();

    public List<string> ReceivedRequests { get; } = new();
    public List<string> ReceivedShellCommands { get; } = new();
    public Dictionary<string, byte[]> PushedFiles { get; } = new(StringComparer.OrdinalIgnoreCase);

    public Dictionary<string, string> ShellResponses { get; } = new(StringComparer.OrdinalIgnoreCase);
    public Func<string, string?>? CustomShellHandler { get; set; }

    public bool SimulateFailResponse { get; set; }
    public string FailErrorMessage { get; set; } = "device offline";
    public bool SimulatePrematureDisconnect { get; set; }
    public bool SimulateMalformedHeader { get; set; }
    public int Port2600FailAttempts { get; set; } = 0;
    private int _port2600CurrentFailures = 0;

    public MockAdbServer(int port = 0)
    {
        _listener = new TcpListener(IPAddress.Loopback, port);
        SetupDefaultShellResponses();
        Start();
    }

    public void Start()
    {
        if (_isStarted) return;
        _isStarted = true;
        _listener.Start();
        _listenTask = Task.Run(AcceptConnectionsAsync);
    }

    private void SetupDefaultShellResponses()
    {
        ShellResponses["pm list package|grep rsuapp"] = "";
        ShellResponses["ps -A|grep tsdaemon;ps|grep tsdaemon"] = "";
        ShellResponses["getprop"] = GetDefaultGetPropOutput();
        ShellResponses["service call iphonesubinfo 1 s16 com.android.shell"] = GetDefaultImeiParcelOutput();
        ShellResponses["getenforce"] = "Enforcing\n";
        ShellResponses["pm path KEYTool.SRV4"] = "package:/data/app/~~Ol5g_-Ev0NgpenK8egqMbg==/KEYTool.SRV4-0maqZwPFkP4Jxg5fB6y2NA==/base.apk\n";
        ShellResponses["echo 'id' |toybox nc localhost 2600"] = "uid=1000(system) gid=1000(system) groups=1000(system),2002(diag) context=u:r:system_app:s0\n";
        ShellResponses["dumpsys package com.samsung.android.app.repaircal | grep -E \"versionName|versionCode\""] = "";
        ShellResponses["dumpsys package com.samsung.android.app.mobiledoctor | grep -E \"versionName|versionCode\""] = "";
        ShellResponses["dumpsys package com.samsung.rms.retailagent.global | grep -E \"versionName|versionCode\""] = "versionCode=30000171 minSdk=34 targetSdk=35\nversionName=5.59.6-main\n";
        ShellResponses["svc power stayon true"] = "";
        ShellResponses["echo 'setprop ctl.stop ssud'|toybox nc localhost 2600"] = "";
        ShellResponses["chmod 777 /data/local/tmp/AT"] = "";
        ShellResponses["logcat -b radio -c"] = "";
        ShellResponses["pm uninstall -k --user 0 com.sec.android.AutoPreconfig;pm uninstall -k --user 0 com.sec.omadmspr;pm clear com.samsung.android.app.mobiledoctor"] =
            "Failure [not installed for 0].\nFailure [not installed for 0].\nFailed.\n";
    }

    public static string GetDefaultGetPropOutput()
    {
        return
            "[ro.product.model]: [SM-S918U]\r\n" +
            "[ro.boot.serialno]: [RFCX90GR21X]\r\n" +
            "[ro.csc.sales_code]: [ATT]\r\n" +
            "[ril.product_code]: [SM-S918UZKFXAA]\r\n" +
            "[ro.build.display.id]: [S918USQS6DYG1]\r\n" +
            "[ro.build.version.release]: [15]\r\n" +
            "[ro.build.version.security_patch]: [2025-07-01]\r\n" +
            "[ro.boot.debug_level]: [0x4f4c]\r\n" +
            "[sys.usb.config]: [mtp,conn_gadget,adb]\r\n";
    }

    public static string GetDefaultImeiParcelOutput()
    {
        return
            "Result: Parcel(\n" +
            "0x00000000: 00000000 0000000f 00350033 00370035 '........3.5.5.7.'\n" +
            "0x00000010: 00320035 00320039 00350030 00320034 '5.2.9.2.0.5.4.2.'\n" +
            "0x00000020: 00330036 00000033                   '6.3.3...        ')\n";
    }

    private async Task AcceptConnectionsAsync()
    {
        try
        {
            while (!_cts.IsCancellationRequested)
            {
                var client = await _listener.AcceptTcpClientAsync(_cts.Token);
                _ = Task.Run(() => HandleClientAsync(client, _cts.Token));
            }
        }
        catch (OperationCanceledException)
        {
        }
        catch (ObjectDisposedException)
        {
        }
    }

    private async Task HandleClientAsync(TcpClient client, CancellationToken cancellationToken)
    {
        using (client)
        using (var stream = client.GetStream())
        {
            try
            {
                while (!cancellationToken.IsCancellationRequested)
                {
                    // Read 4-hex request length
                    byte[] lenBuf = new byte[4];
                    int readLen = await TryReadExactAsync(stream, lenBuf, 4, cancellationToken);
                    if (readLen < 4) break;

                    if (SimulatePrematureDisconnect)
                    {
                        client.Close();
                        return;
                    }

                    if (SimulateMalformedHeader)
                    {
                        await stream.WriteAsync(Encoding.ASCII.GetBytes("BADD"), cancellationToken);
                        client.Close();
                        return;
                    }

                    string hexLenStr = Encoding.ASCII.GetString(lenBuf);
                    if (!int.TryParse(hexLenStr, System.Globalization.NumberStyles.HexNumber, null, out int reqLen))
                    {
                        // Check if direct usb: was sent without length prefix (some adb clients send '0004usb:' which matches hexLen 0004, but if 'usb:' raw)
                        break;
                    }

                    byte[] reqBuf = new byte[reqLen];
                    int reqRead = await TryReadExactAsync(stream, reqBuf, reqLen, cancellationToken);
                    if (reqRead < reqLen) break;

                    string request = Encoding.UTF8.GetString(reqBuf);
                    ReceivedRequests.Add(request);

                    if (SimulateFailResponse)
                    {
                        byte[] failHdr = Encoding.ASCII.GetBytes("FAIL");
                        byte[] errMsgBytes = Encoding.UTF8.GetBytes(FailErrorMessage);
                        string errLenHex = errMsgBytes.Length.ToString("X4");
                        await stream.WriteAsync(failHdr, cancellationToken);
                        await stream.WriteAsync(Encoding.ASCII.GetBytes(errLenHex), cancellationToken);
                        await stream.WriteAsync(errMsgBytes, cancellationToken);
                        await stream.FlushAsync(cancellationToken);
                        continue;
                    }

                    // 1. host:version
                    if (request == "host:version")
                    {
                        await stream.WriteAsync(Encoding.ASCII.GetBytes("OKAY00040029"), cancellationToken);
                        await stream.FlushAsync(cancellationToken);
                        continue;
                    }

                    // 2. host:devices
                    if (request == "host:devices")
                    {
                        string devList = "RFCX90GR21X\tdevice\n";
                        byte[] devBytes = Encoding.UTF8.GetBytes(devList);
                        string devLen = devBytes.Length.ToString("X4");
                        await stream.WriteAsync(Encoding.ASCII.GetBytes("OKAY" + devLen), cancellationToken);
                        await stream.WriteAsync(devBytes, cancellationToken);
                        await stream.FlushAsync(cancellationToken);
                        continue;
                    }

                    // 3. host:transport / transport-any
                    if (request.StartsWith("host:transport"))
                    {
                        await stream.WriteAsync(Encoding.ASCII.GetBytes("OKAY"), cancellationToken);
                        await stream.FlushAsync(cancellationToken);
                        continue;
                    }

                    // 4. usb: (Direct USB mode reset)
                    if (request == "usb:" || request.EndsWith("usb:"))
                    {
                        await stream.WriteAsync(Encoding.ASCII.GetBytes("OKAYrestarting in USB mode"), cancellationToken);
                        await stream.FlushAsync(cancellationToken);
                        break;
                    }

                    // 5. sync: (File Push Service)
                    if (request == "sync:")
                    {
                        await stream.WriteAsync(Encoding.ASCII.GetBytes("OKAY"), cancellationToken);
                        await stream.FlushAsync(cancellationToken);
                        await HandleSyncServiceAsync(stream, cancellationToken);
                        break;
                    }

                    // 6. shell: / shell,v2,raw:
                    if (request.StartsWith("shell:") || request.StartsWith("shell,v2,raw:"))
                    {
                        bool isV2 = request.StartsWith("shell,v2,raw:");
                        string cmd = isV2 ? request.Substring("shell,v2,raw:".Length) : request.Substring("shell:".Length);
                        ReceivedShellCommands.Add(cmd);

                        await stream.WriteAsync(Encoding.ASCII.GetBytes("OKAY"), cancellationToken);
                        await stream.FlushAsync(cancellationToken);

                        if (cmd == "/data/local/tmp/AT" || cmd.StartsWith("/data/local/tmp/AT ") || cmd == "toybox nc localhost 2600")
                        {
                            // Route interactive stream to AtBridge
                            await AtBridge.RunSessionAsync(stream, cancellationToken);
                            break;
                        }

                        // Determine shell command response
                        string output = GetShellCommandResponse(cmd);

                        if (!isV2)
                        {
                            byte[] outBytes = Encoding.UTF8.GetBytes(output);
                            await stream.WriteAsync(outBytes, cancellationToken);
                            await stream.FlushAsync(cancellationToken);
                        }
                        else
                        {
                            // V2 framing: StreamId 1 (stdout)
                            byte[] outBytes = Encoding.UTF8.GetBytes(output);
                            if (outBytes.Length > 0)
                            {
                                byte[] v2Header = new byte[5];
                                v2Header[0] = 1; // stdout
                                BinaryPrimitives.WriteInt32LittleEndian(v2Header.AsSpan(1, 4), outBytes.Length);
                                await stream.WriteAsync(v2Header, cancellationToken);
                                await stream.WriteAsync(outBytes, cancellationToken);
                            }

                            // V2 exit code packet (StreamId 3)
                            byte[] exitHeader = new byte[5];
                            exitHeader[0] = 3; // exit code
                            BinaryPrimitives.WriteInt32LittleEndian(exitHeader.AsSpan(1, 4), 1);
                            await stream.WriteAsync(exitHeader, cancellationToken);
                            await stream.WriteAsync(new byte[] { 0 }, cancellationToken); // exit code 0
                            await stream.FlushAsync(cancellationToken);
                        }
                        break;
                    }

                    // Unknown request: acknowledge OKAY
                    await stream.WriteAsync(Encoding.ASCII.GetBytes("OKAY"), cancellationToken);
                    await stream.FlushAsync(cancellationToken);
                }
            }
            catch (IOException)
            {
            }
            catch (OperationCanceledException)
            {
            }
        }
    }

    private string GetShellCommandResponse(string cmd)
    {
        if (CustomShellHandler != null)
        {
            string? custom = CustomShellHandler(cmd);
            if (custom != null) return custom;
        }

        if (cmd == "echo 'id' |toybox nc localhost 2600")
        {
            if (ShellResponses.TryGetValue(cmd, out string? explicitVal) && explicitVal.Contains("refused"))
            {
                return explicitVal;
            }

            if (_port2600CurrentFailures < Port2600FailAttempts)
            {
                _port2600CurrentFailures++;
                return "nc: connect: Connection refused.\n";
            }
            return "uid=1000(system) gid=1000(system) groups=1000(system) context=u:r:system_app:s0\n";
        }

        if (ShellResponses.TryGetValue(cmd, out string? val))
        {
            return val;
        }

        if (cmd.StartsWith("service call iphonesubinfo"))
        {
            return GetDefaultImeiParcelOutput();
        }

        return "";
    }

    private async Task HandleSyncServiceAsync(NetworkStream stream, CancellationToken cancellationToken)
    {
        string currentSendPath = "";
        using var currentPayloadStream = new MemoryStream();

        while (!cancellationToken.IsCancellationRequested)
        {
            byte[] header = new byte[8];
            int hRead = await TryReadExactAsync(stream, header, 8, cancellationToken);
            if (hRead < 8) break;

            string id = Encoding.ASCII.GetString(header, 0, 4);
            int lengthOrVal = BinaryPrimitives.ReadInt32LittleEndian(header.AsSpan(4, 4));

            if (id == "STAT")
            {
                byte[] pathBuf = new byte[lengthOrVal];
                await TryReadExactAsync(stream, pathBuf, lengthOrVal, cancellationToken);
                string path = Encoding.UTF8.GetString(pathBuf);

                byte[] statResp = new byte[16];
                Encoding.ASCII.GetBytes("STAT", 0, 4, statResp, 0);
                BinaryPrimitives.WriteInt32LittleEndian(statResp.AsSpan(4, 4), 33188); // mode

                int fileSize = PushedFiles.TryGetValue(path, out var data) ? data.Length : 0;
                BinaryPrimitives.WriteInt32LittleEndian(statResp.AsSpan(8, 4), fileSize); // size
                BinaryPrimitives.WriteInt32LittleEndian(statResp.AsSpan(12, 4), (int)DateTimeOffset.UtcNow.ToUnixTimeSeconds()); // mtime

                await stream.WriteAsync(statResp, cancellationToken);
                await stream.FlushAsync(cancellationToken);
            }
            else if (id == "SEND")
            {
                byte[] sendTargetBuf = new byte[lengthOrVal];
                await TryReadExactAsync(stream, sendTargetBuf, lengthOrVal, cancellationToken);
                string sendTarget = Encoding.UTF8.GetString(sendTargetBuf);
                int commaIdx = sendTarget.IndexOf(',');
                currentSendPath = commaIdx > 0 ? sendTarget.Substring(0, commaIdx) : sendTarget;
                currentPayloadStream.SetLength(0);
            }
            else if (id == "DATA")
            {
                byte[] chunk = new byte[lengthOrVal];
                await TryReadExactAsync(stream, chunk, lengthOrVal, cancellationToken);
                currentPayloadStream.Write(chunk, 0, lengthOrVal);
            }
            else if (id == "DONE")
            {
                // Save completed file
                PushedFiles[currentSendPath] = currentPayloadStream.ToArray();

                // Respond OKAY + 4 zero bytes
                byte[] doneResp = new byte[8];
                Encoding.ASCII.GetBytes("OKAY", 0, 4, doneResp, 0);
                await stream.WriteAsync(doneResp, cancellationToken);
                await stream.FlushAsync(cancellationToken);
            }
            else if (id == "QUIT")
            {
                break;
            }
        }
    }

    private static async Task<int> TryReadExactAsync(Stream stream, byte[] buffer, int count, CancellationToken cancellationToken)
    {
        int totalRead = 0;
        while (totalRead < count)
        {
            int read = await stream.ReadAsync(buffer.AsMemory(totalRead, count - totalRead), cancellationToken);
            if (read == 0) break;
            totalRead += read;
        }
        return totalRead;
    }

    public async ValueTask DisposeAsync()
    {
        _cts.Cancel();
        _listener.Stop();
        if (_listenTask != null)
        {
            try { await _listenTask; } catch { }
        }
        _cts.Dispose();
    }
}
