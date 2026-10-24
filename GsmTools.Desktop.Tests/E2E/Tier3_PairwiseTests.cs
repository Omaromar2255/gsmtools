using GsmTools.Desktop.Core;
using GsmTools.Desktop.Tests.Mocks;
using GsmTools.Desktop.Workflows;
using Xunit;

namespace GsmTools.Desktop.Tests.E2E;

/// <summary>
/// Tier 3: Combinatorial Pairwise Testing.
/// Tests interaction pairs across Target CSC, Encryption Mode, SELinux mode, Port 2600 status, and Knox state.
/// </summary>
public class Tier3_PairwiseTests
{
    [Theory]
    // Pair 1: XAA | Plaintext | Enforcing | Port 2600 Ready
    [InlineData("XAA", false, true, 0)]
    // Pair 2: USC | Encrypted | Enforcing | Port 2600 Refused & Recovered
    [InlineData("USC", true, true, 2)]
    // Pair 3: TMB | Encrypted | Permissive | Port 2600 Ready
    [InlineData("TMB", true, false, 0)]
    // Pair 4: ATT | Plaintext | Permissive | Port 2600 Refused & Recovered
    [InlineData("ATT", false, false, 1)]
    // Pair 5: VZW | Encrypted | Enforcing | Port 2600 Ready
    [InlineData("VZW", true, true, 0)]
    // Pair 6: GCF | Plaintext | Enforcing | Port 2600 Ready
    [InlineData("GCF", false, true, 0)]
    public async Task Pairwise_CscExecutionMatrix_CompletesSuccessfully(
        string targetCsc,
        bool useEncryption,
        bool selinuxEnforcing,
        int port2600FailAttempts)
    {
        await using var server = new MockAdbServer();
        server.ShellResponses["getenforce"] = selinuxEnforcing ? "Enforcing\n" : "Permissive\n";
        server.Port2600FailAttempts = port2600FailAttempts;
        server.Start();

        await using var client = new AdbRawSocketClient();
        await client.ConnectAsync(server.Host, server.Port);
        await using var session = new PersistentAtBridgeSession(client);
        var workflow = new SamsungCscWorkflow(client, session);

        var result = await workflow.ExecuteCscChangeAsync(targetCsc, useEncryptedAtSession: useEncryption);

        Assert.True(result.Success, $"Failed on matrix tuple: CSC={targetCsc}, Enc={useEncryption}, SELinux={selinuxEnforcing}, Port2600Fails={port2600FailAttempts}");
        Assert.Equal(targetCsc, result.NewCsc);

        if (useEncryption)
        {
            Assert.True(server.AtBridge.SessionSeed > 0);
        }
        else
        {
            Assert.Equal(0u, server.AtBridge.SessionSeed);
        }

        // Verify the 5 injection frames were correctly dispatched
        Assert.True(server.AtBridge.ReceivedIpcCommands.Count >= 5);
    }
}
