#if !REFERENCE_GSMTOOLS_DESKTOP
namespace GsmTools.Desktop.Core
{
    public record AtBridgeBanner(
        string Model,
        string Csc,
        string AndroidVersion,
        string SecurityPatch,
        string DebugLevel,
        int Uid,
        string ActiveChannel
    );
}

namespace GsmTools.Desktop.Workflows.Models
{
    public record DeviceInfo(
        string Model,
        string Serial,
        string Imei,
        string CurrentCsc,
        string ProductCode,
        string BuildNumber,
        string AndroidVersion,
        string SecurityPatch,
        string UsbConfig = "mtp,adb",
        bool IsSelinuxEnforcing = true,
        bool HasRsuApp = false,
        bool HasTsdaemon = false,
        bool HasKeyTool = false,
        bool Port2600Available = true,
        string RepairCalVersion = "Unknown",
        string MobileDoctorVersion = "Unknown",
        string RetailAgentVersion = "Unknown",
        string ActiveAtChannel = "NONE",
        string SimCardStatus = "Unknown",
        string NetworkLockStatus = "Unknown",
        string BasebandVersion = "Unknown",
        string Imsi = "Unknown",
        string HardwareLock = "NONE",
        string RawDevConInfo = ""
    );

    public record WorkflowStepProgress(
        int StepNumber,
        int TotalSteps,
        string StepName,
        string StatusMessage,
        double ProgressPercentage
    );

    public record CscChangeResult(
        bool Success,
        string OldCsc,
        string NewCsc,
        string Message,
        TimeSpan Duration,
        List<string> ExecutionLog
    );

    public record BoostUnlockResult(
        bool Success,
        string Message,
        TimeSpan Duration,
        int StepsCompleted,
        IReadOnlyList<string> ExecutionLog
    );

    public record CarrierRelockResult(
        bool Success,
        string TargetMcc,
        string TargetMnc,
        string NetworkName,
        string BcdPayload,
        string Message,
        TimeSpan Duration,
        IReadOnlyList<string> ExecutionLog
    );

    public record RadioActionResult(
        bool Success,
        string Action,
        string Response,
        string Message,
        TimeSpan Duration
    );

    public record SecurityChallengeResult(
        bool Success,
        string ChallengeSeed,
        string ResponseHash,
        bool IsAuthenticated,
        string Message
    );

    public record SimLockInfo(
        string CpinState,
        string NetworkLock,
        string NetworkSubsetLock,
        string ServiceProviderLock,
        string CorporateLock,
        string SimPinLock,
        bool IsFullyUnlocked,
        string Summary
    );

    public record MccMncEntry(
        string Mcc,
        string Mnc,
        string Iso,
        string Country,
        string Network,
        string BcdPayload
    );
}
#endif
