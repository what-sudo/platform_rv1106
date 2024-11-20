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

        fvi_info.frame_data = malloc(1024 * 1024 * 10);
        if (fvi_info.frame_data == NULL) {
            fprintf(stderr, "malloc err");
            break;
        }

#define SAVE_FILE   0
#if SAVE_FILE
        int save_count = 0;
        FILE* venc0_file = fopen("/userdata/frame.bin", "w");
        if (!venc0_file) {
            printf("ERROR: open file fail, exit\n");
            break;
        }
#endif
        printf("Start Get Frame......\n");
        struct timespec real_time;
        long long last_time = real_time.tv_sec * 1000LL + real_time.tv_nsec / 1000000;
        long long cur_time = real_time.tv_sec * 1000LL + real_time.tv_nsec / 1000000;
        while (1)
        {

#if 1
            video_ret = video_GetFrame(GET_SCREEN_FRAME, &fvi_info);
            if (video_ret) {
                printf("error: [%s %d] video_GetFrame fail\n", __FILE__, __LINE__);
            } else {
                clock_gettime(CLOCK_REALTIME, &real_time);
                cur_time = real_time.tv_sec * 1000LL + real_time.tv_nsec / 1000000;
                printf("---> VI 1 seq:%d w:%d h:%d fmt:%d size:%lld delay:%lldms\n", fvi_info.frame_seq, fvi_info.width, fvi_info.height, fvi_info.PixelFormat, fvi_info.frame_size, cur_time - last_time);
                last_time = cur_time;

                rgb_lcd_show_rgb888(&fb_dev, 0, 0, fvi_info.width, fvi_info.height, fvi_info.frame_data, 1);
            }
#endif

#if 0
            video_ret = video_GetFrame(GET_VENC_FRAME, &fvi_info);
            if (video_ret) {
                printf("error: [%s %d] video_GetFrame fail\n", __FILE__, __LINE__);
            } else {
                clock_gettime(CLOCK_REALTIME, &real_time);
                cur_time = real_time.tv_sec * 1000LL + real_time.tv_nsec / 1000000;
                printf("---> VENC 0 seq:%d w:%d h:%d fmt:%d size:%lld delay:%lldms\n", fvi_info.frame_seq, fvi_info.width, fvi_info.height, fvi_info.PixelFormat, fvi_info.frame_size, cur_time - last_time);
                last_time = cur_time;
            }
#endif

#if 0
            video_ret = video_GetFrame(GET_test1_FRAME, &fvi_info);
            if (video_ret) {
                printf("error: [%s %d] video_GetFrame fail\n", __FILE__, __LINE__);
            } else {
                clock_gettime(CLOCK_REALTIME, &real_time);
                cur_time = real_time.tv_sec * 1000LL + real_time.tv_nsec / 1000000;
                printf("---> VPSS 1 seq:%d w:%d h:%d fmt:%d size:%lld delay:%lldms\n", fvi_info.frame_seq, fvi_info.width, fvi_info.height, fvi_info.PixelFormat, fvi_info.frame_size, cur_time - last_time);
                last_time = cur_time;
#if SAVE_FILE
                if (venc0_file && save_count++ < 20 && save_count > 10) {
                    fwrite(fvi_info.frame_data, 1, fvi_info.frame_size, venc0_file);
                    fflush(venc0_file);
                }
#endif
            }
#endif

            if (quit) {
                break;
            }
            // usleep(10 * 1000);
        }

#if SAVE_FILE
        if (venc0_file) fclose(venc0_file);
#endif

        ret = 0;
    } while (0);

    if (video_ret) {
        printf("warning: video_ret:0x%X\n", video_ret);
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
