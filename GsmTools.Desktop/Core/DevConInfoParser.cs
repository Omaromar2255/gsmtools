using System.Text.RegularExpressions;

namespace GsmTools.Desktop.Core;

public record ParsedDevConInfo(
    string Model,
    string Baseband,
    string FirmwareVersion,
    string Mnc,
    string Mcc,
    string ProductCode,
    string SerialNumber,
    string Imei,
    string UniqueNumber,
    string LockStatus,
    string SdpState,
    string HardwareId,
    IReadOnlyDictionary<string, string> AllFields
);

public static class DevConInfoParser
{
    private static readonly Regex FieldRegex = new(@"([A-Z0-9_]+)\(([^)]*)\)", RegexOptions.Compiled);

    public static ParsedDevConInfo Parse(string rawDevConInfo)
    {
        var fields = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);

        if (!string.IsNullOrWhiteSpace(rawDevConInfo))
        {
            var matches = FieldRegex.Matches(rawDevConInfo);
            foreach (Match m in matches)
            {
                string key = m.Groups[1].Value.Trim();
                string val = m.Groups[2].Value.Trim();
                fields[key] = val;
            }
        }

        string Get(string key, string fallback = "Unknown") =>
            fields.TryGetValue(key, out var v) && !string.IsNullOrWhiteSpace(v) ? v : fallback;

        return new ParsedDevConInfo(
            Model: Get("MN"),
            Baseband: Get("BASE"),
            FirmwareVersion: Get("VER", Get("HIDVER")),
            Mnc: Get("MNC", "00"),
            Mcc: Get("MCC", "000"),
            ProductCode: Get("PRD"),
            SerialNumber: Get("SN"),
            Imei: Get("IMEI"),
            UniqueNumber: Get("UN"),
            LockStatus: Get("LOCK", "NONE"),
            SdpState: Get("SDP"),
            HardwareId: Get("HVID"),
            AllFields: fields
        );
    }
}
