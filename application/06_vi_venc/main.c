#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

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
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/prctl.h>

#include <errno.h>
#include <pthread.h>
#include <sys/poll.h>

#include <linux/fb.h>
#include <sys/mman.h>

#include "rgb_lcd.h"
#include "video.h"

static screen_info_t fb_dev;
static bool quit = false;

static void sigterm_handler(int sig) {
	fprintf(stderr, "signal %d\n", sig);
	quit = true;
}

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
    int video_ret = -1;
    char *fb_dev_name = "/dev/fb0";

    frameInfo_vi_t fvi_info;

    if (argc < 2) {
        printf("PS: use default fb_dev_name: %s\n", fb_dev_name);
    } else {
        fb_dev_name = argv[1];
    }
    printf("open fb_dev_name: %s\n", fb_dev_name);

    signal(SIGINT, sigterm_handler);

    ret = rgb_lcd_init(fb_dev_name, &fb_dev); assert(ret == 0);
    ret = rgb_lcd_clear_screen(&fb_dev); assert(ret == 0);

    do {

        if (video_init()) {
            printf("ERROR: video_init faile, exit\n");
            break;
        }

        fvi_info.frame_data = malloc(1024 * 1024 * 4);
        if (fvi_info.frame_data == NULL) {
            fprintf(stderr, "malloc err");
            break;
        }

#define SAVE_FILE   0
#if SAVE_FILE
        FILE* venc0_file = fopen("/userdata/venc_0.h264", "w");
        if (!venc0_file) {
            printf("ERROR: open file fail, exit\n");
            break;
        }
#endif
        printf("Start Get Frame......\n");
        while (1)
        {
            video_ret = video_GetFrame(GET_SCREEN_FRAME, &fvi_info);
            if (video_ret) {
                printf("error: video_GetFrame fail\n");
                break;
            }

            rgb_lcd_show_xbgr8888(&fb_dev, 0, 0, fvi_info.width, fvi_info.height, fvi_info.frame_data);

            video_ret = video_GetFrame(GET_VENC_FRAME, &fvi_info);
            if (video_ret) {
                printf("error: video_GetFrame fail\n");
                break;
            }

#if SAVE_FILE
            if (venc0_file) {
                fwrite(fvi_info.frame_data, 1, fvi_info.frame_size, venc0_file);
                fflush(venc0_file);
            }
#endif

            if (quit) {
                break;
            }
            usleep(10 * 1000);
        }

#if SAVE_FILE
        if (venc0_file) fclose(venc0_file);
#endif

        ret = 0;
    } while (0);

    if (video_ret) {
        printf("warning: video_ret:%d\n", ret);
    }

    video_deinit();

    rgb_lcd_clear_screen(&fb_dev);
    rgb_lcd_deinit(&fb_dev);

    return ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
