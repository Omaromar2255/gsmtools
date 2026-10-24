using System.Globalization;
using System.Text;
using System.Text.RegularExpressions;

namespace GsmTools.Desktop.Core;

/// <summary>
/// Parser for Binder Parcel dumps from 'service call iphonesubinfo 1 s16 com.android.shell'.
/// Accurately decodes 32-bit words containing UTF-16LE characters to extract 15-digit IMEI strings.
/// </summary>
public static class ImeiParcelParser
{
    private static readonly Regex ParcelLineRegex = new(
        @"^0x[0-9a-fA-F]+:[ \t]+(?<hex>[0-9a-fA-F \t]{8,})",
        RegexOptions.Compiled | RegexOptions.Multiline);

    private static readonly Regex DigitsOnlyRegex = new(@"^\d{14,16}$", RegexOptions.Compiled);

    /// <summary>
    /// Parses the raw output of 'service call iphonesubinfo 1 s16 com.android.shell' and returns the IMEI.
    /// </summary>
    public static string ParseImeiFromParcelDump(string rawParcelOutput)
    {
        if (string.IsNullOrWhiteSpace(rawParcelOutput))
            return string.Empty;

        // Method 1: Exact 32-bit Binder Parcel Word Decoding
        try
        {
            var words = Extract32BitWords(rawParcelOutput);
            if (words.Count >= 3)
            {
                // Word 0: Status code (00000000 = NO_ERROR)
                // Word 1: String length (e.g. 0000000f = 15 characters)
                int declaredLength = (int)words[1];
                if (declaredLength is >= 14 and <= 16)
                {
                    var sb = new StringBuilder(declaredLength);
                    for (int i = 2; i < words.Count && sb.Length < declaredLength; i++)
                    {
                        uint word = words[i];
                        // In Android String16 parcel serialization, two UTF-16 characters per 32-bit word
                        char char0 = (char)(word & 0xFFFF);
                        if (char0 != '\0' && sb.Length < declaredLength)
                            sb.Append(char0);

                        char char1 = (char)((word >> 16) & 0xFFFF);
                        if (char1 != '\0' && sb.Length < declaredLength)
                            sb.Append(char1);
                    }

                    string candidate = sb.ToString().Trim();
                    if (DigitsOnlyRegex.IsMatch(candidate))
                        return candidate;
                }
            }
        }
        catch
        {
            // Fall through to fallback extraction
        }

        // Method 2: ASCII Dump Char Extraction Fallback
        // Matches quoted lines like '........3.5.5.7.'
        try
        {
            var charMatch = Regex.Matches(rawParcelOutput, @"'([^']*)'");
            var sbFallback = new StringBuilder();
            foreach (Match match in charMatch)
            {
                string text = match.Groups[1].Value;
                foreach (char c in text)
                {
                    if (char.IsDigit(c))
                        sbFallback.Append(c);
                }
            }

            string digits = sbFallback.ToString();
            if (digits.Length >= 15)
                return digits.Substring(0, 15);
        }
        catch
        {
            // Ignore
        }

        return string.Empty;
    }

    private static List<uint> Extract32BitWords(string input)
    {
        var words = new List<uint>();
        var matches = ParcelLineRegex.Matches(input);

        foreach (Match match in matches)
        {
            string hexPart = match.Groups["hex"].Value.Trim();
            // Stop before any single quote or ASCII representation
            int quoteIndex = hexPart.IndexOf('\'');
            if (quoteIndex >= 0)
                hexPart = hexPart.Substring(0, quoteIndex).Trim();

            string[] tokens = hexPart.Split(new[] { ' ', '\t' }, StringSplitOptions.RemoveEmptyEntries);
            foreach (string token in tokens)
            {
                if (token.Length == 8 && uint.TryParse(token, NumberStyles.HexNumber, CultureInfo.InvariantCulture, out uint val))
                {
                    words.Add(val);
                }
            }
        }

        return words;
    }
}
