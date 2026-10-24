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
        LogLevel.Outgoing => new SolidColorBrush(Color.FromRgb(0x4F, 0xC3, 0xF7)),  // Neon Light Blue
        LogLevel.Incoming => new SolidColorBrush(Color.FromRgb(0x81, 0xC7, 0x84)),  // Neon Light Green
        LogLevel.Success  => new SolidColorBrush(Color.FromRgb(0x69, 0xF0, 0xAE)),  // Bright Accent Green
        LogLevel.Warning  => new SolidColorBrush(Color.FromRgb(0xFF, 0xB7, 0x4D)),  // Warm Amber
        LogLevel.Error    => new SolidColorBrush(Color.FromRgb(0xFF, 0x52, 0x52)),  // Neon Red
        _                 => new SolidColorBrush(Color.FromRgb(0xCF, 0xD8, 0xDC))   // Monospace Platinum
    };

    public static ConsoleLogItem FromRawString(string rawLine)
    {
        // Sample: [17:40:24.928] OUT -> shell:logcat -b radio -c
        string ts = DateTime.Now.ToString("HH:mm:ss.fff");
        string dir = "INFO";
        string msg = rawLine;
        LogLevel lvl = LogLevel.Info;

        if (rawLine.StartsWith("["))
        {
            int closeBracket = rawLine.IndexOf(']');
            if (closeBracket > 1)
            {
                ts = rawLine.Substring(1, closeBracket - 1);
                msg = rawLine.Substring(closeBracket + 1).Trim();
            }
        }

        if (msg.StartsWith("OUT ->", StringComparison.OrdinalIgnoreCase))
        {
            dir = "OUT ->";
            lvl = LogLevel.Outgoing;
        }
        else if (msg.StartsWith("IN <-", StringComparison.OrdinalIgnoreCase))
        {
            dir = "IN <-";
            lvl = LogLevel.Incoming;
        }
        else if (msg.Contains(".OK..") || msg.Contains("confirmed") || msg.Contains("successfully"))
        {
            dir = "OK";
            lvl = LogLevel.Success;
        }
        else if (msg.StartsWith("ERROR", StringComparison.OrdinalIgnoreCase) || msg.Contains("failed"))
        {
            dir = "ERR";
            lvl = LogLevel.Error;
        }
        else if (msg.StartsWith("[STEP", StringComparison.OrdinalIgnoreCase))
        {
            dir = "STEP";
            lvl = LogLevel.Warning;
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
