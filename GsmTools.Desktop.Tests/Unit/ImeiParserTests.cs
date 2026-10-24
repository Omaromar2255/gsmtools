using GsmTools.Desktop.Core;
using Xunit;

namespace GsmTools.Desktop.Tests.Unit;

public class ImeiParserTests
{
    [Fact]
    public void ParseImei_AuthenticCsc15Trace_ExtractsCorrect15DigitImei()
    {
        // Authentic log trace from csc 15.txt lines 39-42
        string parcelDump =
            "Result: Parcel(\n" +
            "0x00000000: 00000000 0000000f 00350033 00370035 '........3.5.5.7.'\n" +
            "0x00000010: 00320035 00320039 00350030 00320034 '5.2.9.2.0.5.4.2.'\n" +
            "0x00000020: 00330036 00000033                   '6.3.3...        ')";

        string imei = ImeiParcelParser.ParseImeiFromParcelDump(parcelDump);

        Assert.Equal("355752920542633", imei);
    }

    [Fact]
    public void ParseImei_WithWindowsCarriageReturns_ExtractsCorrectImei()
    {
        string parcelDump =
            "Result: Parcel(\r\n" +
            "0x00000000: 00000000 0000000f 00350033 00370035 '........3.5.5.7.'\r\n" +
            "0x00000010: 00320035 00320039 00350030 00320034 '5.2.9.2.0.5.4.2.'\r\n" +
            "0x00000020: 00330036 00000033                   '6.3.3...        ')\r\n";

        string imei = ImeiParcelParser.ParseImeiFromParcelDump(parcelDump);

        Assert.Equal("355752920542633", imei);
    }

    [Fact]
    public void ParseImei_FallbackToQuotedAscii_WhenHexHeaderCorrupted()
    {
        // When 32-bit words are corrupted/absent but quoted string exists
        string raw = "Result: '........3.5.5.7.''5.2.9.2.0.5.4.2.''6.3.3...        '";
        string imei = ImeiParcelParser.ParseImeiFromParcelDump(raw);

        Assert.Equal("355752920542633", imei);
    }

    [Theory]
    [InlineData("")]
    [InlineData("   ")]
    [InlineData(null)]
    public void ParseImei_EmptyOrNullInput_ReturnsEmptyString(string? input)
    {
        string imei = ImeiParcelParser.ParseImeiFromParcelDump(input!);
        Assert.Equal(string.Empty, imei);
    }

    [Fact]
    public void ParseImei_GarbageInput_ReturnsEmptyString()
    {
        string garbage = "Permission Denial: can't call getDeviceId\nException in thread main";
        string imei = ImeiParcelParser.ParseImeiFromParcelDump(garbage);

        Assert.Equal(string.Empty, imei);
    }

    [Fact]
    public void ParseImei_CustomImeiSample_DecodesAccurately()
    {
        // Construct parcel for IMEI 864502041234567 (15 chars)
        // Length = 0x0000000F (15)
        // '8' (0x38), '6' (0x36) -> word: 0x00360038
        // '4' (0x34), '5' (0x35) -> word: 0x00350034
        // '0' (0x30), '2' (0x32) -> word: 0x00320030
        // '0' (0x30), '4' (0x34) -> word: 0x00340030
        // '1' (0x31), '2' (0x32) -> word: 0x00320031
        // '3' (0x33), '4' (0x34) -> word: 0x00340033
        // '5' (0x35), '6' (0x36) -> word: 0x00360035
        // '7' (0x37), '\0'       -> word: 0x00000037
        string dump =
            "Result: Parcel(\n" +
            "0x00000000: 00000000 0000000f 00360038 00350034 '........8.6.4.5.'\n" +
            "0x00000010: 00320030 00340030 00320031 00340033 '0.2.0.4.1.2.3.4.'\n" +
            "0x00000020: 00360035 00000037                   '5.6.7...        ')\n";

        string imei = ImeiParcelParser.ParseImeiFromParcelDump(dump);
        Assert.Equal("864502041234567", imei);
    }
}
