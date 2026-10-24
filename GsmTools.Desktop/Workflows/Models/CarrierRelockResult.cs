namespace GsmTools.Desktop.Workflows.Models;

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
