using System.Windows.Media;

namespace GsmTools.Desktop.UI.Controls;

public enum LogLevel
{
    Info,
    Outgoing,
    Incoming,
    Success,
    Warning,
    Error
}

public class ConsoleLogItem
{
    public string Timestamp { get; set; } = string.Empty;
    public string Direction { get; set; } = string.Empty;
    public string Message { get; set; } = string.Empty;
    public LogLevel Level { get; set; } = LogLevel.Info;

    public Brush ColorBrush => Level switch
    {
        LogLevel.Outgoing => new SolidColorBrush(Color.FromRgb(0x4F, 0xC3, 0xF7)),  // Neon Light Blue / Cyan
        LogLevel.Incoming => new SolidColorBrush(Color.FromRgb(0x81, 0xC7, 0x84)),  // Neon Light Green
        LogLevel.Success  => new SolidColorBrush(Color.FromRgb(0x69, 0xF0, 0xAE)),  // Bright Accent Green
        LogLevel.Warning  => new SolidColorBrush(Color.FromRgb(0xFF, 0xB7, 0x4D)),  // Warm Amber / Orange
        LogLevel.Error    => new SolidColorBrush(Color.FromRgb(0xFF, 0x52, 0x52)),  // Neon Crimson Red
        _                 => new SolidColorBrush(Color.FromRgb(0xCF, 0xD8, 0xDC))   // Monospace Platinum
    };

    public static ConsoleLogItem FromRawString(string rawLine)
    {
        string ts = DateTime.Now.ToString("HH:mm:ss.fff");
        string dir = "INFO";
        string msg = rawLine ?? string.Empty;
        LogLevel lvl = LogLevel.Info;

        // Extract bracketed timestamp: [16:03:05.642] ...
        if (msg.StartsWith("["))
        {
            int closeBracket = msg.IndexOf(']');
            if (closeBracket > 1 && closeBracket < 16)
            {
                ts = msg.Substring(1, closeBracket - 1);
                msg = msg.Substring(closeBracket + 1).Trim();
            }
        }

        // Clean up duplicate prefixes and assign clean Direction
        while (msg.StartsWith("OUT ->", StringComparison.OrdinalIgnoreCase) ||
               msg.StartsWith("[CMD >>]", StringComparison.OrdinalIgnoreCase) ||
               msg.StartsWith("CMD >>", StringComparison.OrdinalIgnoreCase))
        {
            dir = "CMD >>";
            lvl = LogLevel.Outgoing;
            if (msg.StartsWith("OUT ->", StringComparison.OrdinalIgnoreCase))
                msg = msg.Substring(6).Trim();
            else if (msg.StartsWith("[CMD >>]", StringComparison.OrdinalIgnoreCase))
                msg = msg.Substring(8).Trim();
            else if (msg.StartsWith("CMD >>", StringComparison.OrdinalIgnoreCase))
                msg = msg.Substring(6).Trim();
        }

        while (msg.StartsWith("IN <-", StringComparison.OrdinalIgnoreCase) ||
               msg.StartsWith("[RESP <<]", StringComparison.OrdinalIgnoreCase) ||
               msg.StartsWith("RESP <<", StringComparison.OrdinalIgnoreCase))
        {
            dir = "RESP <<";
            lvl = LogLevel.Incoming;
            if (msg.StartsWith("IN <-", StringComparison.OrdinalIgnoreCase))
                msg = msg.Substring(5).Trim();
            else if (msg.StartsWith("[RESP <<]", StringComparison.OrdinalIgnoreCase))
                msg = msg.Substring(9).Trim();
            else if (msg.StartsWith("RESP <<", StringComparison.OrdinalIgnoreCase))
                msg = msg.Substring(7).Trim();
        }

        if (string.IsNullOrWhiteSpace(msg) && (dir == "RESP <<" || lvl == LogLevel.Incoming))
        {
            msg = "(empty response) -> [OK]";
        }

        // Check for Step headers
        if (msg.StartsWith("STEP ", StringComparison.OrdinalIgnoreCase) || msg.StartsWith("[STEP", StringComparison.OrdinalIgnoreCase))
        {
            dir = "STEP";
            lvl = LogLevel.Warning;
            if (msg.StartsWith("STEP ", StringComparison.OrdinalIgnoreCase))
                msg = msg.Substring(5).Trim();
        }
        else if (msg.StartsWith("INFO ", StringComparison.OrdinalIgnoreCase))
        {
            dir = "INFO";
            lvl = LogLevel.Info;
            msg = msg.Substring(5).Trim();
        }
        else if (msg.StartsWith("ERR ", StringComparison.OrdinalIgnoreCase) || msg.StartsWith("ERROR", StringComparison.OrdinalIgnoreCase))
        {
            dir = "ERR";
            lvl = LogLevel.Error;
            if (msg.StartsWith("ERR ", StringComparison.OrdinalIgnoreCase))
                msg = msg.Substring(4).Trim();
        }
        else if (msg.StartsWith("OK ", StringComparison.OrdinalIgnoreCase))
        {
            dir = "OK";
            lvl = LogLevel.Success;
            msg = msg.Substring(3).Trim();
        }

        // Detect Success / Failure tags in message
        if (msg.Contains("[SUCCESS]") || msg.Contains("-> [OK]") || msg.Contains("completed successfully") || msg.Contains(".OK..") || msg.Contains(".OK_CSC.."))
        {
            if (lvl != LogLevel.Outgoing)
            {
                lvl = LogLevel.Success;
            }
        }
        else if (msg.Contains("[FAILED]") || msg.Contains("failed") || msg.Contains("ERROR") || msg.Contains(".ERR_IPC..") || msg.Contains("Connection refused"))
        {
            lvl = LogLevel.Error;
            if (dir == "INFO" || dir == "RESP <<")
            {
                dir = "FAIL";
            }
        }

        return new ConsoleLogItem
        {
            Timestamp = ts,
            Direction = dir,
            Message = msg,
            Level = lvl
        };
    }
}
