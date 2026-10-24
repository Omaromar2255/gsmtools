using GsmTools.Desktop.Core;
using GsmTools.Desktop.Workflows.Models;

namespace GsmTools.Desktop.Workflows;

public interface ISimLockWorkflow
{
    event Action<string>? OnLog;
    Task<SimLockInfo> QuerySimLocksAsync(Action<string>? logCallback = null, CancellationToken cancellationToken = default);
}

public class SimLockWorkflow : ISimLockWorkflow
{
    private readonly IAdbRawSocketClient _adbClient;
    private readonly IPersistentAtBridgeSession _atSession;

    public event Action<string>? OnLog;

    public SimLockWorkflow(IAdbRawSocketClient adbClient, IPersistentAtBridgeSession atSession)
    {
        _adbClient = adbClient ?? throw new ArgumentNullException(nameof(adbClient));
        _atSession = atSession ?? throw new ArgumentNullException(nameof(atSession));
    }

    public async Task<SimLockInfo> QuerySimLocksAsync(Action<string>? logCallback = null, CancellationToken cancellationToken = default)
    {
        void Log(string message)
        {
            string formatted = $"[{DateTime.Now:HH:mm:ss.fff}] {message}";
            logCallback?.Invoke(formatted);
            OnLog?.Invoke(formatted);
        }

        Log("Querying SIM card status and 3GPP network personalization locks...");

        if (_atSession.IsActive)
        {
            try
            {
                string bridgeLocks = await _atSession.QuerySimLocksAsync(cancellationToken);
                Log($"Bridge Response: {bridgeLocks}");
                if (bridgeLocks.Contains("SIMLOCK|"))
                {
                    return SimLockParser.ParseFromBridgePipeString(bridgeLocks);
                }
            }
            catch (Exception ex)
            {
                Log($"Warning: AT bridge LOCKS query failed ({ex.Message}), falling back to direct AT commands...");
            }
        }

        // Direct ADB shell query fallback
        if (!_adbClient.IsConnected)
        {
            await _adbClient.ConnectAsync(cancellationToken: cancellationToken);
            await _adbClient.SelectDeviceAsync(cancellationToken: cancellationToken);
        }

        string cpin = await RunAtCommand("AT+CPIN?", cancellationToken);
        Log($"CPIN State: {cpin}");

        string clckPn = await RunAtCommand("AT+CLCK=\"PN\",2", cancellationToken);
        Log($"Network Lock (PN): {clckPn}");

        string clckPu = await RunAtCommand("AT+CLCK=\"PU\",2", cancellationToken);
        string clckPp = await RunAtCommand("AT+CLCK=\"PP\",2", cancellationToken);
        string clckPc = await RunAtCommand("AT+CLCK=\"PC\",2", cancellationToken);
        string clckSc = await RunAtCommand("AT+CLCK=\"SC\",2", cancellationToken);

        var lockInfo = SimLockParser.ParseFromResponses(cpin, clckPn, clckPu, clckPp, clckPc, clckSc);
        Log($"SIM & Network lock audit complete: {lockInfo.Summary}");
        return lockInfo;
    }

    private async Task<string> RunAtCommand(string cmd, CancellationToken ct)
    {
        try
        {
            string script = $"if [ -w /dev/umts_atc0 ]; then echo -e '{cmd}\\r' > /dev/umts_atc0; head -n 5 /dev/umts_atc0 2>/dev/null; else echo 'NO_AT_PORT'; fi";
            string res = await _adbClient.ExecuteShellCommandAsync(script, false, ct);
            return res.Trim();
        }
        catch
        {
            return "ERROR";
        }
    }
}
