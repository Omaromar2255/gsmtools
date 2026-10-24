using System.Text;
using GsmTools.Desktop.Core;
using Xunit;

namespace GsmTools.Desktop.Tests.Unit;

public class XorshiftCipherTests
{
    [Fact]
    public void TransformBuffer_WithKnownSeed_MatchesMathematicalKeystream()
    {
        // Derived from Marsaglia [13, 17, 5] specification in spec_miner_survey_1
        // For seed = 0x12345678, the first 5 keystream bytes are:
        // k0 = 0xA5, k1 = 0x03, k2 = 0x1E, k3 = 0xD4, k4 = 0xFE
        uint seed = 0x12345678;
        byte[] input = new byte[5]; // all zeros XORed with keystream yields keystream
        byte[] expectedKeystream = new byte[] { 165, 163, 196, 152, 136 };

        Xorshift32StreamCipher.TransformBuffer(input, seed);

        Assert.Equal(expectedKeystream, input);
    }

    [Fact]
    public void Transform_Roundtrip_SymmetricInversion()
    {
        uint seed = 0xA1B2C3D4;
        byte[] original = Encoding.UTF8.GetBytes("0601000e00005841410000584141");

        byte[] encrypted = Xorshift32StreamCipher.Transform(original, seed);
        Assert.NotEqual(original, encrypted);

        byte[] decrypted = Xorshift32StreamCipher.Transform(encrypted, seed);
        Assert.Equal(original, decrypted);
    }

    [Fact]
    public void Transform_WithZeroSeed_BypassesEncryption()
    {
        uint seed = 0;
        byte[] original = new byte[] { 0x01, 0x02, 0x03, 0x04, 0x05 };
        byte[] transformed = Xorshift32StreamCipher.Transform(original, seed);

        Assert.Equal(original, transformed);
    }

    [Fact]
    public void TransformBuffer_WithEmptyBuffer_DoesNotThrow()
    {
        byte[] empty = Array.Empty<byte>();
        Xorshift32StreamCipher.TransformBuffer(empty, 0x12345678);
        Assert.Empty(empty);

        byte[]? nullBuf = null;
        Xorshift32StreamCipher.TransformBuffer(nullBuf!, 0x12345678);
    }

    [Fact]
    public void TransformBuffer_WithOffsetAndLength_TransformsOnlySlice()
    {
        uint seed = 0x12345678;
        byte[] buffer = new byte[] { 0xFF, 0x00, 0x00, 0x00, 0xFF };

        // Transform only index 1..3 (length 3)
        Xorshift32StreamCipher.TransformBuffer(buffer, 1, 3, seed);

        Assert.Equal(0xFF, buffer[0]); // Untouched
        Assert.Equal(165, buffer[1]);  // First keystream byte
        Assert.Equal(163, buffer[2]);  // Second keystream byte
        Assert.Equal(196, buffer[3]);  // Third keystream byte
        Assert.Equal(0xFF, buffer[4]); // Untouched
    }

    [Theory]
    [InlineData(0x00000001u)]
    [InlineData(0x7FFFFFFFu)]
    [InlineData(0x80000000u)] // MSB set (test logical shift)
    [InlineData(0xFFFFFFFFu)]
    public void Transform_EdgeCaseSeeds_PreservesLengthAndReversibility(uint seed)
    {
        byte[] data = Encoding.ASCII.GetBytes("AT_BRIDGE_CSC_PAYLOAD_TEST");
        byte[] enc = Xorshift32StreamCipher.Transform(data, seed);
        Assert.Equal(data.Length, enc.Length);

        byte[] dec = Xorshift32StreamCipher.Transform(enc, seed);
        Assert.Equal(data, dec);
    }

    [Fact]
    public void TransformBuffer_PerPacketSeedReset_ProducesIdenticalKeystream()
    {
        uint seed = 0x55AA55AA;
        byte[] packet1 = new byte[4];
        byte[] packet2 = new byte[4];

        Xorshift32StreamCipher.TransformBuffer(packet1, seed);
        Xorshift32StreamCipher.TransformBuffer(packet2, seed);

        Assert.Equal(packet1, packet2);
    }
}
