namespace GsmTools.Desktop.Workflows.Models;

public record BoostUnlockResult(
    bool Success,
    string Message,
    TimeSpan Duration,
    int StepsCompleted,
    IReadOnlyList<string> ExecutionLog
);
