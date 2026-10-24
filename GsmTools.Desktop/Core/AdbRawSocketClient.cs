using System.Buffers.Binary;
using System.IO;
using System.Net.Sockets;
using System.Text;

namespace GsmTools.Desktop.Core;

public class AdbException : Exception
{
    public AdbException(string message) : base(message) { }
    public AdbException(string message, Exception innerException) : base(message, innerException) { }
}

public interface IAdbRawSocketClient : IAsyncDisposable
{
    Task ConnectAsync(string host = "127.0.0.1", int port = 5037, CancellationToken cancellationToken = default);
    Task<string> SendAdbRequestAsync(string request, CancellationToken cancellationToken = default);
    Task SelectDeviceAsync(string? serial = null, CancellationToken cancellationToken = default);
    Task<string> ExecuteShellCommandAsync(string command, bool useV2 = false, CancellationToken cancellationToken = default);
    Task PushFileSyncAsync(string localPath, string remotePath, int permissions = 33188, Action<long, long>? progressCallback = null, CancellationToken cancellationToken = default);
    Task<string> RestartUsbModeAsync(CancellationToken cancellationToken = default);
    Task<TcpClient> OpenInteractiveShellAsync(string command, string? serial = null, CancellationToken cancellationToken = default);
    Task<Stream> OpenRawStreamAsync(string service, CancellationToken cancellationToken = default);
    bool IsConnected { get; }
    string? CurrentSerial { get; set; }
}

/// <summary>
/// Direct TCP Socket ADB client communicating over 127.0.0.1:5037 according to the native ADB protocol.
/// Spawns zero external adb.exe processes. Matches csc 15.txt packet traces.
/// </summary>
public class AdbRawSocketClient : IAdbRawSocketClient
{
    private const string DefaultHost = "127.0.0.1";
    private const int DefaultPort = 5037;
    private const int MaxChunkSize = 64 * 1024; // 64KB chunks for sync protocol

    private string _host = DefaultHost;
    private int _port = DefaultPort;
    private TcpClient? _tcpClient;
    private NetworkStream? _stream;
    private readonly SemaphoreSlim _semaphore = new(1, 1);

    public bool IsConnected => _tcpClient != null && _tcpClient.Connected;
    public string? CurrentSerial { get; set; }

    public async Task ConnectAsync(string host = DefaultHost, int port = DefaultPort, CancellationToken cancellationToken = default)
    {
        await _semaphore.WaitAsync(cancellationToken);
        try
        {
            if (_tcpClient != null && (_host != host || _port != port))
            {
                if (_stream != null)
                {
                    await _stream.DisposeAsync();
                    _stream = null;
                }
                _tcpClient.Dispose();
                _tcpClient = null;
            }

            _host = host;
            _port = port;
            if (IsConnected) return;

            _tcpClient = new TcpClient();
            await _tcpClient.ConnectAsync(_host, _port, cancellationToken);
            _stream = _tcpClient.GetStream();
        }
        finally
        {
            _semaphore.Release();
        }
    }

    /// <summary>
    /// Sends length-prefixed ADB request ("0012host:transport-any") and checks OKAY/FAIL.
    /// </summary>
    public async Task<string> SendAdbRequestAsync(string request, CancellationToken cancellationToken = default)
    {
        if (_stream == null)
            throw new InvalidOperationException("Not connected to ADB server.");

        byte[] reqBytes = Encoding.UTF8.GetBytes(request);
        string lenHex = reqBytes.Length.ToString("X4");
        byte[] lenBytes = Encoding.ASCII.GetBytes(lenHex);

        await _stream.WriteAsync(lenBytes, cancellationToken);
        await _stream.WriteAsync(reqBytes, cancellationToken);
        await _stream.FlushAsync(cancellationToken);

        byte[] respHeader = new byte[4];
        await ReadExactAsync(_stream, respHeader, 4, cancellationToken);
        string status = Encoding.ASCII.GetString(respHeader);

        if (status == "OKAY")
        {
            return "OKAY";
        }
        else if (status == "FAIL")
        {
            byte[] errLenBuf = new byte[4];
            await ReadExactAsync(_stream, errLenBuf, 4, cancellationToken);
            int errLen = int.Parse(Encoding.ASCII.GetString(errLenBuf), System.Globalization.NumberStyles.HexNumber);
            byte[] errBuf = new byte[errLen];
            await ReadExactAsync(_stream, errBuf, errLen, cancellationToken);
            string errMsg = Encoding.UTF8.GetString(errBuf);
            throw new AdbException($"ADB request failed ({request}): {errMsg}");
        }

        throw new AdbException($"Unexpected ADB response header: '{status}' for request: {request}");
    }

    public async Task SelectDeviceAsync(string? serial = null, CancellationToken cancellationToken = default)
    {
        string req = string.IsNullOrWhiteSpace(serial) ? "host:transport-any" : $"host:transport:{serial}";
        await SendAdbRequestAsync(req, cancellationToken);
        CurrentSerial = serial;
    }

    /// <summary>
    /// Executes a shell command on the selected device via a dedicated connection and returns full output.
    /// Supports standard 'shell:' and 'shell,v2,raw:'.
    /// </summary>
    public async Task<string> ExecuteShellCommandAsync(string command, bool useV2 = false, CancellationToken cancellationToken = default)
    {
        using var client = new TcpClient();
        await client.ConnectAsync(_host, _port, cancellationToken);
        using var stream = client.GetStream();

        // 1. Select device
        string transReq = string.IsNullOrWhiteSpace(CurrentSerial) ? "host:transport-any" : $"host:transport:{CurrentSerial}";
        await SendFramedRequestOnStreamAsync(stream, transReq, cancellationToken);

        // 2. Request shell service
        string shellService = useV2 ? $"shell,v2,raw:{command}" : $"shell:{command}";
        await SendFramedRequestOnStreamAsync(stream, shellService, cancellationToken);

        if (!useV2)
        {
            // Standard shell stream: read raw bytes until stream closes
            using var ms = new MemoryStream();
            byte[] buf = new byte[4096];
            int read;
            while ((read = await stream.ReadAsync(buf, cancellationToken)) > 0)
            {
                ms.Write(buf, 0, read);
            }
            return Encoding.UTF8.GetString(ms.ToArray());
        }
        else
        {
            // Shell v2 framing: [1B StreamID][4B Little-Endian Length][Data...]
            using var msOut = new MemoryStream();
            byte[] header = new byte[5];

            while (true)
            {
                int hRead = await TryReadExactAsync(stream, header, 5, cancellationToken);
                if (hRead < 5) break;

                byte streamId = header[0];
                int payloadLen = BinaryPrimitives.ReadInt32LittleEndian(header.AsSpan(1, 4));

                if (payloadLen > 0)
                {
                    byte[] payload = new byte[payloadLen];
                    await ReadExactAsync(stream, payload, payloadLen, cancellationToken);

                    // streamId 1 = stdout, 2 = stderr
                    if (streamId is 1 or 2)
                    {
                        msOut.Write(payload, 0, payloadLen);
                    }
                }

                // streamId 3 = exit status
                if (streamId == 3)
                {
                    break;
                }
            }

            return Encoding.UTF8.GetString(msOut.ToArray());
        }
    }

    /// <summary>
    /// Implements the native ADB sync: push protocol matching csc 15.txt (STAT -> SEND -> DATA -> DONE -> QUIT).
    /// </summary>
    public async Task PushFileSyncAsync(string localPath, string remotePath, int permissions = 33188, Action<long, long>? progressCallback = null, CancellationToken cancellationToken = default)
    {
        if (!File.Exists(localPath))
            throw new FileNotFoundException($"Local file to push does not exist: {localPath}", localPath);

        using var client = new TcpClient();
        await client.ConnectAsync(_host, _port, cancellationToken);
        using var stream = client.GetStream();

        // 1. Select device
        string transReq = string.IsNullOrWhiteSpace(CurrentSerial) ? "host:transport-any" : $"host:transport:{CurrentSerial}";
        await SendFramedRequestOnStreamAsync(stream, transReq, cancellationToken);

        // 2. Open sync: service
        await SendFramedRequestOnStreamAsync(stream, "sync:", cancellationToken);

        // 3. STAT check: "STAT" + [4B LE pathLen] + [path]
        byte[] remotePathBytes = Encoding.UTF8.GetBytes(remotePath);
        await WriteSyncHeaderAsync(stream, "STAT", remotePathBytes.Length, cancellationToken);
        await stream.WriteAsync(remotePathBytes, cancellationToken);
        await stream.FlushAsync(cancellationToken);

        // Read 16-byte STAT response: [4B "STAT"][4B mode][4B size][4B mtime]
        byte[] statResp = new byte[16];
        await ReadExactAsync(stream, statResp, 16, cancellationToken);
        string statTag = Encoding.ASCII.GetString(statResp, 0, 4);
        if (statTag != "STAT")
            throw new AdbException($"Expected STAT response from sync service, received: '{statTag}'");

        // 4. SEND request: "SEND" + [4B LE len(remotePath,permissions)] + [remotePath,permissions]
        string sendTarget = $"{remotePath},{permissions}";
        byte[] sendTargetBytes = Encoding.UTF8.GetBytes(sendTarget);
        await WriteSyncHeaderAsync(stream, "SEND", sendTargetBytes.Length, cancellationToken);
        await stream.WriteAsync(sendTargetBytes, cancellationToken);
        await stream.FlushAsync(cancellationToken);

        // 5. DATA packets in chunks up to 64KB
        var fileInfo = new FileInfo(localPath);
        long totalBytes = fileInfo.Length;
        long bytesSent = 0;

        using (var fs = File.OpenRead(localPath))
        {
            byte[] chunkBuf = new byte[MaxChunkSize];
            int bytesRead;
            while ((bytesRead = await fs.ReadAsync(chunkBuf, 0, chunkBuf.Length, cancellationToken)) > 0)
            {
                await WriteSyncHeaderAsync(stream, "DATA", bytesRead, cancellationToken);
                await stream.WriteAsync(chunkBuf.AsMemory(0, bytesRead), cancellationToken);
                bytesSent += bytesRead;
                progressCallback?.Invoke(bytesSent, totalBytes);
            }
        }
        await stream.FlushAsync(cancellationToken);

        // 6. DONE packet: "DONE" + [4B LE mtime]
        int mtime = (int)DateTimeOffset.UtcNow.ToUnixTimeSeconds();
        await WriteSyncHeaderAsync(stream, "DONE", mtime, cancellationToken);
        await stream.FlushAsync(cancellationToken);

        // Read DONE response: [4B "OKAY" or "FAIL"] + [4B LE msgLen]
        byte[] doneResp = new byte[8];
        await ReadExactAsync(stream, doneResp, 8, cancellationToken);
        string doneTag = Encoding.ASCII.GetString(doneResp, 0, 4);
        if (doneTag == "FAIL")
        {
            int msgLen = BinaryPrimitives.ReadInt32LittleEndian(doneResp.AsSpan(4, 4));
            byte[] msgBuf = new byte[msgLen];
            await ReadExactAsync(stream, msgBuf, msgLen, cancellationToken);
            string failMsg = Encoding.UTF8.GetString(msgBuf);
            throw new AdbException($"Sync push failed for {remotePath}: {failMsg}");
        }
        else if (doneTag != "OKAY")
        {
            throw new AdbException($"Unexpected sync DONE response: '{doneTag}'");
        }

        // 7. QUIT packet: "QUIT" + [4B 0]
        await WriteSyncHeaderAsync(stream, "QUIT", 0, cancellationToken);
        await stream.FlushAsync(cancellationToken);
    }

    /// <summary>
    /// Sends direct protocol request '0004usb:' and reads 'restarting in USB mode' matching csc 15.txt.
    /// </summary>
    public async Task<string> RestartUsbModeAsync(CancellationToken cancellationToken = default)
    {
        using var client = new TcpClient();
        await client.ConnectAsync(_host, _port, cancellationToken);
        using var stream = client.GetStream();

        // 1. Select device
        string transReq = string.IsNullOrWhiteSpace(CurrentSerial) ? "host:transport-any" : $"host:transport:{CurrentSerial}";
        await SendFramedRequestOnStreamAsync(stream, transReq, cancellationToken);

        // 2. Send 0004usb:
        byte[] usbReq = Encoding.ASCII.GetBytes("0004usb:");
        await stream.WriteAsync(usbReq, cancellationToken);
        await stream.FlushAsync(cancellationToken);

        // 3. Read OKAY/FAIL
        byte[] respHeader = new byte[4];
        await ReadExactAsync(stream, respHeader, 4, cancellationToken);
        string status = Encoding.ASCII.GetString(respHeader);

        if (status == "OKAY")
        {
            // Read output stream (e.g. "restarting in USB mode")
            using var ms = new MemoryStream();
            byte[] buf = new byte[512];
            int read;
            try
            {
                while ((read = await stream.ReadAsync(buf, cancellationToken)) > 0)
                {
                    ms.Write(buf, 0, read);
                }
            }
            catch (IOException)
            {
                // Disconnection expected as USB mode restarts
            }
            string resp = Encoding.UTF8.GetString(ms.ToArray()).Trim();
            return string.IsNullOrEmpty(resp) ? "restarting in USB mode" : resp;
        }

        throw new AdbException($"RestartUsbMode failed with status: {status}");
    }

    /// <summary>
    /// Opens an interactive shell session, returning the active TcpClient for bidirectional streaming.
    /// Caller is responsible for closing/disposing the returned client.
    /// </summary>
    public async Task<TcpClient> OpenInteractiveShellAsync(string command, string? serial = null, CancellationToken cancellationToken = default)
    {
        var client = new TcpClient();
        await client.ConnectAsync(_host, _port, cancellationToken);
        var stream = client.GetStream();

        string? targetSerial = serial ?? CurrentSerial;
        string transReq = string.IsNullOrWhiteSpace(targetSerial) ? "host:transport-any" : $"host:transport:{targetSerial}";
        await SendFramedRequestOnStreamAsync(stream, transReq, cancellationToken);

        string shellReq = $"shell:{command}";
        await SendFramedRequestOnStreamAsync(stream, shellReq, cancellationToken);

        return client;
    }

    /// <summary>
    /// Opens a raw bidirectional stream to an ADB service (e.g. "shell:..."), returning the Stream.
    /// Uses the stored _host and _port. Caller is responsible for closing/disposing the returned stream.
    /// </summary>
    public async Task<Stream> OpenRawStreamAsync(string service, CancellationToken cancellationToken = default)
    {
        var client = new TcpClient();
        await client.ConnectAsync(_host, _port, cancellationToken);
        var stream = client.GetStream();

        string transReq = string.IsNullOrWhiteSpace(CurrentSerial) ? "host:transport-any" : $"host:transport:{CurrentSerial}";
        await SendFramedRequestOnStreamAsync(stream, transReq, cancellationToken);

        await SendFramedRequestOnStreamAsync(stream, service, cancellationToken);

        return stream;
    }

    private static async Task SendFramedRequestOnStreamAsync(NetworkStream stream, string request, CancellationToken cancellationToken)
    {
        byte[] reqBytes = Encoding.UTF8.GetBytes(request);
        string lenHex = reqBytes.Length.ToString("X4");
        byte[] lenBytes = Encoding.ASCII.GetBytes(lenHex);

        await stream.WriteAsync(lenBytes, cancellationToken);
        await stream.WriteAsync(reqBytes, cancellationToken);
        await stream.FlushAsync(cancellationToken);

        byte[] respHeader = new byte[4];
        await ReadExactAsync(stream, respHeader, 4, cancellationToken);
        string status = Encoding.ASCII.GetString(respHeader);

        if (status != "OKAY")
        {
            if (status == "FAIL")
            {
                byte[] errLenBuf = new byte[4];
                await ReadExactAsync(stream, errLenBuf, 4, cancellationToken);
                int errLen = int.Parse(Encoding.ASCII.GetString(errLenBuf), System.Globalization.NumberStyles.HexNumber);
                byte[] errBuf = new byte[errLen];
                await ReadExactAsync(stream, errBuf, errLen, cancellationToken);
                string errMsg = Encoding.UTF8.GetString(errBuf);
                throw new AdbException($"ADB stream request failed ({request}): {errMsg}");
            }
            throw new AdbException($"Unexpected ADB response header: '{status}' for stream request: {request}");
        }
    }

    private static async Task WriteSyncHeaderAsync(NetworkStream stream, string fourCharId, int lengthOrValue, CancellationToken cancellationToken)
    {
        byte[] header = new byte[8];
        Encoding.ASCII.GetBytes(fourCharId, 0, 4, header, 0);
        BinaryPrimitives.WriteInt32LittleEndian(header.AsSpan(4, 4), lengthOrValue);
        await stream.WriteAsync(header, cancellationToken);
    }

    private static async Task ReadExactAsync(Stream stream, byte[] buffer, int count, CancellationToken cancellationToken)
    {
        int totalRead = 0;
        while (totalRead < count)
        {
            int read = await stream.ReadAsync(buffer.AsMemory(totalRead, count - totalRead), cancellationToken);
            if (read == 0)
                throw new EndOfStreamException($"Stream closed prematurely while reading {count} bytes (read {totalRead} bytes).");
            totalRead += read;
        }
    }

    private static async Task<int> TryReadExactAsync(Stream stream, byte[] buffer, int count, CancellationToken cancellationToken)
    {
        int totalRead = 0;
        while (totalRead < count)
        {
            int read = await stream.ReadAsync(buffer.AsMemory(totalRead, count - totalRead), cancellationToken);
            if (read == 0)
                break;
            totalRead += read;
        }
        return totalRead;
    }

    public async ValueTask DisposeAsync()
    {
        if (_stream != null)
        {
            await _stream.DisposeAsync();
            _stream = null;
        }
        if (_tcpClient != null)
        {
            _tcpClient.Dispose();
            _tcpClient = null;
        }
        _semaphore.Dispose();
        GC.SuppressFinalize(this);
    }
}
