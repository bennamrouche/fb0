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

// Function prototypes
void handle_client(int client_fd, unsigned char *framebuffer, size_t screensize);
int setup_framebuffer(unsigned char **framebuffer, size_t *screensize);
int setup_server_socket(void);
void cleanup(int fb, int server_fd, unsigned char *framebuffer);

void handle_client(int client_fd, unsigned char *framebuffer, size_t screensize) {
    unsigned char *frame_data = (unsigned char *)malloc(screensize);
    if (!frame_data) {
        perror("Error allocating memory for frame data");
        close(client_fd);
        return;
    }

    ssize_t bytes_received;
    while ((bytes_received = recv(client_fd, frame_data, screensize, 0)) > 0) {
        if (bytes_received < (ssize_t)screensize) {
            fprintf(stderr, "Received incomplete frame.\n");
        } else {
            memcpy(framebuffer, frame_data, bytes_received);
            printf("Frame data written to framebuffer.\n");
        }
    }

    if (bytes_received < 0) {
        perror("Error receiving data");
    }

    free(frame_data);
    close(client_fd);
}

int setup_framebuffer(unsigned char **framebuffer, size_t *screensize) {
    int fb = open(FB_PATH, O_RDWR);
    if (fb == -1) {
        perror("Error opening framebuffer device");
        return -1;
    }

    struct fb_var_screeninfo vinfo;
    if (ioctl(fb, FBIOGET_VSCREENINFO, &vinfo)) {
        perror("Error reading variable information");
        close(fb);
        return -1;
    }

    int width = vinfo.xres_virtual;
    int height = vinfo.yres_virtual;
    int bpp = vinfo.bits_per_pixel / 8;  // bytes per pixel

    *screensize = width * height * bpp;

    *framebuffer = mmap(0, *screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0);
    if (*framebuffer == MAP_FAILED) {
        perror("Error mapping framebuffer device to memory");
        close(fb);
        return -1;
    }

    return fb;
}

int setup_server_socket(void) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Error creating socket");
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error binding socket");
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, 1) < 0) {
        perror("Error listening on socket");
        close(server_fd);
        return -1;
    }

    return server_fd;
}

void cleanup(int fb, int server_fd, unsigned char *framebuffer) {
    if (framebuffer) {
        munmap(framebuffer, fb);
    }
    if (fb != -1) {
        close(fb);
    }
    if (server_fd != -1) {
        close(server_fd);
    }
}

int main() {
    unsigned char *framebuffer = NULL;
    size_t screensize = 0;
    int fb = -1;
    int server_fd = -1;

    // Setup framebuffer and server socket
    if ((fb = setup_framebuffer(&framebuffer, &screensize)) == -1 ||
        (server_fd = setup_server_socket()) == -1) {
        cleanup(fb, server_fd, framebuffer);
        return 1;
    }

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) {
            perror("Error accepting connection");
            continue;
        }

        printf("Client connected.\n");
        handle_client(client_fd, framebuffer, screensize);
        printf("Client disconnected.\n");
    }

    cleanup(fb, server_fd, framebuffer);
    return 0;
}
