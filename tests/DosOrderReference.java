import java.io.FileOutputStream;
import com.webcodepro.applecommander.storage.physical.ByteArrayImageLayout;
import com.webcodepro.applecommander.storage.physical.DosOrder;

/* Reference uses the actual library bundled with ADTPro 2.1.0. */
public class DosOrderReference {
    public static void main(String[] args) throws Exception {
        byte[] data = new byte[143360];
        for (int track = 0; track < 35; ++track)
            for (int sector = 0; sector < 16; ++sector)
                for (int i = 0; i < 256; ++i)
                    data[track * 4096 + sector * 256 + i] = (byte)(track * 17 + sector * 11 + i);
        DosOrder order = new DosOrder(new ByteArrayImageLayout(data));
        try (FileOutputStream out = new FileOutputStream(args[0])) {
            for (int block = 0; block < 280; ++block) out.write(order.readBlock(block));
        }
    }
}
