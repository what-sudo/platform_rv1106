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

#include "rtsp_demo.h"
#include "rv1106_iva.h"

static screen_info_t fb_dev;
static bool quit = false;

static pthread_t screen_refresh_thread_id = 0;

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

static void *screen_refresh_thread(void *pArgs)
{
    int video_ret = -1;
    frameInfo_vi_t fvi_info;

    fvi_info.frame_data = malloc(1024 * 1024 * 2);
    if (fvi_info.frame_data == NULL) {
        fprintf(stderr, "[%s %d] malloc err\n", __FILE__, __LINE__);
        return RK_NULL;
    }

    printf("[%s %d] Start screen refresh thread......\n", __FILE__, __LINE__);

    while (!quit) {
        video_ret = video_GetFrame(GET_SCREEN_FRAME, &fvi_info);
        if (!video_ret) {
            static uint64_t last_timestamp = 0;
            // printf("SCREEN ---> seq:%d w:%d h:%d fmt:%d size:%lld delay:%dms fps:%.1f\n", fvi_info.frame_seq, fvi_info.width, fvi_info.height, fvi_info.PixelFormat, fvi_info.frame_size, (uint32_t)(fvi_info.timestamp - last_timestamp) / 1000, (1000.0 / ((fvi_info.timestamp - last_timestamp) / 1000)));
            last_timestamp = fvi_info.timestamp;
            rgb_lcd_show_rgb888(&fb_dev, 0, 0, fvi_info.width, fvi_info.height, fvi_info.frame_data, 1);
        }
        usleep(10 * 1000);
    }

    if (fvi_info.frame_data) {
        free(fvi_info.frame_data);
    }

    return RK_NULL;
}

int main(int argc, char *argv[])
{
    int ret = -1;
    char *fb_dev_name = "/dev/fb0";

    if (argc < 2) {
        printf("PS: use default fb_dev_name: %s\n", fb_dev_name);
    } else {
        fb_dev_name = argv[1];
    }
    printf("open fb_dev_name: %s\n", fb_dev_name);

    signal(SIGINT, sigterm_handler);
    signal(SIGTERM, sigterm_handler);

    ret = rgb_lcd_init(fb_dev_name, &fb_dev); assert(ret == 0);
    ret = rgb_lcd_clear_screen(&fb_dev); assert(ret == 0);

    do {

        if (video_init()) {
            printf("ERROR: video_init faile, exit\n");
            break;
        }

        pthread_create(&screen_refresh_thread_id, 0, screen_refresh_thread, NULL);

        while (!quit) {
            sleep(1);
        }
        printf("%s exit!\n", __func__);

        if (screen_refresh_thread_id) {
            printf("[%s %d] wait screen refresh thread joid\n", __FILE__, __LINE__);
            pthread_join(screen_refresh_thread_id, NULL);
        }

        ret = 0;
    } while (0);

    printf("[%s %d] video_deinit \n", __FILE__, __LINE__);
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
