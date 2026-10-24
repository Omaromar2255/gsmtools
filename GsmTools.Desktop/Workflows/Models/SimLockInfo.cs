namespace GsmTools.Desktop.Workflows.Models;

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
