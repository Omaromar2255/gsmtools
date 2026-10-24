namespace GsmTools.Desktop.Workflows.Models;

public record WorkflowStepProgress(
    int StepNumber,
    int TotalSteps,
    string StepName,
    string StatusMessage,
    double ProgressPercentage
);
