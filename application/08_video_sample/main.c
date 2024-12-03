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
static pthread_t rtsp_stream_thread_id = 0;

static rtsp_demo_handle g_rtsplive = NULL;
static rtsp_session_handle g_rtsp_session;

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

static int save_file(frameInfo_vi_t fvi_info)
{
    static int save_count = 0;
    static FILE* save_file = NULL;

    if (!save_file && save_count < 20) {
        printf("open file\n");
        save_file = fopen("/root/frame.bin", "w");
    }

    if (save_count++ < 20 && save_count > 10) {
        printf("write seq:%d index:%d\n", fvi_info.frame_seq, save_count - 10);
        fwrite(fvi_info.frame_data, 1, fvi_info.frame_size, save_file);
        fflush(save_file);
    }

    if (save_file && save_count > 20) {
        printf("close file\n");
        fclose(save_file);
        save_file = NULL;
    }

    return 0;
}

static void *screen_refresh_thread(void *pArgs)
{
    int video_ret = -1;
    frameInfo_vi_t fvi_info;
    int flip = 0;

    fvi_info.frame_data = malloc(1024 * 1024 * 2);
    if (fvi_info.frame_data == NULL) {
        fprintf(stderr, "[%s %d] malloc err\n", __FILE__, __LINE__);
        return RK_NULL;
    }

    printf("[%s %d] Start screen refresh thread......\n", __FILE__, __LINE__);

    int value, last_value;
    export_gpio(GPIO(RK_GPIO4, RK_PC0));
    set_gpio_direction(GPIO(RK_GPIO4, RK_PC0), "in");
    value = read_gpio_value(GPIO(RK_GPIO4, RK_PC0));
    last_value = value;

    while (!quit) {
        video_ret = video_GetFrame(GET_SCREEN_FRAME, &fvi_info);
        if (!video_ret) {
            // static uint64_t last_timestamp = 0;
            // printf("SCREEN ---> seq:%d w:%d h:%d fmt:%d size:%lld delay:%dms fps:%.1f\n", fvi_info.frame_seq, fvi_info.width, fvi_info.height, fvi_info.PixelFormat, fvi_info.frame_size, (uint32_t)(fvi_info.timestamp - last_timestamp) / 1000, (1000.0 / ((fvi_info.timestamp - last_timestamp) / 1000)));
            // last_timestamp = fvi_info.timestamp;

            value = read_gpio_value(GPIO(RK_GPIO4, RK_PC0));
            if (value == 0 && last_value == 1) {
                flip = flip == 3 ? 0 : 3;
                printf("flip:%d\n", flip);
            }
            last_value = value;

            rgb_lcd_show_rgb888(&fb_dev, 0, 0, fvi_info.width, fvi_info.height, fvi_info.frame_data, 1, flip);
            // save_file(fvi_info);
        }

        usleep(10 * 1000);
    }

    if (fvi_info.frame_data) {
        free(fvi_info.frame_data);
    }

    return RK_NULL;
}

static void *rtsp_push_stream_thread(void *pArgs)
{
    int video_ret = -1;
    frameInfo_vi_t fvi_info;

    fvi_info.frame_data = malloc(1024 * 1024 * 8);
    if (fvi_info.frame_data == NULL) {
        fprintf(stderr, "[%s %d] malloc err\n", __FILE__, __LINE__);
        return RK_NULL;
    }

    // init rtsp
    const char* rtsp_path = "/live/0";
    printf("[%s %d] create rtsp server: RTSP://IP:554/%s\n", __FILE__, __LINE__, rtsp_path);
    g_rtsplive = create_rtsp_demo(554);
    g_rtsp_session = rtsp_new_session(g_rtsplive, rtsp_path);
    rtsp_set_video(g_rtsp_session, RTSP_CODEC_ID_VIDEO_H264, NULL, 0);
    rtsp_sync_video_ts(g_rtsp_session, rtsp_get_reltime(), rtsp_get_ntptime());

    printf("[%s %d] Start Rtsp push stream thread......\n", __FILE__, __LINE__);
    while (!quit) {
        video_ret = video_GetFrame(GET_RTSP_FRAME, &fvi_info);
        if (!video_ret) {
            // static uint64_t last_timestamp = 0;
            // printf("RTSP ---> seq:%d w:%d h:%d fmt:%d size:%lld delay:%dms fps:%.1f\n", fvi_info.frame_seq, fvi_info.width, fvi_info.height, fvi_info.PixelFormat, fvi_info.frame_size, (uint32_t)(fvi_info.timestamp - last_timestamp) / 1000, (1000.0 / ((fvi_info.timestamp - last_timestamp) / 1000)));
            // last_timestamp = fvi_info.timestamp;
            rtsp_tx_video(g_rtsp_session, fvi_info.frame_data, fvi_info.frame_size,
                            fvi_info.timestamp);
            rtsp_do_event(g_rtsplive);
        }
        usleep(10 * 1000);
    }
    if (fvi_info.frame_data) {
        free(fvi_info.frame_data);
    }

    if (g_rtsplive)
        rtsp_del_demo(g_rtsplive);

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
        pthread_create(&rtsp_stream_thread_id, 0, rtsp_push_stream_thread, NULL);

        while (!quit) {
            sleep(1);
        }
        printf("%s exit!\n", __func__);

        if (screen_refresh_thread_id) {
            printf("[%s %d] wait screen refresh thread joid\n", __FILE__, __LINE__);
            pthread_join(screen_refresh_thread_id, NULL);
        }

        if (rtsp_stream_thread_id) {
            printf("[%s %d] wait rtsp thread joid\n", __FILE__, __LINE__);
            pthread_join(rtsp_stream_thread_id, NULL);
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
