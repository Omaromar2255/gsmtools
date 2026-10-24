using GsmTools.Desktop.Core;

namespace GsmTools.Desktop.Workflows;

public interface IInteractiveAtWorkflow
{
    event Action<string>? OnLog;
    Task<string> ExecuteRawCommandAsync(string command, Action<string>? logCallback = null, CancellationToken cancellationToken = default);
}

public class InteractiveAtWorkflow : IInteractiveAtWorkflow
{
    private readonly IAdbRawSocketClient _adbClient;
    private readonly IPersistentAtBridgeSession _atSession;

    public event Action<string>? OnLog;

    public InteractiveAtWorkflow(IAdbRawSocketClient adbClient, IPersistentAtBridgeSession atSession)
    {
        _adbClient = adbClient ?? throw new ArgumentNullException(nameof(adbClient));
        _atSession = atSession ?? throw new ArgumentNullException(nameof(atSession));
    }

    public async Task<string> ExecuteRawCommandAsync(string command, Action<string>? logCallback = null, CancellationToken cancellationToken = default)
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

        Log($"Executing terminal command: '{cmd}'");

        try
        {
            if (_atSession.IsActive)
            {
                if (cmd.StartsWith("AT", StringComparison.OrdinalIgnoreCase))
                {
                    string resp = await _atSession.SendAtCommandAsync(cmd, cancellationToken);
                    Log($"Bridge AT Response: {resp}");
                    return resp;
                }
                else if (System.Text.RegularExpressions.Regex.IsMatch(cmd, "^[0-9A-Fa-f]+$"))
                {
                    string resp = await _atSession.SendIpcHexCommandAsync(cmd, cancellationToken);
                    Log($"Bridge IPC Response: {resp}");
                    return resp;
                }
                else
                {
                    // Direct bridge interactive command (e.g. STATUS, LOCKS, DEVINFO)
                    string resp = await _atSession.SendAtCommandAsync(cmd, cancellationToken);
                    Log($"Bridge Response: {resp}");
                    return resp;
                }
            }

            // Direct ADB shell fallback
            if (!_adbClient.IsConnected)
            {
                await _adbClient.ConnectAsync(cancellationToken: cancellationToken);
                await _adbClient.SelectDeviceAsync(cancellationToken: cancellationToken);
            }

            string script = $"if [ -w /dev/umts_atc0 ]; then echo -e '{cmd}\\r' > /dev/umts_atc0; head -n 10 /dev/umts_atc0 2>/dev/null; else echo 'MODEM_TTY_UNAVAILABLE'; fi";
            string shellResp = await _adbClient.ExecuteShellCommandAsync(script, false, cancellationToken);
            Log($"ADB Response: {shellResp.Trim()}");
            return shellResp.Trim();
        }
        catch (Exception ex)
        {
            Log($"ERROR executing command '{cmd}': {ex.Message}");
            return $"ERROR: {ex.Message}";
        }
    }
}
