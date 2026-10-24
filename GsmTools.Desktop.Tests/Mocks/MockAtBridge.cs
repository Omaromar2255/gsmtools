using System.Buffers.Binary;
using System.Net.Sockets;
using System.Text;
using System.Text.RegularExpressions;
using GsmTools.Desktop.Core;

namespace GsmTools.Desktop.Tests.Mocks;

/// <summary>
/// In-memory simulator of the custom native C binary 'at_bridge.c'.
/// Simulates compact startup banner, interactive prompt, dynamic AUTH session encryption (Xorshift32),
/// IPC command execution (CSC 5-frame sequence), delimiters (.OK..#), and graceful teardown.
/// </summary>
public class MockAtBridge
{
    public string Model { get; set; } = "SM-S918U";
    public string Csc { get; set; } = "ATT";
    public string AndroidVersion { get; set; } = "15";
    public string SecurityPatch { get; set; } = "2025-07-01";
    public string DebugLevel { get; set; } = "0x4f4c";
    public int Uid { get; set; } = 1000;
    public string ActiveChannel { get; set; } = "/dev/socket/rild";
    public uint SessionSeed { get; set; } = 0;

    public List<string> ReceivedIpcCommands { get; } = new();
    public List<string> ReceivedRawLines { get; } = new();

    public Func<string, (bool Success, string? HexResponse)>? CustomIpcHandler { get; set; }
    public bool SimulateIpcError { get; set; }
    public bool SimulateTimeout { get; set; }

    public string BuildBanner()
    {
        return $"INFO|MODEL={Model}|CSC={Csc}|AND={AndroidVersion}|PATCH={SecurityPatch}|DBG={DebugLevel}|UID={Uid}|CH={ActiveChannel}\n";
    }

    public async Task RunSessionAsync(NetworkStream stream, CancellationToken cancellationToken = default)
    {
        // 1. Send banner + prompt
        string banner = BuildBanner();
        byte[] bannerBytes = Encoding.UTF8.GetBytes(banner);
        await stream.WriteAsync(bannerBytes, cancellationToken);

        byte[] promptBytes = Encoding.ASCII.GetBytes("<< ");
        await stream.WriteAsync(promptBytes, cancellationToken);
        await stream.FlushAsync(cancellationToken);

        // 2. Interactive command loop
        using var reader = new StreamReader(stream, Encoding.UTF8, leaveOpen: true);
        using var writer = new StreamWriter(stream, Encoding.UTF8, leaveOpen: true) { AutoFlush = true };

        while (!cancellationToken.IsCancellationRequested)
        {
            string? line = await reader.ReadLineAsync(cancellationToken);
            if (line == null) break;

            line = line.Trim();
            if (string.IsNullOrEmpty(line))
            {
                await writer.WriteAsync("<< ");
                continue;
            }

            ReceivedRawLines.Add(line);

            if (string.Equals(line, "EXIT", StringComparison.OrdinalIgnoreCase) ||
                string.Equals(line, "QUIT", StringComparison.OrdinalIgnoreCase))
            {
                break;
            }

            if (line.StartsWith("/data/local/tmp/AT", StringComparison.OrdinalIgnoreCase))
            {
                Uid = 1000;
                ActiveChannel = "/dev/socket/rild";
                continue;
            }

            if (string.Equals(line, "INFO", StringComparison.OrdinalIgnoreCase))
            {
                await writer.WriteAsync(BuildBanner());
                await writer.WriteAsync("<< ");
                continue;
            }

            if (line.StartsWith("AUTH:", StringComparison.OrdinalIgnoreCase))
            {
                string seedHex = line.Substring(5).Trim();
                if (uint.TryParse(seedHex, System.Globalization.NumberStyles.HexNumber, null, out uint parsedSeed))
                {
                    SessionSeed = parsedSeed;
                }
                else
                {
                    SessionSeed = 0;
                }

                await writer.WriteAsync(".OK_AUTH..\n<< ");
                continue;
            }

            if (line.StartsWith("MODE:AT", StringComparison.OrdinalIgnoreCase))
            {
                ActiveChannel = "/dev/umts_atc0";
                await writer.WriteAsync($".CH={ActiveChannel}..\n<< ");
                continue;
            }

            if (line.StartsWith("MODE:IPC", StringComparison.OrdinalIgnoreCase))
            {
                ActiveChannel = "/dev/umts_ipc0";
                await writer.WriteAsync($".CH={ActiveChannel}..\n<< ");
                continue;
            }

            if (line.StartsWith("AT:", StringComparison.OrdinalIgnoreCase))
            {
                if (SimulateTimeout)
                {
                    await writer.WriteAsync(".TIMEOUT..\n<< ");
                }
                else
                {
                    await writer.WriteAsync("OK\n<< ");
                }
                continue;
            }

            // IPC Hex Command Handling
            if (SimulateIpcError)
            {
                await writer.WriteAsync(".ERR_IPC..\n<< ");
                continue;
            }

            // Verify Hex Syntax
            if (!Regex.IsMatch(line, "^[0-9A-Fa-f]+$"))
            {
                await writer.WriteAsync(".ERR_SYNTAX..\n<< ");
                continue;
            }

            byte[] payloadBytes;
            try
            {
                payloadBytes = Convert.FromHexString(line);
            }
            catch
            {
                await writer.WriteAsync(".ERR_SYNTAX..\n<< ");
                continue;
            }

            // Decrypt if encrypted session active
            if (SessionSeed > 0)
            {
                Xorshift32StreamCipher.TransformBuffer(payloadBytes, SessionSeed);
            }

            string decryptedHex = Convert.ToHexString(payloadBytes);
            ReceivedIpcCommands.Add(decryptedHex);

            if (CustomIpcHandler != null)
            {
                var (success, hexResp) = CustomIpcHandler(decryptedHex);
                if (!success)
                {
                    await writer.WriteAsync(".ERR_IPC..\n<< ");
                }
                else if (string.IsNullOrEmpty(hexResp))
                {
                    await writer.WriteAsync(".OK..#\n<< ");
                }
                else
                {
                    byte[] respBytes = Convert.FromHexString(hexResp);
                    if (SessionSeed > 0)
                    {
                        Xorshift32StreamCipher.TransformBuffer(respBytes, SessionSeed);
                    }
                    string outHex = Convert.ToHexString(respBytes);
                    await writer.WriteAsync($".OK..{outHex}#\n<< ");
                }
                continue;
            }

            // Default: successful acknowledgment .OK..# matching csc 15.txt
            await writer.WriteAsync(".OK..#\n<< ");
        }
    }
}
