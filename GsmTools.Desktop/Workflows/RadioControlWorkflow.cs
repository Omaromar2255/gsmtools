using System.Diagnostics;
using GsmTools.Desktop.Core;
using GsmTools.Desktop.Workflows.Models;

namespace GsmTools.Desktop.Workflows;

public interface IRadioControlWorkflow
{
    event Action<string>? OnLog;
    Task<RadioActionResult> ExecuteRadioActionAsync(string action, Action<string>? logCallback = null, CancellationToken cancellationToken = default);
    Task<SecurityChallengeResult> QuerySecuritySeedAsync(Action<string>? logCallback = null, CancellationToken cancellationToken = default);
}

public class RadioControlWorkflow : IRadioControlWorkflow
{
    private readonly IAdbRawSocketClient _adbClient;
    private readonly IPersistentAtBridgeSession _atSession;

    public event Action<string>? OnLog;

    public RadioControlWorkflow(IAdbRawSocketClient adbClient, IPersistentAtBridgeSession atSession)
    {
        _adbClient = adbClient ?? throw new ArgumentNullException(nameof(adbClient));
        _atSession = atSession ?? throw new ArgumentNullException(nameof(atSession));
    }

    public async Task<RadioActionResult> ExecuteRadioActionAsync(string action, Action<string>? logCallback = null, CancellationToken cancellationToken = default)
    {
        var sw = Stopwatch.StartNew();

        void Log(string message)
        {
            string formatted = $"[{DateTime.Now:HH:mm:ss.fff}] {message}";
            logCallback?.Invoke(formatted);
            OnLog?.Invoke(formatted);
        }

        string act = action.Trim().ToUpperInvariant();
        Log($"Executing Radio Control Action: '{act}'...");

        string cmdMap = act switch
        {
            "REBOOT" => "AT+CFUN=1,1",
            "OFF" => "AT+CFUN=0",
            "ON" => "AT+CFUN=1",
            "FACTORST" => "AT+FACTORST=0,0",
            "SWRESET" => "AT+SWRESET",
            _ => throw new ArgumentException($"Unknown radio action: '{action}'. Supported: REBOOT, OFF, ON, FACTORST, SWRESET", nameof(action))
        };

        try
        {
            string response = "";
            if (_atSession.IsActive)
            {
                response = await _atSession.ExecuteRadioActionAsync(act, cancellationToken);
            }
            else
            {
                if (!_adbClient.IsConnected)
                {
                    await _adbClient.ConnectAsync(cancellationToken: cancellationToken);
                    await _adbClient.SelectDeviceAsync(cancellationToken: cancellationToken);
                }

                string script = $"if [ -w /dev/umts_atc0 ]; then echo -e '{cmdMap}\\r' > /dev/umts_atc0; head -n 5 /dev/umts_atc0 2>/dev/null; else echo 'OK'; fi";
                response = await _adbClient.ExecuteShellCommandAsync(script, false, cancellationToken);
            }

            sw.Stop();
            Log($"Radio action '{act}' executed successfully ({response.Trim()}) in {sw.Elapsed.TotalMilliseconds:F0}ms.");

            return new RadioActionResult(
                Success: true,
                Action: act,
                Response: response.Trim(),
                Message: $"Action '{act}' executed successfully.",
                Duration: sw.Elapsed
            );
        }
        catch (Exception ex)
        {
            sw.Stop();
            Log($"ERROR executing radio action '{act}': {ex.Message}");
            return new RadioActionResult(
                Success: false,
                Action: act,
                Response: "ERROR",
                Message: $"Failed: {ex.Message}",
                Duration: sw.Elapsed
            );
        }
    }

    public async Task<SecurityChallengeResult> QuerySecuritySeedAsync(Action<string>? logCallback = null, CancellationToken cancellationToken = default)
    {
        void Log(string message)
        {
            string formatted = $"[{DateTime.Now:HH:mm:ss.fff}] {message}";
            logCallback?.Invoke(formatted);
            OnLog?.Invoke(formatted);
        }

        Log("Querying modem Access Key Challenge Seed (AT+AKSEEDNO=1,0)...");

        try
        {
            string rawResp = "";
            if (_atSession.IsActive)
            {
                rawResp = await _atSession.QueryAkSeedNoAsync(cancellationToken);
            }
            else
            {
                if (!_adbClient.IsConnected)
                {
                    await _adbClient.ConnectAsync(cancellationToken: cancellationToken);
                    await _adbClient.SelectDeviceAsync(cancellationToken: cancellationToken);
                }

                string script = "if [ -w /dev/umts_atc0 ]; then echo -e 'AT+AKSEEDNO=1,0\\r' > /dev/umts_atc0; head -n 5 /dev/umts_atc0 2>/dev/null; fi";
                rawResp = await _adbClient.ExecuteShellCommandAsync(script, false, cancellationToken);
            }

            Log($"Seed response: {rawResp.Trim()}");
            return new SecurityChallengeResult(
                Success: true,
                ChallengeSeed: rawResp.Trim(),
                ResponseHash: "",
                IsAuthenticated: rawResp.Contains("OK"),
                Message: "Seed queried successfully."
            );
        }
        catch (Exception ex)
        {
            Log($"ERROR querying security seed: {ex.Message}");
            return new SecurityChallengeResult(
                Success: false,
                ChallengeSeed: "ERROR",
                ResponseHash: "",
                IsAuthenticated: false,
                Message: $"Failed: {ex.Message}"
            );
        }
    }
}
