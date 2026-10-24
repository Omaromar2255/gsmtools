namespace GsmTools.Desktop.Workflows.Models;

public record MccMncEntry(
    string Mcc,
    string Mnc,
    string Iso,
    string Country,
    string Network,
    string BcdPayload
)
{
    public string DisplayName => $"[{Mcc}-{Mnc}] {Country} - {Network} (BCD: {BcdPayload})";
}
