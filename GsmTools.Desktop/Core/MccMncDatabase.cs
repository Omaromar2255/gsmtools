using System.IO;
using System.Text.RegularExpressions;
using GsmTools.Desktop.Workflows.Models;

namespace GsmTools.Desktop.Core;

public static class MccMncDatabase
{
    private static readonly List<MccMncEntry> _entries = new();
    private static bool _initialized = false;
    private static readonly object _lock = new();

    public static string EncodeMccMncToBcd(string mcc, string mnc)
    {
        string cleanMcc = (mcc ?? string.Empty).Trim();
        string cleanMnc = (mnc ?? string.Empty).Trim();
        if (cleanMnc.Length == 1)
            cleanMnc = "0" + cleanMnc;

        if (cleanMnc.Length == 2)
            return $"{cleanMcc}F{cleanMnc}00".ToUpperInvariant();

        return $"{cleanMcc}{cleanMnc}00".ToUpperInvariant();
    }

    public static IReadOnlyList<MccMncEntry> GetAllEntries(string? csvPath = null)
    {
        EnsureInitialized(csvPath);
        lock (_lock)
        {
            return _entries.ToList();
        }
    }

    public static IReadOnlyList<MccMncEntry> Search(string query, string? csvPath = null)
    {
        EnsureInitialized(csvPath);
        if (string.IsNullOrWhiteSpace(query))
            return GetAllEntries(csvPath);

        string q = query.Trim();
        lock (_lock)
        {
            return _entries.Where(e =>
                e.Country.Contains(q, StringComparison.OrdinalIgnoreCase) ||
                e.Network.Contains(q, StringComparison.OrdinalIgnoreCase) ||
                e.Mcc.Contains(q, StringComparison.OrdinalIgnoreCase) ||
                e.Mnc.Contains(q, StringComparison.OrdinalIgnoreCase) ||
                e.Iso.Contains(q, StringComparison.OrdinalIgnoreCase)
            ).ToList();
        }
    }

    private static void EnsureInitialized(string? customCsvPath)
    {
        if (_initialized) return;

        lock (_lock)
        {
            if (_initialized) return;

            string? resolvedPath = FindCsvPath(customCsvPath);
            if (resolvedPath != null && File.Exists(resolvedPath))
            {
                try
                {
                    LoadFromCsv(resolvedPath);
                }
                catch
                {
                    LoadFallbackPresets();
                }
            }
            else
            {
                LoadFallbackPresets();
            }

            _initialized = true;
        }
    }

    private static string? FindCsvPath(string? customCsvPath)
    {
        if (!string.IsNullOrWhiteSpace(customCsvPath) && File.Exists(customCsvPath))
            return customCsvPath;

        string[] candidates = new[]
        {
            Path.Combine(AppContext.BaseDirectory, "references", "MCC_MNC_LIST.csv"),
            Path.Combine(Directory.GetCurrentDirectory(), "references", "MCC_MNC_LIST.csv"),
            Path.Combine(AppContext.BaseDirectory, "..", "..", "..", "..", "references", "MCC_MNC_LIST.csv"),
            Path.Combine(AppContext.BaseDirectory, "..", "..", "..", "references", "MCC_MNC_LIST.csv")
        };

        foreach (var path in candidates)
        {
            string full = Path.GetFullPath(path);
            if (File.Exists(full))
                return full;
        }

        return null;
    }

    private static void LoadFromCsv(string csvPath)
    {
        _entries.Clear();
        var lines = File.ReadAllLines(csvPath);
        bool isHeader = true;

        foreach (var rawLine in lines)
        {
            if (isHeader)
            {
                isHeader = false;
                continue;
            }

            if (string.IsNullOrWhiteSpace(rawLine)) continue;

            // Format: MCC,MNC,ISO,Country,Network
            var parts = rawLine.Split(',');
            if (parts.Length >= 5)
            {
                string mcc = parts[0].Trim();
                string mnc = parts[1].Trim();
                string iso = parts[2].Trim();
                string country = parts[3].Trim();
                string network = string.Join(",", parts.Skip(4)).Trim().Trim('"');

                string bcd = EncodeMccMncToBcd(mcc, mnc);
                _entries.Add(new MccMncEntry(mcc, mnc, iso, country, network, bcd));
            }
        }

        if (_entries.Count == 0)
            LoadFallbackPresets();
    }

    private static void LoadFallbackPresets()
    {
        _entries.Clear();
        // Fallback popular carriers
        AddEntry("421", "01", "ye", "Yemen", "Sabafon");
        AddEntry("421", "02", "ye", "Yemen", "MTN / YOU");
        AddEntry("421", "03", "ye", "Yemen", "Yemen Mobile");
        AddEntry("421", "04", "ye", "Yemen", "Y Telecom");
        AddEntry("310", "120", "us", "United States", "Sprint");
        AddEntry("310", "260", "us", "United States", "T-Mobile USA");
        AddEntry("310", "410", "us", "United States", "AT&T Mobility");
        AddEntry("311", "480", "us", "United States", "Verizon Wireless");
        AddEntry("311", "580", "us", "United States", "US Cellular");
        AddEntry("310", "000", "us", "United States", "Boost Mobile");
        AddEntry("450", "05", "kr", "South Korea", "SK Telecom");
        AddEntry("450", "08", "kr", "South Korea", "KT");
        AddEntry("450", "06", "kr", "South Korea", "LG U+");
        AddEntry("420", "01", "sa", "Saudi Arabia", "STC");
        AddEntry("420", "03", "sa", "Saudi Arabia", "Mobily");
        AddEntry("420", "04", "sa", "Saudi Arabia", "Zain");
        AddEntry("424", "02", "ae", "United Arab Emirates", "Etisalat");
        AddEntry("424", "03", "ae", "United Arab Emirates", "du");
    }

    private static void AddEntry(string mcc, string mnc, string iso, string country, string network)
    {
        string bcd = EncodeMccMncToBcd(mcc, mnc);
        _entries.Add(new MccMncEntry(mcc, mnc, iso, country, network, bcd));
    }
}
