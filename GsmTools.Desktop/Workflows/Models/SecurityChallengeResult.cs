namespace GsmTools.Desktop.Workflows.Models;

public record SecurityChallengeResult(
    bool Success,
    string ChallengeSeed,
    string ResponseHash,
    bool IsAuthenticated,
    string Message
);
