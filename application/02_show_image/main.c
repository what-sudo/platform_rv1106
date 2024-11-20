#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <time.h>
#include <assert.h>

#include "data_type.h"
#include "rgb_lcd.h"
#include "show_bmp.h"

static screen_info_t fb_dev;

void show_hex(uint8_t *buf, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        printf("%x ", buf[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n");
}

int main(int argc, char *argv[])
{
    int ret = -1;
    struct timespec start, end;
    int fps_interval = 0;
    int count = 0;

    if (argc < 3) {
        printf("PS: %s /dev/fbX img_file \n", argv[0]);
        exit(0);
    }

    ret = rgb_lcd_init(argv[1], &fb_dev); assert(ret == 0);

    ret = rgb_lcd_clear_screen(&fb_dev); assert(ret == 0);

    show_bmp_image(argv[2], fb_dev);

    do {
        // rgb_lcd_show_fillrectangle(&fb_dev, 20, 20, 200, 200, 0x000080);
        // rgb_lcd_show_fillrectangle(&fb_dev, 200, 20, 400, 200, 0x008000);
        // rgb_lcd_show_fillrectangle(&fb_dev, 400, 20, 600, 200, 0x800000);
        ret = 0;
    } while (0);

    if (ret) {
        printf("warning: ret:%d\n", ret);
    }
    rgb_lcd_deinit(&fb_dev);

    return ret;
}
