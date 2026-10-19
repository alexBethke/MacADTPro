/* GPL-2.0-or-later. Packet/CRC body copied from ADTPro 2.1.0
 * CommsThread.java, Copyright 2006-2020 David Schmidt.
 * Test harness replaces only the transport; no serial port is opened.
 */
import java.io.*;
public class UpstreamWideReference {
    static class UnsignedByte {
        static byte loByte(int v) { return (byte)v; }
        static byte hiByte(int v) { return (byte)(v >> 8); }
        static int intValue(byte v) { return v & 255; }
    }
    static class Transport {
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        void writeByte(byte b) { out.write(b); }
        void pushBuffer() {}
    }
    boolean _shouldRun = true;
    int[] CRCTABLE = new int[256];
    Transport _transport = new Transport();
    byte[] encode(byte[] buffer, int block, int blocksAtOnce) {
        boolean rc = false;
        int byteCount = 0, crc;
        byte data, prev, newprev, check = 0;
        _transport.out.reset();
			prev = 0;
			_transport.writeByte(UnsignedByte.loByte(0xc1)); // 'A'
			check = UnsignedByte.loByte(0xc1);
			_transport.writeByte(UnsignedByte.loByte(blocksAtOnce * 512)); // Bytes after RLE decompression lsb
			check ^= UnsignedByte.loByte(blocksAtOnce * 512);
			_transport.writeByte(UnsignedByte.hiByte(blocksAtOnce * 512)); // Bytes after RLE decompression msb
			check ^= UnsignedByte.hiByte(blocksAtOnce * 512);
			_transport.writeByte(UnsignedByte.loByte(0xd3)); // 'S'
			check ^= UnsignedByte.loByte(0xd3);
			_transport.writeByte(UnsignedByte.loByte(check)); // Envelope check byte
			_transport.writeByte(UnsignedByte.loByte(block)); // Starting block number lsb
			_transport.writeByte(UnsignedByte.hiByte(block)); // Starting block number msb
			int bcOffset = 0;
			for (int pageCount = 0; pageCount < blocksAtOnce * 2; pageCount++)
			{
				for (byteCount = 0; byteCount < 256;)
				{
					newprev = buffer[bcOffset + byteCount];
					data = (byte) (UnsignedByte.intValue(newprev) - UnsignedByte.intValue(prev));
					prev = newprev;
					_transport.writeByte(data);
					if (UnsignedByte.intValue(data) > 0)
						byteCount++;
					else
					{
						while ((_shouldRun == true) && byteCount < 256 && buffer[bcOffset + byteCount] == newprev)
						{
							byteCount++;
						}
						_transport.writeByte((byte) (byteCount & 0xFF)); // 256 becomes 0
					}
					if (!_shouldRun)
					{
						rc = false;
						break;
					}
				}
				bcOffset += byteCount;
			}
			if (_shouldRun)
			{
				crc = doCrc(buffer, 0, (blocksAtOnce * 512));
				_transport.writeByte((byte) (crc & 0xff));
				_transport.writeByte((byte) (((crc & 0xff00) >> 8) & 0xff));
				_transport.pushBuffer();
				// Pull a real ack packet
        }
        return _transport.out.toByteArray();
    }
	public int doCrc(byte[] buffer, int offset, int count)
	{
		/* Return the CRC of ptr[0]..ptr[count-1] */
		int crc = 0;
		for (int i = 0; i < count; i++)
		{
			crc = ((crc << 8) & 0xff00) ^ (CRCTABLE[(((crc & 0xff00) >> 8) ^ buffer[offset + i]) & 0xff] & 0xffff);
		}
		return crc;
	}

	void makeCrcTable()
	/* Fill the crctable[] array needed by doCrc */
	{
		int oneByte, bit;
		int crc;

		for (oneByte = 0; oneByte < 256; oneByte++)
		{
			crc = oneByte << 8;
			for (bit = 0; bit < 8; bit++)
			{
				crc = (((crc & 0x8000) != 0) ? (crc << 1) ^ 0x1021 : crc << 1);
			}
			CRCTABLE[oneByte] = crc;
		}
	}


    public static void main(String[] args) throws Exception {
        UpstreamWideReference ref = new UpstreamWideReference();
        ref.makeCrcTable();
        try (DataOutputStream out = new DataOutputStream(new FileOutputStream(args[0]))) {
            for (int pattern = 0; pattern < 3; ++pattern) {
                for (int blocks : new int[]{1, 2, 10, 17, 40}) {
                    byte[] data = new byte[blocks * 512];
                    for (int i = 0; i < data.length; ++i)
                        data[i] = (byte)(pattern == 0 ? 0 : pattern == 1 ? (i / 13) ^ (i * 7) : i < 512 ? i : 0);
                    byte[] packet = ref.encode(data, 280 - blocks, blocks);
                    out.writeInt(pattern); out.writeInt(blocks); out.writeInt(packet.length); out.write(packet);
                }
            }
        }
    }
}
