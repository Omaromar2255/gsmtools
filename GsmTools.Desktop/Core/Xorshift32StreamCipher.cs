namespace GsmTools.Desktop.Core;

/// <summary>
/// Implements Marsaglia [13, 17, 5] Xorshift32 stream cipher, exactly matching transform_buffer in at_bridge.c.
/// Used for zero-loss obfuscation of AT/IPC communications across ADB transport.
/// </summary>
public static class Xorshift32StreamCipher
{
    public static void TransformBuffer(byte[] buffer, int offset, int length, uint seed)
    {
        if (seed == 0 || buffer == null || length <= 0) return;
        uint state = seed;
        int end = Math.Min(buffer.Length, offset + length);
        for (int i = offset; i < end; i++)
        {
            state ^= state << 13;
            state ^= state >> 17;
            state ^= state << 5;
            buffer[i] ^= (byte)(state & 0xFF);
        }
    }

    public static void TransformBuffer(byte[] buffer, uint seed)
    {
        if (buffer == null) return;
        TransformBuffer(buffer, 0, buffer.Length, seed);
    }

    public static byte[] Transform(byte[] data, uint seed)
    {
        if (data == null) return Array.Empty<byte>();
        byte[] copy = (byte[])data.Clone();
        TransformBuffer(copy, seed);
        return copy;
    }
}
