using GsmTools.Desktop.Core;
using Xunit;

namespace GsmTools.Desktop.Tests.Unit;

public class DevConInfoParserTests
{
    [Fact]
    public void Parse_AuthenticDevConInfo_ExtractsAllFieldsCorrectly()
    {
        string raw = "+DEVCONINFO: MN(SM-G965F);BASE(UNKNOWN);VER(G965FXXU2CSA1/G965FOXM2CSA1/G965FXXU2CRLI);HIDVER(G965FXXU2CSA1);MNC(01);MCC(421);PRD(XEU);SN(R38HC09NWMR);IMEI(354003081234567);UN(9887BC453678901);LOCK(NONE);LIMIT(FALSE);SDP(RUNTIME);HVID(0x1234)\r\nOK";

        var parsed = DevConInfoParser.Parse(raw);

        Assert.Equal("SM-G965F", parsed.Model);
        Assert.Equal("UNKNOWN", parsed.Baseband);
        Assert.Equal("G965FXXU2CSA1/G965FOXM2CSA1/G965FXXU2CRLI", parsed.FirmwareVersion);
        Assert.Equal("01", parsed.Mnc);
        Assert.Equal("421", parsed.Mcc);
        Assert.Equal("XEU", parsed.ProductCode);
        Assert.Equal("R38HC09NWMR", parsed.SerialNumber);
        Assert.Equal("354003081234567", parsed.Imei);
        Assert.Equal("9887BC453678901", parsed.UniqueNumber);
        Assert.Equal("NONE", parsed.LockStatus);
        Assert.Equal("RUNTIME", parsed.SdpState);
        Assert.Equal("0x1234", parsed.HardwareId);
    }

    [Fact]
    public void Parse_LockedSamsungDevice_ReportsLockStatusCorrectly()
    {
        string raw = "+DEVCONINFO: MN(SM-S918U);BASE(S918U);VER(S918USQS6DYG1);MNC(410);MCC(310);PRD(ATT);SN(RFCX90GR21X);IMEI(355752920542633);LOCK(LOCKED);";

        var parsed = DevConInfoParser.Parse(raw);

        Assert.Equal("SM-S918U", parsed.Model);
        Assert.Equal("ATT", parsed.ProductCode);
        Assert.Equal("LOCKED", parsed.LockStatus);
        Assert.Equal("RFCX90GR21X", parsed.SerialNumber);
        Assert.Equal("355752920542633", parsed.Imei);
    }

    [Theory]
    [InlineData("")]
    [InlineData("   ")]
    [InlineData(null)]
    public void Parse_EmptyOrNullInput_ReturnsDefaultsWithoutThrowing(string? raw)
    {
        var parsed = DevConInfoParser.Parse(raw!);

        Assert.Equal("Unknown", parsed.Model);
        Assert.Equal("Unknown", parsed.SerialNumber);
        Assert.Equal("Unknown", parsed.Imei);
        Assert.Equal("NONE", parsed.LockStatus);
    }
}
