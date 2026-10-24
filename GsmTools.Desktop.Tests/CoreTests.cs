using System.Text;
using GsmTools.Desktop.Core;
using Xunit;

namespace GsmTools.Desktop.Tests;

public class CoreTests
{
    [Fact]
    public void ImeiParcelParser_Matches_Csc15Txt_ForensicSample()
    {
        string sampleDump =
@"Result: Parcel(	
0x00000000: 00000000 0000000f 00350033 00370035 '........3.5.5.7.'
0x00000010: 00320035 00320039 00350030 00320034 '5.2.9.2.0.5.4.2.'
0x00000020: 00330036 00000033                   '6.3.3...        ')";

        string imei = ImeiParcelParser.ParseImeiFromParcelDump(sampleDump);

        Assert.Equal("355752920542633", imei);
    }

    [Fact]
    public void Xorshift32_SeedZero_LeavesBufferUnchanged()
    {
        byte[] original = Encoding.ASCII.GetBytes("0601000e00005841410000584141");
        byte[] transformed = Xorshift32StreamCipher.Transform(original, 0);

        Assert.Equal(original, transformed);
    }

    [Fact]
    public void Xorshift32_WithSeed_IsInvertibleSymmetricCipher()
    {
        byte[] original = Encoding.ASCII.GetBytes("0601000e00005841410000584141");
        uint seed = 0x93E4D628;

        byte[] encrypted = Xorshift32StreamCipher.Transform(original, seed);
        Assert.NotEqual(original, encrypted);

        byte[] decrypted = Xorshift32StreamCipher.Transform(encrypted, seed);
        Assert.Equal(original, decrypted);
    }

    [Fact]
    public void CscInjectionFrames_FormatMatches_Csc15Txt()
    {
        string csc = "XAA";
        string hexCsc = Convert.ToHexString(Encoding.ASCII.GetBytes(csc));
        Assert.Equal("584141", hexCsc);

        string f1 = $"0601000e0000{hexCsc}0000{hexCsc}";
        string f2 = $"060100090000{hexCsc}";
        string f3 = $"060100120000{hexCsc}0000{hexCsc}{hexCsc}";
        string f4 = "060100120000";
        string f5 = $"060100090000{hexCsc}";

        // Matching line 468 in csc 15.txt
        Assert.Equal("0601000e00005841410000584141", f1);
        // Matching line 479 in csc 15.txt
        Assert.Equal("060100090000584141", f2);
        // Matching line 490 in csc 15.txt
        Assert.Equal("0601001200005841410000584141584141", f3);
        // Matching line 501 in csc 15.txt
        Assert.Equal("060100120000", f4);
        // Matching line 512 in csc 15.txt
        Assert.Equal("060100090000584141", f5);
    }
}
