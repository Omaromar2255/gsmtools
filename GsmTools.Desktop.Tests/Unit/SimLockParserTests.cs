using GsmTools.Desktop.Core;
using Xunit;

namespace GsmTools.Desktop.Tests.Unit;

public class SimLockParserTests
{
    [Fact]
    public void Parse_UnlockedSimResponses_ReportsUnlockedState()
    {
        string cpin = "+CPIN: READY\r\nOK";
        string pn = "+CLCK: 0\r\nOK";
        string pu = "+CLCK: 0\r\nOK";
        string pp = "+CLCK: 0\r\nOK";
        string pc = "+CLCK: 0\r\nOK";
        string sc = "+CLCK: 0\r\nOK";

        var res = SimLockParser.ParseFromResponses(cpin, pn, pu, pp, pc, sc);

        Assert.Equal("READY", res.CpinState);
        Assert.Equal("UNLOCKED (0)", res.NetworkLock);
        Assert.Equal("UNLOCKED (0)", res.NetworkSubsetLock);
        Assert.Equal("UNLOCKED (0)", res.ServiceProviderLock);
        Assert.Equal("UNLOCKED (0)", res.CorporateLock);
        Assert.Equal("UNLOCKED (0)", res.SimPinLock);
        Assert.True(res.IsFullyUnlocked);
        Assert.Equal("Device Unlocked", res.Summary);
    }

    [Fact]
    public void Parse_LockedNetworkState_ReportsLockedState()
    {
        string cpin = "+CPIN: READY\r\nOK";
        string pn = "+CLCK: 1\r\nOK";

        var res = SimLockParser.ParseFromResponses(cpin, pn);

        Assert.Equal("READY", res.CpinState);
        Assert.Equal("LOCKED (1)", res.NetworkLock);
        Assert.False(res.IsFullyUnlocked);
        Assert.Equal("Network Locked", res.Summary);
    }

    [Fact]
    public void ParseFromBridgePipeString_ParsesAllFieldsCorrectly()
    {
        string line = "SIMLOCK|CPIN=READY|PN=UNLOCKED(0)|PU=UNLOCKED(0)|PP=UNLOCKED(0)|PC=UNLOCKED(0)|SC=UNLOCKED(0)";

        var res = SimLockParser.ParseFromBridgePipeString(line);

        Assert.Equal("READY", res.CpinState);
        Assert.Equal("UNLOCKED(0)", res.NetworkLock);
        Assert.True(res.IsFullyUnlocked);
    }
}
