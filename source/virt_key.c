#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/uinput.h>

void emit(int fd, int type, int code, int val) {
    struct input_event ie = {0};
    ie.type = type; ie.code = code; ie.value = val;
    write(fd, &ie, sizeof(ie));
}

int main() {
    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) return 1;

    // Разрешаем клавиши
    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    // Регистрируем все основные клавиши (от 1 до 255 - это буквы, цифры, стрелки)
    for (int i = 1; i < 255; i++) {
        ioctl(fd, UI_SET_KEYBIT, i);
    }

    struct uinput_user_dev uidev = {0};
    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "VirtualKey");
    uidev.id.bustype = BUS_USB;

    write(fd, &uidev, sizeof(uidev));
    ioctl(fd, UI_DEV_CREATE);

    fprintf(stderr, "Клавиатура готова. Жду коды...\n");

    int key_code, state;
    while (scanf("%d %d", &key_code, &state) != EOF) {
        emit(fd, EV_KEY, key_code, state);
        emit(fd, EV_SYN, SYN_REPORT, 0);
    }

    ioctl(fd, UI_DEV_DESTROY);
    close(fd);
    return 0;
}
