#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define FB_PATH "/dev/fb0"
#define PORT 8000

// Function to handle the incoming frame data and write it to the framebuffer
void write_frame_to_fb(unsigned char *frame_data, size_t frame_size, unsigned char *framebuffer, size_t screensize) {
    if (frame_size != screensize) {
        fprintf(stderr, "Frame size does not match framebuffer size.\n");
        return;
    }
    memcpy(framebuffer, frame_data, frame_size);
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

    // Set up the server
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Error creating socket");
        munmap(framebuffer, screensize);
        close(fb);
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error binding socket");
        close(server_fd);
        munmap(framebuffer, screensize);
        close(fb);
        return 1;
    }

    if (listen(server_fd, 1) < 0) {
        perror("Error listening on socket");
        close(server_fd);
        munmap(framebuffer, screensize);
        close(fb);
        return 1;
    }

    printf("Server listening on port %d...\n", PORT);

    // Buffer for receiving data
    unsigned char *frame_data = (unsigned char *)malloc(screensize);
    if (!frame_data) {
        perror("Error allocating memory for frame data");
        close(server_fd);
        munmap(framebuffer, screensize);
        close(fb);
        return 1;
    }

    // Main loop
    while (1) {
        // Accept incoming connections
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) {
            perror("Error accepting connection");
            continue; // Continue to the next iteration of the loop
        }

        printf("Client connected.\n");

        // Receive and display frames
        ssize_t bytes_received;
        while ((bytes_received = recv(client_fd, frame_data, screensize, 0)) > 0) {
            write_frame_to_fb(frame_data, bytes_received, framebuffer, screensize);
        }

        if (bytes_received < 0) {
            perror("Error receiving data");
        }

        printf("Client disconnected.\n");
        close(client_fd);
    }

    // Cleanup
    free(frame_data);
    close(server_fd);
    munmap(framebuffer, screensize);
    close(fb);
    return 0;
}

