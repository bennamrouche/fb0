#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>

#define FB_PATH "/dev/fb0"

// Function to create a color buffer with a given color
void create_color_buffer(unsigned char *buffer, int r, int g, int b, int width, int height, int bpp) {
    size_t buffer_size = width * height * bpp;
    for (size_t i = 0; i < buffer_size; i += bpp) {
        buffer[i] = r;   // Red
        buffer[i + 1] = g; // Green
        buffer[i + 2] = b; // Blue
    }
}

int main() {
    int fb = open(FB_PATH, O_RDWR);
    if (fb == -1) {
        perror("Error opening framebuffer device");
        return 1;
    }

    // Get framebuffer variable information
    struct fb_var_screeninfo vinfo;
    if (ioctl(fb, FBIOGET_VSCREENINFO, &vinfo)) {
        perror("Error reading variable information");
        close(fb);
        return 1;
    }

    int width = vinfo.xres_virtual;
    int height = vinfo.yres_virtual;
    int bpp = vinfo.bits_per_pixel / 8;  // bytes per pixel

    // Framebuffer size
    size_t screensize = width * height * bpp;

    // Map framebuffer to memory
    unsigned char *framebuffer = mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0);
    if (framebuffer == MAP_FAILED) {
        perror("Error mapping framebuffer device to memory");
        close(fb);
        return 1;
    }

    unsigned char *color_buffer = (unsigned char *)malloc(screensize);
    if (!color_buffer) {
        perror("Error allocating memory for color buffer");
        munmap(framebuffer, screensize);
        close(fb);
        return 1;
    }

    // Seed the random number generator
    srand(time(NULL));

    // Main loop
    while (1) {
        // Generate random colors for each frame
        int r = rand() % 256; // Random red component (0-255)
        int g = rand() % 256; // Random green component (0-255)
        int b = rand() % 256; // Random blue component (0-255)

        create_color_buffer(color_buffer, r, g, b, width, height, bpp);

        // Write color buffer to framebuffer
        memcpy(framebuffer, color_buffer, screensize);

        // Sleep for 40 milliseconds
        usleep(40000);
    }

    // Cleanup
    free(color_buffer);
    munmap(framebuffer, screensize);
    close(fb);
    return 0;
}

