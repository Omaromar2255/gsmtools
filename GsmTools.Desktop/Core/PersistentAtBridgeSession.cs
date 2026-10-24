using System.Net.Sockets;
using System.Security.Cryptography;
using System.Text;
using System.Text.RegularExpressions;

namespace GsmTools.Desktop.Core;

public record AtBridgeBanner(
    string Model,
    string Csc,
    string AndroidVersion,
    string SecurityPatch,
    string DebugLevel,
    int Uid,
    string ActiveChannel
);

public interface IPersistentAtBridgeSession : IAsyncDisposable
{
    Task<AtBridgeBanner> StartSessionAsync(string? serial = null, uint sessionSeed = 0, CancellationToken cancellationToken = default);
    Task<AtBridgeBanner> StartSessionAsync(string? serial, uint sessionSeed, string customCommand, CancellationToken cancellationToken = default);
    Task<string> SendIpcHexCommandAsync(string hexCommand, CancellationToken cancellationToken = default);
    Task<string> SendAtCommandAsync(string atCommand, CancellationToken cancellationToken = default);
    Task<string> QuerySimLocksAsync(CancellationToken cancellationToken = default);
    Task TerminateSessionAsync(CancellationToken cancellationToken = default);
    AtBridgeBanner? Banner { get; }
    uint SessionSeed { get; }
    bool IsActive { get; }
}

/// <summary>
/// Manages a continuous interactive session with the /data/local/tmp/AT bridge.
/// Supports Marsaglia Xorshift32 session encryption and smart UID 1000 relay fallback via port 2600.
/// </summary>
public class PersistentAtBridgeSession : IPersistentAtBridgeSession
{
    private readonly IAdbRawSocketClient _adbClient;
    private TcpClient? _shellClient;
    private NetworkStream? _stream;
    private readonly StringBuilder _readBuffer = new();
    private readonly SemaphoreSlim _sessionLock = new(1, 1);

    public AtBridgeBanner? Banner { get; private set; }
    public uint SessionSeed { get; private set; }
    public bool IsActive => _shellClient != null && _shellClient.Connected;

    public PersistentAtBridgeSession(IAdbRawSocketClient adbClient)
    {
        _adbClient = adbClient ?? throw new ArgumentNullException(nameof(adbClient));
    }

    public Task<AtBridgeBanner> StartSessionAsync(string? serial = null, uint sessionSeed = 0, CancellationToken cancellationToken = default)
    {
        return StartSessionAsync(serial, sessionSeed, "/data/local/tmp/AT", cancellationToken);
    }

    public async Task<AtBridgeBanner> StartSessionAsync(string? serial, uint sessionSeed, string customCommand, CancellationToken cancellationToken = default)
    {
        await _sessionLock.WaitAsync(cancellationToken);
        try
        {
            if (IsActive)
                await TerminateSessionInternalAsync(cancellationToken);

            // Step 1: Open direct interactive shell running customCommand
            _shellClient = await _adbClient.OpenInteractiveShellAsync(customCommand, serial, cancellationToken);
            _stream = _shellClient.GetStream();
            _readBuffer.Clear();

            // Step 2: Read startup output until banner and prompt "<< "
            string bannerOutput = await ReadUntilPromptAsync("<< ", TimeSpan.FromSeconds(10), cancellationToken);
            var parsedBanner = ParseBanner(bannerOutput);

            // Step 3: Smart UID 1000 Relay Fallback
            // If shell is UID 2000 and has no channel access (CH=NONE), attempt relay via port 2600
            if (parsedBanner.Uid != 1000 && parsedBanner.ActiveChannel == "NONE")
            {
                // Test if port 2600 is responding
                string idCheck = await _adbClient.ExecuteShellCommandAsync("echo 'id' |toybox nc localhost 2600", false, cancellationToken);
                if (idCheck.Contains("uid=1000"))
                {
                    // Close unprivileged session and launch relay
                    await TerminateSessionInternalAsync(cancellationToken);

                    _shellClient = await _adbClient.OpenInteractiveShellAsync("toybox nc localhost 2600", serial, cancellationToken);
                    _stream = _shellClient.GetStream();
                    _readBuffer.Clear();

                    // Launch AT binary inside UID 1000 context
                    byte[] startCmd = Encoding.ASCII.GetBytes($"{customCommand}\n");
                    await _stream.WriteAsync(startCmd, cancellationToken);
                    await _stream.FlushAsync(cancellationToken);

                    string relayBannerOutput = await ReadUntilPromptAsync("<< ", TimeSpan.FromSeconds(10), cancellationToken);
                    parsedBanner = ParseBanner(relayBannerOutput);
                }
            }

            Banner = parsedBanner;

            // Step 4: Session Encryption Handshake
            if (sessionSeed > 0)
            {
                string authCmd = $"AUTH:{sessionSeed:X8}\n";
                byte[] authBytes = Encoding.ASCII.GetBytes(authCmd);
                await _stream.WriteAsync(authBytes, cancellationToken);
                await _stream.FlushAsync(cancellationToken);

                string authResp = await ReadUntilPromptAsync("<< ", TimeSpan.FromSeconds(5), cancellationToken);
                if (!authResp.Contains(".OK_AUTH.."))
                {
                    throw new InvalidOperationException($"AT bridge authentication failed. Response: {authResp}");
                }
                SessionSeed = sessionSeed;
            }
            else
            {
                SessionSeed = 0; // Raw Hex Mode matching csc 15.txt
            }

            return Banner;
        }
        finally
        {
            _sessionLock.Release();
        }
    }

    /// <summary>
    /// Transmits an IPC hex string to the persistent AT bridge and waits for .OK..# delimiter.
    /// Handles Xorshift32 obfuscation and deobfuscation when encrypted session is active.
    /// </summary>
    public async Task<string> SendIpcHexCommandAsync(string hexCommand, CancellationToken cancellationToken = default)
    {
        if (_stream == null || !IsActive)
            throw new InvalidOperationException("AT bridge session is not active.");

        await _sessionLock.WaitAsync(cancellationToken);
        try
        {
            string payloadToSend = hexCommand.Trim();
            if (!Regex.IsMatch(payloadToSend, "^[0-9A-Fa-f]+$"))
                throw new FormatException($"Invalid hex syntax for IPC command: '{hexCommand}'");

            // Obfuscate outgoing payload if encrypted session is enabled
            if (SessionSeed > 0)
            {
                byte[] binPayload = Convert.FromHexString(payloadToSend);
                Xorshift32StreamCipher.TransformBuffer(binPayload, SessionSeed);
                payloadToSend = Convert.ToHexString(binPayload);
            }

            byte[] cmdBytes = Encoding.ASCII.GetBytes($"{payloadToSend}\n");
            await _stream.WriteAsync(cmdBytes, cancellationToken);
            await _stream.FlushAsync(cancellationToken);

            // Read response until "<< " prompt
            string resp = await ReadUntilPromptAsync("<< ", TimeSpan.FromSeconds(15), cancellationToken);

            if (resp.Contains(".ERR_IPC.."))
                throw new InvalidOperationException($"AT bridge returned .ERR_IPC.. for command: {hexCommand}");
            if (resp.Contains(".ERR_SYNTAX.."))
                throw new FormatException($"AT bridge returned .ERR_SYNTAX.. for command: {hexCommand}");

            // Look for .OK..<HEX># or .OK..#
            var match = Regex.Match(resp, @"\.OK\.\.(?<hex>[0-9A-Fa-f]*)#");
            if (match.Success)
            {
                string respHex = match.Groups["hex"].Value;
                if (!string.IsNullOrEmpty(respHex) && SessionSeed > 0)
                {
                    byte[] binResp = Convert.FromHexString(respHex);
                    Xorshift32StreamCipher.TransformBuffer(binResp, SessionSeed);
                    return Convert.ToHexString(binResp);
                }
                return respHex;
            }

            // Return full response string if matching standard OK
            if (resp.Contains(".OK.."))
                return "OK";

            return resp.Trim();
        }
        finally
        {
            _sessionLock.Release();
        }
    }

    /// <summary>
    /// Transmits a raw text AT command (e.g. AT+CPIN? or AT+CLCK="PN",2) and returns response.
    /// </summary>
    public async Task<string> SendAtCommandAsync(string atCommand, CancellationToken cancellationToken = default)
    {
        if (_stream == null || !IsActive)
            throw new InvalidOperationException("AT bridge session is not active.");

        await _sessionLock.WaitAsync(cancellationToken);
        try
        {
            string cmd = atCommand.StartsWith("AT:", StringComparison.OrdinalIgnoreCase)
                ? atCommand.Trim()
                : $"AT:{atCommand.Trim()}";

            byte[] cmdBytes = Encoding.ASCII.GetBytes($"{cmd}\n");
            await _stream.WriteAsync(cmdBytes, cancellationToken);
            await _stream.FlushAsync(cancellationToken);

            string resp = await ReadUntilPromptAsync("<< ", TimeSpan.FromSeconds(10), cancellationToken);
            return resp.Trim();
        }
        finally
        {
            _sessionLock.Release();
        }
    }

    /// <summary>
    /// Queries SIM Card State and Network Personalization locks via LOCKS command.
    /// </summary>
    public async Task<string> QuerySimLocksAsync(CancellationToken cancellationToken = default)
    {
        if (_stream == null || !IsActive)
            throw new InvalidOperationException("AT bridge session is not active.");

        await _sessionLock.WaitAsync(cancellationToken);
        try
        {
            byte[] cmdBytes = Encoding.ASCII.GetBytes("LOCKS\n");
            await _stream.WriteAsync(cmdBytes, cancellationToken);
            await _stream.FlushAsync(cancellationToken);

            string resp = await ReadUntilPromptAsync("<< ", TimeSpan.FromSeconds(10), cancellationToken);
            return resp.Trim();
        }
        finally
        {
            _sessionLock.Release();
        }
    }

    public async Task TerminateSessionAsync(CancellationToken cancellationToken = default)
    {
        await _sessionLock.WaitAsync(cancellationToken);
        try
        {
            await TerminateSessionInternalAsync(cancellationToken);
        }
        finally
        {
            _sessionLock.Release();
        }
    }

    private async Task TerminateSessionInternalAsync(CancellationToken cancellationToken)
    {
        if (_stream != null)
        {
            try
            {
                byte[] exitBytes = Encoding.ASCII.GetBytes("EXIT\n");
                await _stream.WriteAsync(exitBytes, cancellationToken);
                await _stream.FlushAsync(cancellationToken);
                await Task.Delay(50, cancellationToken);
            }
            catch
            {
                // Ignore teardown write errors
            }

            await _stream.DisposeAsync();
            _stream = null;
        }

        if (_shellClient != null)
        {
            _shellClient.Dispose();
            _shellClient = null;
        }

        Banner = null;
        SessionSeed = 0;
        _readBuffer.Clear();
    }

    private async Task<string> ReadUntilPromptAsync(string prompt, TimeSpan timeout, CancellationToken cancellationToken)
    {
        if (_stream == null)
            throw new InvalidOperationException("Stream is null.");

        using var cts = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
        cts.CancelAfter(timeout);

        byte[] buf = new byte[2048];

        while (!cts.IsCancellationRequested)
        {
            string current = _readBuffer.ToString();
            int idx = current.IndexOf(prompt, StringComparison.Ordinal);
            if (idx >= 0)
            {
                string result = current.Substring(0, idx + prompt.Length);
                _readBuffer.Remove(0, idx + prompt.Length);
                return result;
            }

            int read = await _stream.ReadAsync(buf, 0, buf.Length, cts.Token);
            if (read == 0)
                break; // Stream closed

            _readBuffer.Append(Encoding.UTF8.GetString(buf, 0, read));
        }

        return _readBuffer.ToString();
    }

    private static AtBridgeBanner ParseBanner(string output)
    {
        // INFO|MODEL=SM-S918U|CSC=ATT|AND=15|PATCH=2025-07-01|DBG=0|UID=1000|CH=/dev/umts_ipc0
        string model = "N/A", csc = "N/A", andVer = "N/A", patch = "N/A", dbg = "0", ch = "NONE";
        int uid = -1;

        var lineMatch = Regex.Match(output, @"INFO\|(?<fields>[^\r\n]+)");
        if (lineMatch.Success)
        {
            string fields = lineMatch.Groups["fields"].Value;
            var tokens = fields.Split('|');
            foreach (var token in tokens)
            {
                int eq = token.IndexOf('=');
                if (eq <= 0) continue;
                string k = token.Substring(0, eq).Trim();
                string v = token.Substring(eq + 1).Trim();

                switch (k.ToUpperInvariant())
                {
                    case "MODEL": model = v; break;
                    case "CSC": csc = v; break;
                    case "AND": andVer = v; break;
                    case "PATCH": patch = v; break;
                    case "DBG": dbg = v; break;
                    case "UID": int.TryParse(v, out uid); break;
                    case "CH": ch = v; break;
                }
            }
        }

        return new AtBridgeBanner(model, csc, andVer, patch, dbg, uid, ch);
    }

    public async ValueTask DisposeAsync()
    {
        await TerminateSessionAsync();
        _sessionLock.Dispose();
        GC.SuppressFinalize(this);
    }
}
