namespace GsmTools.Desktop.Workflows.Models;

public record DeviceInfo(
    string Model,
    string Serial,
    string Imei,
    string CurrentCsc,
    string ProductCode,
    string BuildNumber,
    string AndroidVersion,
    string SecurityPatch,
    string UsbConfig,
    bool IsSelinuxEnforcing,
    bool HasRsuApp,
    bool HasTsdaemon,
    bool HasKeyTool,
    bool Port2600Available,
    string RepairCalVersion,
    string MobileDoctorVersion,
    string RetailAgentVersion,
    string ActiveAtChannel = "NONE",
    string SimCardStatus = "Unknown",
    string NetworkLockStatus = "Unknown",
    string BasebandVersion = "Unknown",
    string Imsi = "Unknown",
    string HardwareLock = "NONE",
    string RawDevConInfo = ""
);
