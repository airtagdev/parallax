// Original reference harness. Calls the installed JDK; not part of the C runtime.
import java.util.Random;
public class RandomOracle {
    public static void main(String[] args) {
        for (int i=0;i<4096;i++) {
            long seed = i==0?0:i==1?-1:i==2?Long.MIN_VALUE:i==3?Long.MAX_VALUE:(long)i*0x9e3779b97f4a7c15L;
            Random r = new Random(seed);
            System.out.printf("%d %d %d %d %08x %016x%n", seed, r.nextInt(), r.nextInt(1073741825), r.nextLong(), Float.floatToRawIntBits(r.nextFloat()), Double.doubleToRawLongBits(r.nextDouble()));
        }
    }
}
