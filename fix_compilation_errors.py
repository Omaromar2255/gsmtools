from pathlib import Path

# 1. Update InteractiveAtWorkflow.cs
path_ia = Path('GsmTools.Desktop/Workflows/InteractiveAtWorkflow.cs')
content_ia = path_ia.read_text(encoding='utf-8')
old_line = 'var results = new List<(string, string, bool)>();'
new_line = 'var results = new List<(string Command, string Response, bool Success)>();'
if old_line in content_ia:
    content_ia = content_ia.replace(old_line, new_line)
    path_ia.write_text(content_ia, encoding='utf-8')
    print('Updated InteractiveAtWorkflow.cs')
else:
    print('Warning: old_line not found in InteractiveAtWorkflow.cs')

# 2. Update ModemSerialPortClient.cs
path_mp = Path('GsmTools.Desktop/Core/ModemSerialPortClient.cs')
content_mp = path_mp.read_text(encoding='utf-8')

# Update ModemPortInfo
old_record = '''public record ModemPortInfo(string PortName, string Description)
{
    public string DisplayName => string.IsNullOrWhiteSpace(Description) ? PortName : $"{PortName} - {Description}";
    public override string ToString() => DisplayName;
}'''

new_record = '''public record ModemPortInfo(string PortName, string Description)
{
    public bool IsSamsungModem =>
        Description.Contains("SAMSUNG", StringComparison.OrdinalIgnoreCase) ||
        Description.Contains("Modem", StringComparison.OrdinalIgnoreCase);

    public string DisplayName => string.IsNullOrWhiteSpace(Description) ? PortName : $"{PortName} - {Description}";
    public override string ToString() => DisplayName;
}'''

# Handle CRLF vs LF
content_mp_norm = content_mp.replace('\r\n', '\n')
if old_record in content_mp_norm:
    content_mp_norm = content_mp_norm.replace(old_record, new_record)
else:
    print('Warning: old_record not found in ModemSerialPortClient.cs')

# Update interface Connect
content_mp_norm = content_mp_norm.replace(
    'void Connect(string portName, int baudRate = 115200, int timeoutMs = 3000);',
    'bool Connect(string portName, int baudRate = 115200, int timeoutMs = 3000);'
)

# Update class Connect
old_connect = '''    public void Connect(string portName, int baudRate = 115200, int timeoutMs = 3000)
    {
        lock (_lock)
        {
            Disconnect();

            _serialPort = new SerialPort(portName, baudRate, Parity.None, 8, StopBits.One)
            {
                ReadTimeout = timeoutMs,
                WriteTimeout = timeoutMs,
                DtrEnable = true,
                RtsEnable = true,
                NewLine = "\\r\\n",
                Encoding = Encoding.ASCII
            };

            _serialPort.Open();
            _serialPort.DiscardInBuffer();
            _serialPort.DiscardOutBuffer();
        }
    }'''

new_connect = '''    public bool Connect(string portName, int baudRate = 115200, int timeoutMs = 3000)
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
                    NewLine = "\\r\\n",
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
    }'''

if old_connect in content_mp_norm:
    content_mp_norm = content_mp_norm.replace(old_connect, new_connect)
else:
    print('Warning: old_connect not found in ModemSerialPortClient.cs')

if '\r\n' in content_mp:
    content_mp_norm = content_mp_norm.replace('\n', '\r\n')

path_mp.write_text(content_mp_norm, encoding='utf-8')
print('Updated ModemSerialPortClient.cs')
