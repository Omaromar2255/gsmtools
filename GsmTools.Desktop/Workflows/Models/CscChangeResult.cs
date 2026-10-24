namespace GsmTools.Desktop.Workflows.Models;

public record CscChangeResult(
    bool Success,
    string OldCsc,
    string NewCsc,
    string Message,
    TimeSpan Duration,
    List<string> ExecutionLog
);
