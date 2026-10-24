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
        bool IsSelinuxEnforcing,
        bool HasRsuApp,
        bool HasTsdaemon,
        bool HasKeyTool,
        bool Port2600Available,
        string RepairCalVersion,
        string MobileDoctorVersion,
        string RetailAgentVersion
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
}
#endif
