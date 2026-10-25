using System.IO.Ports;
using System.Text;
using Microsoft.Win32;

namespace GsmTools.Desktop.Core;

public record ModemPortInfo(string PortName, string Description)
{
    public bool IsSamsungModem =>
        Description.Contains("SAMSUNG", StringComparison.OrdinalIgnoreCase) ||
        Description.Contains("Modem", StringComparison.OrdinalIgnoreCase);

    public string DisplayName => string.IsNullOrWhiteSpace(Description) ? PortName : $"{PortName} - {Description}";
    public override string ToString() => DisplayName;
}

public interface IModemSerialClient : IDisposable
{
    bool IsConnected { get; }
    string? CurrentPort { get; }
    int BaudRate { get; }

    IReadOnlyList<ModemPortInfo> GetAvailablePorts();
    bool Connect(string portName, int baudRate = 115200, int timeoutMs = 3000);
    void Disconnect();

    Task<(bool Success, string Response)> SendAtCommandAsync(string command, int timeoutMs = 3000, CancellationToken ct = default);
    Task<List<(string Command, string Response, bool Success)>> ExecuteBatchCommandsAsync(
        IEnumerable<string> commands,
        int delayBetweenMs = 150,
        Action<string, string, bool>? onCommandExecuted = null,
        CancellationToken ct = default);
}

/// <summary>
/// Professional Modem Serial Port Client communicating directly with Samsung / Qualcomm / MediaTek
/// USB modems over physical COM ports (CDC ACM / USB Serial), bypassing ADB entirely.
/// </summary>
public class ModemSerialPortClient : IModemSerialClient
{
    private SerialPort? _serialPort;
    private readonly object _lock = new();

    public bool IsConnected => _serialPort != null && _serialPort.IsOpen;
    public string? CurrentPort => _serialPort?.PortName;
    public int BaudRate => _serialPort?.BaudRate ?? 115200;

    /// <summary>
    /// Scans the operating system for all available COM ports and identifies Samsung / Qualcomm modems.
    /// </summary>
    public IReadOnlyList<ModemPortInfo> GetAvailablePorts()
    {
        var result = new List<ModemPortInfo>();
        var friendlyMap = GetPortFriendlyNamesFromRegistry();

        string[] portNames;
        try
        {
            portNames = SerialPort.GetPortNames();
        }
        catch
        {
            portNames = Array.Empty<string>();
        }

        foreach (var port in portNames.Distinct(StringComparer.OrdinalIgnoreCase).OrderBy(p => p))
        {
            friendlyMap.TryGetValue(port, out string? desc);
            if (string.IsNullOrWhiteSpace(desc))
            {
                desc = port.Contains("COM", StringComparison.OrdinalIgnoreCase) ? "Serial Device" : "Modem Interface";
            }
            result.Add(new ModemPortInfo(port, desc));
        }

        return result;
    }

    private static Dictionary<string, string> GetPortFriendlyNamesFromRegistry()
    {
        var map = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        try
        {
            // Registry check 1: SERIALCOMM
            using var key = Registry.LocalMachine.OpenSubKey(@"HARDWARE\DEVICEMAP\SERIALCOMM");
            if (key != null)
            {
                foreach (var valName in key.GetValueNames())
                {
                    var portObj = key.GetValue(valName);
                    if (portObj is string portName && !string.IsNullOrWhiteSpace(portName))
                    {
                        string desc = valName.Contains("Modem", StringComparison.OrdinalIgnoreCase) || valName.Contains("Samsung", StringComparison.OrdinalIgnoreCase)
                            ? "SAMSUNG Mobile USB Modem"
                            : valName;
                        map[portName] = desc;
                    }
                }
            }
        }
        catch
        {
            // Ignore registry read restrictions
        }

        return map;
    }

    public bool Connect(string portName, int baudRate = 115200, int timeoutMs = 3000)
    {
        lock (_lock)
        {
            Disconnect();

            try
            {
                _serialPort = new SerialPort(portName, baudRate, Parity.None, 8, StopBits.One)
                {
                    ReadTimeout = timeoutMs,
                    WriteTimeout = timeoutMs,
                    DtrEnable = true,
                    RtsEnable = true,
                    NewLine = "\r\n",
                    Encoding = Encoding.ASCII
                };

                _serialPort.Open();
                _serialPort.DiscardInBuffer();
                _serialPort.DiscardOutBuffer();
                return _serialPort.IsOpen;
            }
            catch
            {
                _serialPort?.Dispose();
                _serialPort = null;
                return false;
            }
        }
    }

    public void Disconnect()
    {
        lock (_lock)
        {
            if (_serialPort != null)
            {
                try
                {
                    if (_serialPort.IsOpen)
                    {
                        _serialPort.DiscardInBuffer();
                        _serialPort.DiscardOutBuffer();
                        _serialPort.Close();
                    }
                }
                catch
                {
                    // Ignore close exceptions
                }
                finally
                {
                    _serialPort.Dispose();
                    _serialPort = null;
                }
            }
        }
    }

    public async Task<(bool Success, string Response)> SendAtCommandAsync(string command, int timeoutMs = 3000, CancellationToken ct = default)
    {
        return await Task.Run(() =>
        {
            lock (_lock)
            {
                if (!IsConnected || _serialPort == null)
                {
                    return (false, "ERROR: Modem COM port is not connected.");
                }

                try
                {
                    _serialPort.ReadTimeout = timeoutMs;
                    _serialPort.DiscardInBuffer();

                    string cleanCmd = (command ?? string.Empty).Trim();
                    _serialPort.Write($"{cleanCmd}\r\n");

                    var sb = new StringBuilder();
                    var deadline = DateTime.UtcNow.AddMilliseconds(timeoutMs);

                    while (DateTime.UtcNow < deadline)
                    {
                        ct.ThrowIfCancellationRequested();

                        try
                        {
                            string line = _serialPort.ReadLine();
                            if (!string.IsNullOrWhiteSpace(line))
                            {
                                string trimmed = line.Trim();
                                sb.AppendLine(trimmed);

                                if (trimmed.Equals("OK", StringComparison.OrdinalIgnoreCase))
                                {
                                    return (true, sb.ToString().Trim());
                                }
                                if (trimmed.StartsWith("ERROR", StringComparison.OrdinalIgnoreCase) ||
                                    trimmed.StartsWith("+CME ERROR", StringComparison.OrdinalIgnoreCase) ||
                                    trimmed.StartsWith("+CMS ERROR", StringComparison.OrdinalIgnoreCase))
                                {
                                    return (false, sb.ToString().Trim());
                                }
                            }
                        }
                        catch (TimeoutException)
                        {
                            break;
                        }
                    }

                    string finalOutput = sb.ToString().Trim();
                    bool hasOk = finalOutput.Contains("OK", StringComparison.OrdinalIgnoreCase);
                    return (hasOk, string.IsNullOrEmpty(finalOutput) ? ".TIMEOUT.." : finalOutput);
                }
                catch (Exception ex)
                {
                    return (false, $"EXCEPTION: {ex.Message}");
                }
            }
        }, ct);
    }

    public async Task<List<(string Command, string Response, bool Success)>> ExecuteBatchCommandsAsync(
        IEnumerable<string> commands,
        int delayBetweenMs = 150,
        Action<string, string, bool>? onCommandExecuted = null,
        CancellationToken ct = default)
    {
        var results = new List<(string, string, bool)>();

        foreach (var rawCmd in commands)
        {
            ct.ThrowIfCancellationRequested();

            string cmd = rawCmd?.Trim() ?? string.Empty;
            if (string.IsNullOrWhiteSpace(cmd) || cmd.StartsWith("#") || cmd.StartsWith("//"))
            {
                continue;
            }

            var (success, response) = await SendAtCommandAsync(cmd, 3500, ct);
            results.Add((cmd, response, success));
            onCommandExecuted?.Invoke(cmd, response, success);

            if (delayBetweenMs > 0)
            {
                await Task.Delay(delayBetweenMs, ct);
            }
        }

        return results;
    }

    public void Dispose()
    {
        Disconnect();
        GC.SuppressFinalize(this);
    }
}
