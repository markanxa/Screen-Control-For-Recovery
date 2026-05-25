#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/uinput.h>

void emit(int fd, int type, int code, int val) {
    struct input_event ie;
    ie.type = type; ie.code = code; ie.value = val;
    ie.time.tv_sec = 0; ie.time.tv_usec = 0;
    write(fd, &ie, sizeof(ie));
}

int main() {
    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) return 1;

    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_KEYBIT, BTN_TOUCH);
    ioctl(fd, UI_SET_EVBIT, EV_ABS);
    // Мультитач
    ioctl(fd, UI_SET_ABSBIT, ABS_MT_POSITION_X);
    ioctl(fd, UI_SET_ABSBIT, ABS_MT_POSITION_Y);
    ioctl(fd, UI_SET_ABSBIT, ABS_MT_TRACKING_ID);
    // Обычный тач (для надежности)
    ioctl(fd, UI_SET_ABSBIT, ABS_X);
    ioctl(fd, UI_SET_ABSBIT, ABS_Y);

    struct uinput_user_dev uidev;
    memset(&uidev, 0, sizeof(uidev));
    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "VirtualTouch");
    uidev.absmin[ABS_MT_POSITION_X] = 0; uidev.absmax[ABS_MT_POSITION_X] = 1080;
    uidev.absmin[ABS_MT_POSITION_Y] = 0; uidev.absmax[ABS_MT_POSITION_Y] = 2400;
    uidev.absmin[ABS_X] = 0; uidev.absmax[ABS_X] = 1080;
    uidev.absmin[ABS_Y] = 0; uidev.absmax[ABS_Y] = 2400;

    write(fd, &uidev, sizeof(uidev));
    ioctl(fd, UI_DEV_CREATE);

    int x, y, track_id = 1;
    while (scanf("%d %d", &x, &y) != EOF) {
        // DOWN
        emit(fd, EV_ABS, ABS_MT_TRACKING_ID, track_id++);
        emit(fd, EV_ABS, ABS_MT_POSITION_X, x);
        emit(fd, EV_ABS, ABS_MT_POSITION_Y, y);
        emit(fd, EV_ABS, ABS_X, x);
        emit(fd, EV_ABS, ABS_Y, y);
        emit(fd, EV_KEY, BTN_TOUCH, 1);
        emit(fd, EV_SYN, SYN_REPORT, 0);

        usleep(70000); // 70ms задержка нажатия

        // UP
        emit(fd, EV_ABS, ABS_MT_TRACKING_ID, -1);
        emit(fd, EV_KEY, BTN_TOUCH, 0);
        emit(fd, EV_SYN, SYN_REPORT, 0);

        fprintf(stderr, "Touch: %d %d (ID:%d)\n", x, y, track_id-1);
    }

    ioctl(fd, UI_DEV_DESTROY);
    close(fd);
    return 0;
}
