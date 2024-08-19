import java.io.File;
import java.io.RandomAccessFile;
import java.io.IOException;

public class main {

    // Path to the framebuffer device
    private static final String FB_PATH = "/dev/fb0";

    // Width and height of the framebuffer
    private static int width = 800;   // Example width, adjust as needed
    private static int height = 600;  // Example height, adjust as needed
    private static int bpp = 3;       // Bytes per pixel (RGB888)

    public static void main(String[] args) {
        try (RandomAccessFile fb = new RandomAccessFile(new File(FB_PATH), "rw")) {
            // Main loop
            while (true) {
                // Cycle through colors (red, green, blue)
                for (int[] color : new int[][]{{255, 0, 0}, {0, 255, 0}, {0, 0, 255}}) {
                    byte[] colorBuffer = createColorBuffer(color[0], color[1], color[2]);

                    // Write color buffer to framebuffer
                    fb.seek(0);
                    fb.write(colorBuffer);

                    // Sleep for 40 milliseconds
                    Thread.sleep(40);
                }
            }
        } catch (IOException | InterruptedException e) {
            e.printStackTrace();
        }
    }

    // Function to generate color buffer with a given color
    private static byte[] createColorBuffer(int r, int g, int b) {
        int bufferSize = width * height * bpp;
        byte[] buffer = new byte[bufferSize];

        for (int i = 0; i < bufferSize; i += bpp) {
            buffer[i] = (byte) r;   // Red
            buffer[i + 1] = (byte) g; // Green
            buffer[i + 2] = (byte) b; // Blue
        }

        return buffer;
    }
}
