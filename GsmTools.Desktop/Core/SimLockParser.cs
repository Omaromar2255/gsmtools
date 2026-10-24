using System.Text.RegularExpressions;
using GsmTools.Desktop.Workflows.Models;

namespace GsmTools.Desktop.Core;

public static class SimLockParser
{
    public static SimLockInfo ParseFromResponses(
        string cpinRaw,
        string clckPnRaw,
        string clckPuRaw = "",
        string clckPpRaw = "",
        string clckPcRaw = "",
        string clckScRaw = "")
    {
        string cpinState = ExtractCpinState(cpinRaw);
        string pn = ExtractClckStatus(clckPnRaw);
        string pu = ExtractClckStatus(clckPuRaw);
        string pp = ExtractClckStatus(clckPpRaw);
        string pc = ExtractClckStatus(clckPcRaw);
        string sc = ExtractClckStatus(clckScRaw);

        bool isUnlocked = (pn == "UNLOCKED (0)" || pn == "UNLOCKED") &&
                          (pu != "LOCKED (1)") &&
                          (pp != "LOCKED (1)") &&
                          (pc != "LOCKED (1)");

        string summary = isUnlocked ? "Device Unlocked" : "Network Locked";
        if (cpinState != "READY")
            summary += $" (SIM: {cpinState})";

        return new SimLockInfo(
            CpinState: cpinState,
            NetworkLock: pn,
            NetworkSubsetLock: pu,
            ServiceProviderLock: pp,
            CorporateLock: pc,
            SimPinLock: sc,
            IsFullyUnlocked: isUnlocked,
            Summary: summary
        );
    }

    public static SimLockInfo ParseFromBridgePipeString(string line)
    {
        // Example: SIMLOCK|CPIN=READY|PN=UNLOCKED(0)|PU=UNLOCKED(0)|PP=UNLOCKED(0)|PC=UNLOCKED(0)|SC=UNLOCKED(0)
        var parts = line.Split('|');
        var dict = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);

        foreach (var p in parts)
        {
            var kv = p.Split('=', 2);
            if (kv.Length == 2)
            {
                dict[kv[0].Trim()] = kv[1].Trim();
            }
        }

        string Get(string k) => dict.TryGetValue(k, out var v) ? v : "UNKNOWN";

        string cpin = Get("CPIN");
        string pn = Get("PN");
        string pu = Get("PU");
        string pp = Get("PP");
        string pc = Get("PC");
        string sc = Get("SC");

        bool isUnlocked = pn.Contains("0") || pn.Contains("UNLOCKED", StringComparison.OrdinalIgnoreCase);
        string summary = isUnlocked ? "Device Unlocked" : "Network Locked";

        return new SimLockInfo(
            CpinState: cpin,
            NetworkLock: pn,
            NetworkSubsetLock: pu,
            ServiceProviderLock: pp,
            CorporateLock: pc,
            SimPinLock: sc,
            IsFullyUnlocked: isUnlocked,
            Summary: summary
        );
    }

    private static string ExtractCpinState(string raw)
    {
        if (string.IsNullOrWhiteSpace(raw)) return "UNKNOWN";
        if (raw.Contains("READY", StringComparison.OrdinalIgnoreCase)) return "READY";
        if (raw.Contains("SIM PIN", StringComparison.OrdinalIgnoreCase)) return "PIN_REQUIRED";
        if (raw.Contains("SIM PUK", StringComparison.OrdinalIgnoreCase)) return "PUK_REQUIRED";
        if (raw.Contains("NOT INSERTED", StringComparison.OrdinalIgnoreCase) || raw.Contains("ERROR: 10")) return "ABSENT";
        if (raw.Contains("ERROR", StringComparison.OrdinalIgnoreCase)) return "MODEM_ERROR";
        return "UNKNOWN";
    }

    private static string ExtractClckStatus(string raw)
    {
        if (string.IsNullOrWhiteSpace(raw)) return "UNKNOWN";
        if (raw.Contains("+CLCK: 0") || raw.Contains("+CLCK:0")) return "UNLOCKED (0)";
        if (raw.Contains("+CLCK: 1") || raw.Contains("+CLCK:1")) return "LOCKED (1)";
        if (raw.Contains("ERROR")) return "NOT_SUPPORTED";
        return "UNKNOWN";
    }
}
