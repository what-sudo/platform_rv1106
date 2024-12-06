#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

int export_gpio(int gpio_number) {
    int ret = -1;
    char number[32];

    snprintf(number, sizeof(number), "/sys/class/gpio/gpio%d", gpio_number);
    if (access(number, F_OK) == 0) {
        return 0;
    }

    snprintf(number, sizeof(number), "%d", gpio_number);
    int fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd == -1) {
        perror("Unable to open /sys/class/gpio/export");
        return ret;
    }
    if (write(fd, number, strlen(number)) == -1) {
        perror("Error writing to /sys/class/gpio/export");
        close(fd);
        return ret;
    }
    close(fd);

    return 0;
}

int set_gpio_direction(int gpio_number, const char *direction) {
    int ret = -1;
    char path[48];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", gpio_number);
    int fd = open(path, O_WRONLY);
    if (fd == -1) {
        perror("Unable to open gpio direction");
        return ret;
    }
    if (write(fd, direction, strlen(direction)) == -1) {
        perror("Error writing to gpio direction");
        close(fd);
        return ret;
    }
    close(fd);
    return 0;
}

int read_gpio_value(int gpio_number) {
    char path[32];
    char value_str[3];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", gpio_number);
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        perror("Unable to open gpio value");
        return -1;
    }
    if (read(fd, value_str, 3) == -1) {
        perror("Error reading gpio value");
        close(fd);
        return -1;
    }
    close(fd);
    return atoi(value_str);
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
