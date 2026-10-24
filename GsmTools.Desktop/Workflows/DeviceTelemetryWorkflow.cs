using System.Diagnostics;
using System.Text.RegularExpressions;
using GsmTools.Desktop.Core;
using GsmTools.Desktop.Workflows.Models;

namespace GsmTools.Desktop.Workflows;

public interface IDeviceTelemetryWorkflow
{
    event Action<string>? OnLog;
    event Action<DeviceInfo>? OnDeviceInfoUpdated;

    Task<DeviceInfo> ReadDeepTelemetryAsync(Action<string>? logCallback = null, CancellationToken cancellationToken = default);
    Task<ParsedDevConInfo> ReadDevConInfoAsync(Action<string>? logCallback = null, CancellationToken cancellationToken = default);
}

public class DeviceTelemetryWorkflow : IDeviceTelemetryWorkflow
{
    private readonly IAdbRawSocketClient _adbClient;
    private readonly IPersistentAtBridgeSession _atSession;

    public event Action<string>? OnLog;
    public event Action<DeviceInfo>? OnDeviceInfoUpdated;

    public DeviceTelemetryWorkflow(IAdbRawSocketClient adbClient, IPersistentAtBridgeSession atSession)
    {
        _adbClient = adbClient ?? throw new ArgumentNullException(nameof(adbClient));
        _atSession = atSession ?? throw new ArgumentNullException(nameof(atSession));
    }

    public async Task<DeviceInfo> ReadDeepTelemetryAsync(Action<string>? logCallback = null, CancellationToken cancellationToken = default)
    {
        void Log(string message)
        {
            string formatted = $"[{DateTime.Now:HH:mm:ss.fff}] {message}";
            logCallback?.Invoke(formatted);
            OnLog?.Invoke(formatted);
        }

        Log("Acquiring deep device telemetry via ADB and AT modem bridge...");

        if (!_adbClient.IsConnected)
        {
            Log("Connecting to ADB server at 127.0.0.1:5037...");
            await _adbClient.ConnectAsync(cancellationToken: cancellationToken);
            await _adbClient.SelectDeviceAsync(cancellationToken: cancellationToken);
        }

        // 1. getprop Properties
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

        // 2. Security status
        string selinuxRaw = await _adbClient.ExecuteShellCommandAsync("getenforce", false, cancellationToken);
        bool isSelinuxEnforcing = selinuxRaw.Contains("Enforcing", StringComparison.OrdinalIgnoreCase);

        string rsuRaw = await _adbClient.ExecuteShellCommandAsync("pm list package|grep rsuapp", false, cancellationToken);
        bool hasRsuApp = rsuRaw.Contains("rsuapp", StringComparison.OrdinalIgnoreCase);

        string tsdaemonRaw = await _adbClient.ExecuteShellCommandAsync("ps -A|grep tsdaemon;ps|grep tsdaemon", false, cancellationToken);
        bool hasTsdaemon = tsdaemonRaw.Contains("tsdaemon", StringComparison.OrdinalIgnoreCase);

        string keytoolRaw = await _adbClient.ExecuteShellCommandAsync("pm path KEYTool.SRV4", false, cancellationToken);
        bool hasKeyTool = keytoolRaw.Contains("package:", StringComparison.OrdinalIgnoreCase);

        string port2600Raw = await _adbClient.ExecuteShellCommandAsync("echo 'id' |toybox nc localhost 2600", false, cancellationToken);
        bool port2600Available = port2600Raw.Contains("uid=1000", StringComparison.OrdinalIgnoreCase);

        // 3. Diagnostic Packages
        string repairCalRaw = await _adbClient.ExecuteShellCommandAsync("dumpsys package com.samsung.android.app.repaircal | grep -E \"versionName|versionCode\"", false, cancellationToken);
        string mobileDocRaw = await _adbClient.ExecuteShellCommandAsync("dumpsys package com.samsung.android.app.mobiledoctor | grep -E \"versionName|versionCode\"", false, cancellationToken);
        string retailAgentRaw = await _adbClient.ExecuteShellCommandAsync("dumpsys package com.samsung.rms.retailagent.global | grep -E \"versionName|versionCode\"", false, cancellationToken);

        // 4. IMEI via service call
        string imeiRaw = await _adbClient.ExecuteShellCommandAsync("service call iphonesubinfo 1 s16 com.android.shell", false, cancellationToken);
        string imei = ImeiParcelParser.ParseImeiFromParcelDump(imeiRaw);

        // 5. AT Modem Telemetry (AT+DEVCONINFO, CIMI, VERSNAME)
        string baseband = "Unknown";
        string imsi = "Unknown";
        string hwLock = "NONE";
        string rawDevCon = "";
        string simCardStatus = GetPropValue(props, "gsm.sim.state", "ABSENT");
        string networkLockStatus = "Unknown";

        try
        {
            // Try query DEVCONINFO via modem node if available
            string atDevCon = await _adbClient.ExecuteShellCommandAsync("if [ -w /dev/umts_atc0 ]; then echo -e 'AT+DEVCONINFO\\r' > /dev/umts_atc0; head -n 10 /dev/umts_atc0 2>/dev/null; fi", false, cancellationToken);
            if (atDevCon.Contains("+DEVCONINFO:"))
            {
                rawDevCon = atDevCon;
                var parsed = DevConInfoParser.Parse(atDevCon);
                if (parsed.Model != "Unknown") model = parsed.Model;
                if (parsed.Baseband != "Unknown") baseband = parsed.Baseband;
                if (parsed.SerialNumber != "Unknown") serial = parsed.SerialNumber;
                if (parsed.Imei != "Unknown") imei = parsed.Imei;
                if (parsed.LockStatus != "Unknown") hwLock = parsed.LockStatus;
                Log($"[AT-MODEM] +DEVCONINFO: Model={parsed.Model}, Lock={parsed.LockStatus}, Baseband={parsed.Baseband}");
            }

            // AT+CIMI
            string atCimi = await _adbClient.ExecuteShellCommandAsync("if [ -w /dev/umts_atc0 ]; then echo -e 'AT+CIMI\\r' > /dev/umts_atc0; head -n 5 /dev/umts_atc0 2>/dev/null; fi", false, cancellationToken);
            var cimiMatch = Regex.Match(atCimi, @"(\d{14,15})");
            if (cimiMatch.Success)
            {
                imsi = cimiMatch.Value;
                Log($"[AT-MODEM] CIMI IMSI: {imsi}");
            }

            // AT+CLCK="PN",2
            string atProbe = await _adbClient.ExecuteShellCommandAsync("if [ -w /dev/umts_atc0 ]; then echo -e 'AT+CLCK=\"PN\",2\\r' > /dev/umts_atc0; head -n 5 /dev/umts_atc0 2>/dev/null; fi", false, cancellationToken);
            if (atProbe.Contains("+CLCK: 0") || atProbe.Contains("+CLCK:0"))
                networkLockStatus = "UNLOCKED (0)";
            else if (atProbe.Contains("+CLCK: 1") || atProbe.Contains("+CLCK:1"))
                networkLockStatus = "LOCKED (1)";
        }
        catch
        {
            // Shell fallback
        }

        if (networkLockStatus == "Unknown")
        {
            if (hasRsuApp) networkLockStatus = "Carrier RSU Lock (App Detected)";
            else if (simCardStatus.Contains("NETWORK_LOCKED", StringComparison.OrdinalIgnoreCase)) networkLockStatus = "LOCKED (Network Locked)";
            else if (simCardStatus.StartsWith("LOADED", StringComparison.OrdinalIgnoreCase) || simCardStatus.StartsWith("READY", StringComparison.OrdinalIgnoreCase)) networkLockStatus = "UNLOCKED / Active";
            else networkLockStatus = "No SIM / Absent";
        }

        var devInfo = new DeviceInfo(
            Model: model,
            Serial: serial,
            Imei: imei,
            CurrentCsc: currentCsc,
            ProductCode: productCode,
            BuildNumber: buildNumber,
            AndroidVersion: androidVer,
            SecurityPatch: securityPatch,
            UsbConfig: usbConfig,
            IsSelinuxEnforcing: isSelinuxEnforcing,
            HasRsuApp: hasRsuApp,
            HasTsdaemon: hasTsdaemon,
            HasKeyTool: hasKeyTool,
            Port2600Available: port2600Available,
            RepairCalVersion: ExtractVersion(repairCalRaw),
            MobileDoctorVersion: ExtractVersion(mobileDocRaw),
            RetailAgentVersion: ExtractVersion(retailAgentRaw),
            ActiveAtChannel: "/dev/umts_ipc0",
            SimCardStatus: simCardStatus,
            NetworkLockStatus: networkLockStatus,
            BasebandVersion: baseband,
            Imsi: imsi,
            HardwareLock: hwLock,
            RawDevConInfo: rawDevCon
        );

        OnDeviceInfoUpdated?.Invoke(devInfo);
        Log($"Telemetry acquisition completed for model: {model} (Serial: {serial})");
        return devInfo;
    }

    public async Task<ParsedDevConInfo> ReadDevConInfoAsync(Action<string>? logCallback = null, CancellationToken cancellationToken = default)
    {
        string raw = await _adbClient.ExecuteShellCommandAsync("if [ -w /dev/umts_atc0 ]; then echo -e 'AT+DEVCONINFO\\r' > /dev/umts_atc0; head -n 10 /dev/umts_atc0 2>/dev/null; fi", false, cancellationToken);
        return DevConInfoParser.Parse(raw);
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

    private static string ExtractVersion(string packageDump)
    {
        var match = Regex.Match(packageDump, @"versionName=(?<ver>[^\s]+)");
        return match.Success ? match.Groups["ver"].Value.Trim() : "Not Installed";
    }
}
