#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define DRM_IOCTL_MODE_GETRESOURCES 0xC0A064A0
#define DRM_IOCTL_MODE_GETCRTC      0xC06864A1
#define DRM_IOCTL_MODE_GETFB        0xC01C64AD
#define DRM_IOCTL_MODE_MAP_DUMB     0xC01064B3

struct drm_mode_card_res { uint64_t f, c, co, e; uint32_t nf, nc, nco, ne, miw, maw, mih, mah; };
struct drm_mode_crtc { uint64_t co; uint32_t nco, id, fb_id, x, y, g, v; char m[68]; };
struct drm_mode_fb_cmd { uint32_t fb_id, width, height, pitch, bpp, depth, handle; };
struct drm_mode_map_dumb { uint32_t handle, pad; uint64_t offset; };

typedef struct {
    uint8_t* map;
    uint32_t fb_width, fb_height, fb_pitch;
    uint32_t ow, oh;
    uint8_t* buf;
    int skip;
    int fd;
    volatile int running;
} SharedData;

SharedData data = {0};

void* monitor_thread(void* arg) {
    while (data.running) {
        uint32_t ids[32];
        struct drm_mode_card_res res = { .c = (uintptr_t)ids, .nc = 32 };
        ioctl(data.fd, DRM_IOCTL_MODE_GETRESOURCES, &res);

        uint32_t fb_id = 0;
        for (int i = 0; i < (int)res.nc; i++) {
            struct drm_mode_crtc c = { .id = ids[i] };
            if (ioctl(data.fd, DRM_IOCTL_MODE_GETCRTC, &c) == 0 && c.fb_id > 0) {
                fb_id = c.fb_id;
                break;
            }
        }

        if (fb_id) {
            struct drm_mode_fb_cmd fb = { .fb_id = fb_id };
            if (ioctl(data.fd, DRM_IOCTL_MODE_GETFB, &fb) == 0) {
                if (fb.width != data.fb_width || fb.height != data.fb_height || !data.map) {
                    if (data.map) munmap(data.map, data.fb_pitch * data.fb_height);
                    if (data.buf) free(data.buf);

                    struct drm_mode_map_dumb m = { .handle = fb.handle };
                    ioctl(data.fd, DRM_IOCTL_MODE_MAP_DUMB, &m);

                    data.map = mmap(0, fb.pitch * fb.height, PROT_READ, MAP_SHARED, data.fd, m.offset);
                    data.fb_width = fb.width;
                    data.fb_height = fb.height;
                    data.fb_pitch = fb.pitch;

                    data.ow = fb.width / data.skip;
                    data.oh = fb.height / data.skip;
                    data.buf = malloc(data.ow * 3);

                    fprintf(stderr, "FFPLAY SIZE: %dx%d\n", data.ow, data.oh);
                }
            }
        }
        usleep(80000);
    }
    return NULL;
}

int main(int argc, char** argv) {
    data.skip = (argc > 1) ? atoi(argv[1]) : 1;
    data.fd = open("/dev/dri/card0", O_RDWR);
    if (data.fd < 0) {
        fprintf(stderr, "Не удалось открыть /dev/dri/card0\n");
        return 1;
    }

    data.running = 1;
    pthread_t monitor;
    pthread_create(&monitor, NULL, monitor_thread, NULL);

    uint64_t total_bytes = 0;
    time_t last_time = time(NULL);
    int frames = 0;

    while(1) {
        if (!data.map || !data.buf) {
            usleep(50000);
            continue;
        }

        uint64_t frame_bytes = (uint64_t)data.ow * data.oh * 3;

        for (uint32_t y = 0; y < data.oh; y++) {
            uint8_t* row_start = data.map + (y * data.skip * data.fb_pitch);
            uint8_t* d = data.buf;

            for (uint32_t x = 0; x < data.ow; x++) {
                uint8_t* s = row_start + (x * data.skip * 4);
                d[0] = s[0];
                d[1] = s[1];
                d[2] = s[2];
                d += 3;
            }
            fwrite(data.buf, data.ow * 3, 1, stdout);
        }
        fflush(stdout);

        total_bytes += frame_bytes;
        frames++;

        if (time(NULL) - last_time >= 2) {
            double seconds = difftime(time(NULL), last_time);
            double mbytes_per_sec = (total_bytes / 1024.0 / 1024.0) / seconds;

            fprintf(stderr, "\033[1;32mSpeed: %.1f MB/s | FPS: %d | Res: %dx%d\033[0m\n",
                    mbytes_per_sec, frames / (int)seconds, data.ow, data.oh);
            fflush(stderr);

            total_bytes = 0;
            frames = 0;
            last_time = time(NULL);
        }

        usleep(18000);
    }

    return 0;
}
