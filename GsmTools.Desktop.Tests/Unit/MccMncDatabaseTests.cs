using GsmTools.Desktop.Core;
using Xunit;

namespace GsmTools.Desktop.Tests.Unit;

public class MccMncDatabaseTests
{
    [Theory]
    [InlineData("421", "01", "421F0100")] // Sabafon Yemen
    [InlineData("421", "02", "421F0200")] // MTN / YOU Yemen
    [InlineData("421", "03", "421F0300")] // Yemen Mobile
    [InlineData("421", "04", "421F0400")] // Y Telecom Yemen
    [InlineData("310", "120", "31012000")] // Sprint USA (3-digit MNC)
    [InlineData("310", "260", "31026000")] // T-Mobile USA
    [InlineData("450", "05", "450F0500")] // SK Telecom South Korea
    [InlineData("420", "1", "420F0100")]  // Single digit MNC padded to 2 digits
    public void EncodeMccMncToBcd_EncodesCorrectly(string mcc, string mnc, string expectedBcd)
    {
        string actualBcd = MccMncDatabase.EncodeMccMncToBcd(mcc, mnc);
        Assert.Equal(expectedBcd, actualBcd);
    }

    [Fact]
    public void Search_ByCountry_ReturnsMatchingCarriers()
    {
        var results = MccMncDatabase.Search("Yemen");
        Assert.NotEmpty(results);
        Assert.Contains(results, r => r.Network.Contains("Sabafon", StringComparison.OrdinalIgnoreCase));
    }

    [Fact]
    public void Search_ByMcc_ReturnsMatchingCarriers()
    {
        var results = MccMncDatabase.Search("421");
        Assert.NotEmpty(results);
        Assert.All(results, r => Assert.Equal("421", r.Mcc));
    }
}
