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

#include "main.h"

static screen_info_t g_fb_dev;
static system_status_t g_system_status;

static bool thread_quit = false;

static pthread_t g_system_monitor_thread_id = 0;
static pthread_t g_screen_refresh_thread_id = 0;
// static pthread_t g_rtsp_stream_thread_id = 0;

static rtsp_demo_handle g_rtsplive = NULL;
static rtsp_session_handle g_rtsp_session;

static void sigterm_handler(int sig) {
	fprintf(stderr, "signal %d\n", sig);
	thread_quit = true;
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

system_status_t *get_system_status(void)
{
    return &g_system_status;
}

static void *system_monitor_thread(void *pArgs)
{
    int result;
    FILE *fp = NULL;

    // 设置线程为可取消的
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    // 设置取消类型为延迟取消
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    printf("[%s %d] Start system monitor thread......\n", __FILE__, __LINE__);
    const char *get_cpu_usage = "top -n 1 | grep \"CPU:\" | grep -v \"grep\" | awk '{print $8}'";
    const char *get_npu_usage = "cat /proc/rknpu/load | awk '{print $3}'";
    const char *get_cpu_temp = "cat /sys/class/thermal/thermal_zone0/temp";

    while (!thread_quit) {
        fp = popen(get_cpu_usage, "r");
        fscanf(fp, "%d", &result);
        pclose(fp);
        g_system_status.cpu_usage = (100 - result) * 10;

        fp = popen(get_cpu_temp, "r");
        fscanf(fp, "%d", &result);
        pclose(fp);
        g_system_status.cpu_temp = result / 100;

        fp = popen(get_npu_usage, "r");
        fscanf(fp, "%d", &result);
        pclose(fp);
        g_system_status.npu_usage = result * 10;

        usleep(1000 * 500);
    }

    return RK_NULL;
}

static void *screen_refresh_thread(void *pArgs)
{
    printf("[%s %d] Start screen refresh thread......\n", __FILE__, __LINE__);

    int flip = 3;
    int value, last_value;
    export_gpio(GPIO(RK_GPIO4, RK_PC0));
    set_gpio_direction(GPIO(RK_GPIO4, RK_PC0), "in");
    value = read_gpio_value(GPIO(RK_GPIO4, RK_PC0));
    last_value = value;

    while (!thread_quit) {

        value = read_gpio_value(GPIO(RK_GPIO4, RK_PC0));
        if (value == 0 && last_value == 1) {
            flip = flip == 3 ? 0 : 3;
            printf("flip:%d\n", flip);
        }
        last_value = value;

        video_update_screen(g_fb_dev.screen_base, flip);
        usleep(10 * 1000);
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
    while (!thread_quit) {
        video_ret = video_GetFrame(GET_RTSP_FRAME, &fvi_info, NULL);
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

    signal(SIGINT, sigterm_handler);
    signal(SIGTERM, sigterm_handler);

    ret = rgb_lcd_init(fb_dev_name, &g_fb_dev); assert(ret == 0);
    ret = rgb_lcd_clear_screen(&g_fb_dev); assert(ret == 0);

    do {
        if (video_init()) {
            printf("ERROR: video_init faile, exit\n");
            break;
        }

        pthread_create(&g_system_monitor_thread_id, 0, system_monitor_thread, NULL);
        pthread_create(&g_screen_refresh_thread_id, 0, screen_refresh_thread, NULL);
        // pthread_create(&g_rtsp_stream_thread_id, 0, rtsp_push_stream_thread, NULL);

        while (!thread_quit) {
            sleep(1);
        }
        printf("%s exit!\n", __func__);

        if (g_system_monitor_thread_id) {
            pthread_cancel(g_system_monitor_thread_id);
            printf("[%s %d] wait system monitor thread join\n", __FILE__, __LINE__);
            pthread_join(g_system_monitor_thread_id, NULL);
        }

        if (g_screen_refresh_thread_id) {
            printf("[%s %d] wait screen refresh thread join\n", __FILE__, __LINE__);
            pthread_join(g_screen_refresh_thread_id, NULL);
        }

        // if (g_rtsp_stream_thread_id) {
        //     printf("[%s %d] wait rtsp thread join\n", __FILE__, __LINE__);
        //     pthread_join(g_rtsp_stream_thread_id, NULL);
        // }

        ret = 0;
    } while (0);

    printf("[%s %d] video_deinit \n", __FILE__, __LINE__);
    video_deinit();

    rgb_lcd_clear_screen(&g_fb_dev);
    rgb_lcd_deinit(&g_fb_dev);

    printf("[%s %d] main exit\n", __FILE__, __LINE__);
    return ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
