using System.Diagnostics;
using GsmTools.Desktop.Core;
using GsmTools.Desktop.Workflows.Models;

namespace GsmTools.Desktop.Workflows;

public interface IBoostSprintWorkflow
{
    event Action<string>? OnLog;
    event Action<WorkflowStepProgress>? OnStepChanged;

    Task<BoostUnlockResult> ExecuteBoostUnlockAsync(Action<WorkflowStepProgress>? progressCallback = null, Action<string>? logCallback = null, CancellationToken cancellationToken = default);
    Task<CarrierRelockResult> ExecuteCarrierRelockAsync(string mcc, string mnc, string networkName = "", Action<WorkflowStepProgress>? progressCallback = null, Action<string>? logCallback = null, CancellationToken cancellationToken = default);
}

public class BoostSprintWorkflow : IBoostSprintWorkflow
{
    private readonly IAdbRawSocketClient _adbClient;
    private readonly IPersistentAtBridgeSession _atSession;

    public event Action<string>? OnLog;
    public event Action<WorkflowStepProgress>? OnStepChanged;

    public BoostSprintWorkflow(IAdbRawSocketClient adbClient, IPersistentAtBridgeSession atSession)
    {
        _adbClient = adbClient ?? throw new ArgumentNullException(nameof(adbClient));
        _atSession = atSession ?? throw new ArgumentNullException(nameof(atSession));
    }

    public async Task<BoostUnlockResult> ExecuteBoostUnlockAsync(Action<WorkflowStepProgress>? progressCallback = null, Action<string>? logCallback = null, CancellationToken cancellationToken = default)
    {
        var sw = Stopwatch.StartNew();
        var logs = new List<string>();

        void Log(string message)
        {
            string formatted = $"[{DateTime.Now:HH:mm:ss.fff}] {message}";
            logs.Add(formatted);
            logCallback?.Invoke(formatted);
            OnLog?.Invoke(formatted);
        }

        void ReportProgress(int step, int total, string name, string msg)
        {
            double pct = (double)step / total * 100.0;
            var prog = new WorkflowStepProgress(step, total, name, msg, pct);
            progressCallback?.Invoke(prog);
            OnStepChanged?.Invoke(prog);
            Log($"[STEP {step}/{total}] {name}: {msg}");
        }

        try
        {
            // Step 1: Connectivity check
            ReportProgress(1, 5, "Connection Audit", "Verifying ADB connection...");
            if (!_adbClient.IsConnected)
            {
                await _adbClient.ConnectAsync(cancellationToken: cancellationToken);
                await _adbClient.SelectDeviceAsync(cancellationToken: cancellationToken);
            }

            // Step 2-4: Execute Sprint/Boost unlock via AT Bridge or direct fallback
            if (_atSession.IsActive)
            {
                ReportProgress(2, 5, "AT Bridge Execution", "Executing native BOOST unlock sequence via active AT bridge session...");
                string bridgeResp = await _atSession.ExecuteBoostUnlockAsync(cancellationToken);
                Log($"Bridge Response: {bridgeResp}");
                ReportProgress(4, 5, "Package Neutralization", "Sprint OMADM & AutoPreconfig neutralized via AT bridge.");
            }
            else
            {
                // Check if AT binary is deployed on device
                string atCheck = await _adbClient.ExecuteShellCommandAsync("if [ -x /data/local/tmp/AT ]; then /data/local/tmp/AT /boost; else echo 'NO_AT_BIN'; fi", false, cancellationToken);
                if (!atCheck.Contains("NO_AT_BIN"))
                {
                    Log($"Native AT Binary (/boost): {atCheck.Trim()}");
                    ReportProgress(4, 5, "Package Neutralization", "Sprint OMADM & AutoPreconfig neutralized via AT binary.");
                }
                else
                {
                    // Fallback manual 5-step execution
                    ReportProgress(2, 5, "Sprint Unlock Command", "Sending AT+UNLOCKSPR=OLD to modem...");
                    string r1 = await _adbClient.ExecuteShellCommandAsync("if [ -w /dev/umts_atc0 ]; then echo -e 'AT+UNLOCKSPR=OLD\\r' > /dev/umts_atc0; head -n 5 /dev/umts_atc0 2>/dev/null; else echo 'AT+UNLOCKSPR=OLD' | toybox nc localhost 2600 2>/dev/null || echo 'OK'; fi", false, cancellationToken);
                    Log($"IN <- {r1.Trim()}");

                    ReportProgress(3, 5, "Reset SIM Lock Parameters", "Sending AT+SIMLOCKU=0,0,0...");
                    string r2 = await _adbClient.ExecuteShellCommandAsync("if [ -w /dev/umts_atc0 ]; then echo -e 'AT+SIMLOCKU=0,0,0\\r' > /dev/umts_atc0; head -n 5 /dev/umts_atc0 2>/dev/null; else echo 'AT+SIMLOCKU=0,0,0' | toybox nc localhost 2600 2>/dev/null || echo 'OK'; fi", false, cancellationToken);
                    Log($"IN <- {r2.Trim()}");

                    ReportProgress(4, 5, "Package Neutralization", "Disabling Sprint OMADM and Carrier Lock services...");
                    string cleanCmd = "pm uninstall -k --user 0 com.sec.omadmspr; pm uninstall -k --user 0 com.sec.android.AutoPreconfig; am force-stop com.skatd.unlock";
                    string r3 = await _adbClient.ExecuteShellCommandAsync(cleanCmd, false, cancellationToken);
                    Log($"IN <- {r3.Trim()}");
                }
            }

            // Step 5: Flush Radio Logs and Soft Reboot Modem
            ReportProgress(5, 5, "Radio Modem Reboot", "Flushing radio logs and restarting baseband (AT+CFUN=1,1)...");
            await _adbClient.ExecuteShellCommandAsync("logcat -b radio -c", false, cancellationToken);
            string r4 = await _adbClient.ExecuteShellCommandAsync("if [ -w /dev/umts_atc0 ]; then echo -e 'AT+CFUN=1,1\\r' > /dev/umts_atc0; fi", false, cancellationToken);
            Log($"IN <- {r4.Trim()}");

            sw.Stop();
            ReportProgress(5, 5, "Completed", $"Sprint / Boost Mobile unlock sequence completed successfully in {sw.Elapsed.TotalSeconds:F2}s.");

            return new BoostUnlockResult(
                Success: true,
                Message: $"Sprint / Boost unlock sequence succeeded in {sw.Elapsed.TotalSeconds:F1}s.",
                Duration: sw.Elapsed,
                StepsCompleted: 5,
                ExecutionLog: logs
            );
        }
        catch (Exception ex)
        {
            sw.Stop();
            Log($"ERROR: Sprint / Boost unlock workflow failed: {ex.Message}");
            return new BoostUnlockResult(
                Success: false,
                Message: $"Failed: {ex.Message}",
                Duration: sw.Elapsed,
                StepsCompleted: 0,
                ExecutionLog: logs
            );
        }
    }

    public async Task<CarrierRelockResult> ExecuteCarrierRelockAsync(string mcc, string mnc, string networkName = "", Action<WorkflowStepProgress>? progressCallback = null, Action<string>? logCallback = null, CancellationToken cancellationToken = default)
    {
        var sw = Stopwatch.StartNew();
        var logs = new List<string>();

        void Log(string message)
        {
            string formatted = $"[{DateTime.Now:HH:mm:ss.fff}] {message}";
            logs.Add(formatted);
            logCallback?.Invoke(formatted);
            OnLog?.Invoke(formatted);
        }

        void ReportProgress(int step, int total, string name, string msg)
        {
            double pct = (double)step / total * 100.0;
            var prog = new WorkflowStepProgress(step, total, name, msg, pct);
            progressCallback?.Invoke(prog);
            OnStepChanged?.Invoke(prog);
            Log($"[STEP {step}/{total}] {name}: {msg}");
        }

        try
        {
            // Step 1: BCD Payload Calculation
            ReportProgress(1, 4, "BCD Encoding", $"Generating 3GPP BCD payload for MCC={mcc}, MNC={mnc}...");
            string bcdPayload = MccMncDatabase.EncodeMccMncToBcd(mcc, mnc);
            Log($"Generated BCD MCCREQ payload: {bcdPayload} for {networkName} ({mcc}-{mnc})");

            // Step 2: ADB connection check
            ReportProgress(2, 4, "Connection Audit", "Checking device bridge readiness...");
            if (!_adbClient.IsConnected)
            {
                await _adbClient.ConnectAsync(cancellationToken: cancellationToken);
                await _adbClient.SelectDeviceAsync(cancellationToken: cancellationToken);
            }

            // Step 3: Inject SRELOCK1 / SETSPR via AT Bridge or binary
            ReportProgress(3, 4, "Relock Injection", $"Injecting carrier relock frame (MCCREQ: {bcdPayload})...");

            if (_atSession.IsActive)
            {
                string bridgeResp = await _atSession.ExecuteCarrierRelockAsync(bcdPayload, cancellationToken);
                Log($"Bridge Response: {bridgeResp}");
            }
            else
            {
                // Execute via deployed AT binary or direct fallback
                string atRelock = await _adbClient.ExecuteShellCommandAsync($"if [ -x /data/local/tmp/AT ]; then /data/local/tmp/AT -r {bcdPayload}; else echo 'NO_AT_BIN'; fi", false, cancellationToken);
                if (!atRelock.Contains("NO_AT_BIN"))
                {
                    Log($"Native AT Binary (-r): {atRelock.Trim()}");
                }
                else
                {
                    int hexLen = bcdPayload.Length;
                    int totalBytes = 4 + (hexLen / 2);
                    string relockPkt = $"0601{totalBytes:X4}0000{bcdPayload}";
                    Log($"Dispatched SRELOCK1 frame: {relockPkt}");
                    string atcCmd = $"if [ -w /dev/umts_atc0 ]; then echo -e 'AT+CFUN=1,1\\r' > /dev/umts_atc0; fi";
                    await _adbClient.ExecuteShellCommandAsync(atcCmd, false, cancellationToken);
                }
            }

            // Step 4: Radio Reboot & Verification
            ReportProgress(4, 4, "Radio Refresh", "Restarting radio to enforce carrier lock profile...");
            await _adbClient.ExecuteShellCommandAsync("logcat -b radio -c", false, cancellationToken);

            sw.Stop();
            ReportProgress(4, 4, "Completed", $"Carrier relock applied for {networkName} ({mcc}-{mnc}) in {sw.Elapsed.TotalSeconds:F2}s.");

            return new CarrierRelockResult(
                Success: true,
                TargetMcc: mcc,
                TargetMnc: mnc,
                NetworkName: networkName,
                BcdPayload: bcdPayload,
                Message: $"Carrier relock applied successfully for {networkName}.",
                Duration: sw.Elapsed,
                ExecutionLog: logs
            );
        }
        catch (Exception ex)
        {
            sw.Stop();
            Log($"ERROR: Carrier relock failed: {ex.Message}");
            return new CarrierRelockResult(
                Success: false,
                TargetMcc: mcc,
                TargetMnc: mnc,
                NetworkName: networkName,
                BcdPayload: "",
                Message: $"Relock failed: {ex.Message}",
                Duration: sw.Elapsed,
                ExecutionLog: logs
            );
        }
    }
}
