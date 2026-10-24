using GsmTools.Desktop.Core;

namespace GsmTools.Desktop.Workflows;

public interface IInteractiveAtWorkflow
{
    event Action<string>? OnLog;
    Task<string> ExecuteRawCommandAsync(string command, bool useDirectModem = false, Action<string>? logCallback = null, CancellationToken cancellationToken = default);
    Task<List<(string Command, string Response, bool Success)>> ExecuteBatchCommandsAsync(string batchCommands, bool useDirectModem = false, Action<string>? logCallback = null, CancellationToken cancellationToken = default);
}

public class InteractiveAtWorkflow : IInteractiveAtWorkflow
{
    private readonly IAdbRawSocketClient _adbClient;
    private readonly IPersistentAtBridgeSession _atSession;
    private readonly IModemSerialClient? _serialClient;

    public event Action<string>? OnLog;

    public InteractiveAtWorkflow(IAdbRawSocketClient adbClient, IPersistentAtBridgeSession atSession, IModemSerialClient? serialClient = null)
    {
        _adbClient = adbClient ?? throw new ArgumentNullException(nameof(adbClient));
        _atSession = atSession ?? throw new ArgumentNullException(nameof(atSession));
        _serialClient = serialClient;
    }

    public async Task<string> ExecuteRawCommandAsync(string command, bool useDirectModem = false, Action<string>? logCallback = null, CancellationToken cancellationToken = default)
    {
        void Log(string message)
        {
            string formatted = $"[{DateTime.Now:HH:mm:ss.fff}] {message}";
            logCallback?.Invoke(formatted);
            OnLog?.Invoke(formatted);
        }

        string cmd = (command ?? string.Empty).Trim();
        if (string.IsNullOrWhiteSpace(cmd))
            return "ERROR: Empty command.";

        Log($"[CMD >>] Executing: {cmd} (Mode: {(useDirectModem ? "Direct Modem Port" : "ADB / AT Bridge")})");

        try
        {
            // Mode 1: Direct Serial Modem COM Port
            if (useDirectModem && _serialClient != null)
            {
                if (!_serialClient.IsConnected)
                {
                    Log("[FAIL] Error: Modem COM port is not connected. Please select and connect port first.");
                    return "ERROR: Modem COM port is not connected.";
                }

                var (success, response) = await _serialClient.SendAtCommandAsync(cmd, 3500, cancellationToken);
                string statusTag = success ? "[SUCCESS]" : "[FAILED]";
                Log($"[RESP <<] {response} -> {statusTag}");
                return response;
            }

            // Mode 2: Persistent AT Bridge Session
            if (_atSession.IsActive)
            {
                if (cmd.StartsWith("AT", StringComparison.OrdinalIgnoreCase) ||
                    cmd.StartsWith("+") || cmd.StartsWith("$") ||
                    cmd.StartsWith("GETSPR", StringComparison.OrdinalIgnoreCase) ||
                    cmd.StartsWith("SETSPR", StringComparison.OrdinalIgnoreCase) ||
                    cmd.StartsWith("SRELOCK1", StringComparison.OrdinalIgnoreCase))
                {
                    string resp = await _atSession.SendAtCommandAsync(cmd, cancellationToken);
                    bool ok = resp.Contains("OK", StringComparison.OrdinalIgnoreCase);
                    Log($"[RESP <<] {resp} -> [{(ok ? "SUCCESS" : "FAILED")}]");
                    return resp;
                }
                else if (System.Text.RegularExpressions.Regex.IsMatch(cmd, "^[0-9A-Fa-f]+$"))
                {
                    string resp = await _atSession.SendIpcHexCommandAsync(cmd, cancellationToken);
                    Log($"[RESP <<] Hex IPC Response: {resp} -> [SUCCESS]");
                    return resp;
                }
                else
                {
                    string resp = await _atSession.SendAtCommandAsync(cmd, cancellationToken);
                    Log($"[RESP <<] Bridge Response: {resp} -> [SUCCESS]");
                    return resp;
                }
            }

            // Mode 3: Direct ADB shell device node injection
            if (!_adbClient.IsConnected)
            {
                Log("[CMD >>] Connecting to ADB daemon at 127.0.0.1:5037...");
                await _adbClient.ConnectAsync(cancellationToken: cancellationToken);
                await _adbClient.SelectDeviceAsync(cancellationToken: cancellationToken);
            }

            string script = $"if [ -w /dev/umts_atc0 ]; then echo -e '{cmd}\\r' > /dev/umts_atc0; head -n 10 /dev/umts_atc0 2>/dev/null; " +
                            $"elif [ -w /dev/ttyGS0 ]; then echo -e '{cmd}\\r' > /dev/ttyGS0; head -n 10 /dev/ttyGS0 2>/dev/null; " +
                            $"elif [ -w /dev/smd11 ]; then echo -e '{cmd}\\r' > /dev/smd11; head -n 10 /dev/smd11 2>/dev/null; " +
                            $"else echo 'MODEM_NODE_PERMISSION_DENIED'; fi";
            string shellResp = await _adbClient.ExecuteShellCommandAsync(script, false, cancellationToken);
            string trimmed = shellResp.Trim();
            bool shellSuccess = !trimmed.Contains("DENIED") && !trimmed.Contains("ERROR");
            Log($"[RESP <<] ADB Device Response: {trimmed} -> [{(shellSuccess ? "OK" : "FAILED")}]");
            return trimmed;
        }
        catch (Exception ex)
        {
            Log($"[FAIL] Execution failed for '{cmd}': {ex.Message}");
            return $"ERROR: {ex.Message}";
        }
    }

    public async Task<List<(string Command, string Response, bool Success)>> ExecuteBatchCommandsAsync(
        string batchCommands,
        bool useDirectModem = false,
        Action<string>? logCallback = null,
        CancellationToken cancellationToken = default)
    {
        void Log(string message)
        {
            string formatted = $"[{DateTime.Now:HH:mm:ss.fff}] {message}";
            logCallback?.Invoke(formatted);
            OnLog?.Invoke(formatted);
        }

        var results = new List<(string, string, bool)>();
        if (string.IsNullOrWhiteSpace(batchCommands))
        {
            Log("[WARN] No commands provided in batch input.");
            return results;
        }

        string[] lines = batchCommands.Split(new[] { "\r\n", "\r", "\n" }, StringSplitOptions.RemoveEmptyEntries);
        var validCommands = lines.Select(l => l.Trim())
                                 .Where(l => !string.IsNullOrWhiteSpace(l) && !l.StartsWith("#") && !l.StartsWith("//"))
                                 .ToList();

        Log($"[STEP] Starting sequential batch execution of {validCommands.Count} commands...");

        for (int i = 0; i < validCommands.Count; i++)
        {
            cancellationToken.ThrowIfCancellationRequested();
            string cmd = validCommands[i];

            Log($"[CMD >>] [{i + 1}/{validCommands.Count}] Sending: {cmd}");
            string resp = await ExecuteRawCommandAsync(cmd, useDirectModem, logCallback, cancellationToken);
            bool success = resp.Contains("OK", StringComparison.OrdinalIgnoreCase) && !resp.Contains("ERROR", StringComparison.OrdinalIgnoreCase);

            results.Add((cmd, resp, success));

            if (i < validCommands.Count - 1)
            {
                await Task.Delay(180, cancellationToken);
            }
        }

        int successCount = results.Count(r => r.Success);
        Log($"[OK] Batch execution finished: {successCount}/{results.Count} commands succeeded.");
        return results;
    }
}
