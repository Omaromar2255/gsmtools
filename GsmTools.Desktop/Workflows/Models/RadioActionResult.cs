namespace GsmTools.Desktop.Workflows.Models;

public record RadioActionResult(
    bool Success,
    string Action,
    string Response,
    string Message,
    TimeSpan Duration
);
