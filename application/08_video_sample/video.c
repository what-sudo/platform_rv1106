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
#include <semaphore.h>

#include "video.h"
#include "rv1106_video_init.h"

#define SENSOR_WIDTH     2304
#define SENSOR_HEIGHT    1296

typedef struct {
    sem_t sem;
    pthread_mutex_t mutex;
    pthread_t threadId;
    uint32_t objNum;                /* 目标个数 */
    RockIvaObjectInfo objInfo[128]; /* 各目标检测信息 */
} iva_result_param_t;

static bool g_thread_quit = true;
static iva_result_param_t g_iva_result_ctx = {0};
static iva_result_param_t g_screen_iva_ctx = {0};
static pthread_t g_iva_thread_id = 0;

static graphics_image_t g_graphics_image = {0};

static int rv1106_iva_result_cb(video_iva_callback_param_t *ctx);

static rv1106_video_init_param_t g_video_param_list_ctx = {
    .isp = {
        {
            .enable = 1,
            .ViDevId = 0,
            .iq_file_dir = "/etc/iqfiles",
            .hdr_mode = RK_AIQ_WORKING_MODE_NORMAL,
        }
    },
    .vi_dev = {
        {
            .enable = 1,
            .ViDevId = 0,
            .BindPipe = {
                .u32Num = 1,
                .PipeId = {0},
            },
        },
    },
    .vi_chn = {
        {
            .enable = 1,
            .ViPipe = 0,
            .viChnId = 0,
            .width = SENSOR_WIDTH,
            .height = SENSOR_HEIGHT,
            .SrcFrameRate = -1,
            .DstFrameRate = -1,
            .PixelFormat = RK_FMT_YUV420SP,
        },
        {
            .enable = 1,
            .ViPipe = 0,
            .viChnId = 1,
            .width = 576,
            .height = 324,
            .SrcFrameRate = 25,
            .DstFrameRate = 10,
            .PixelFormat = RK_FMT_YUV420SP,
        },
    },
    .vpss = {
        {
            .enable = 1,
            .VpssGrpID = 0,
            .inWidth = SENSOR_WIDTH,
            .inHeight = SENSOR_HEIGHT,
            .SrcFrameRate = -1,
            .DstFrameRate = -1,
            .inPixelFormat = RK_FMT_YUV420SP,
            .bindSrcChn = {RK_ID_VI, 0, 0},
            .chn = {
                {
                    .enable = 1,
                    .VpssChnID = 0,
                    .outWidth = 800,
                    .outHeight = 480,
                    .SrcFrameRate = 25,
                    .DstFrameRate = 25,
                    .outPixelFormat = RK_FMT_RGB888,
                    .bMirror = false,
                    .bFlip = false,
                },
                {
                    .enable = 0,
                    .VpssChnID = 1,
                    .outWidth = SENSOR_WIDTH,
                    .outHeight = SENSOR_HEIGHT,
                    .SrcFrameRate = 25,
                    .DstFrameRate = 25,
                    .outPixelFormat = RK_FMT_YUV420SP,
                    .bMirror = false,
                    .bFlip = false,
                }
            },
        },
    },
    .venc = {
        {
            .enable = 1,
            .enType = RK_VIDEO_ID_AVC,
            .vencChnId = 0,
            .width  = SENSOR_WIDTH,
            .height = SENSOR_HEIGHT,
            .PixelFormat = RK_FMT_YUV420SP,
            .bindSrcChn = {RK_ID_VI, 0, 0},
        },
        {
            .enable = 0
        },
    },
    .rgn = {
        {
            .enable = 1,
            .rgnHandle = 0,
            .layer = 0,
            .type = OVERLAY_RGN,
            .X = 0,
            .Y = 0,
            .width = SENSOR_WIDTH,
            .height = SENSOR_HEIGHT,
            .show = true,
            .mppChn = {RK_ID_VENC, 0, 0},
            .overlay = {
                .format = RK_FMT_BGRA5551,
                .u32FgAlpha = 255,
                .u32BgAlpha = 0,
            }
        },
    },
    .iva = {
        {
            .enable = 1,
            .models_path = "/oem/rockiva_data/",
            .width = 576,
            .height = 324,
            .IvaPixelFormat = ROCKIVA_IMAGE_FORMAT_YUV420SP_NV12,
            .result_cb = rv1106_iva_result_cb,
        },
    }
};

static int osd_init(void)
{
    RK_S32 s32Ret = RK_FAILURE;
    RGN_CANVAS_INFO_S CanvasInfo = {0};
    do {
        s32Ret = rv1106_rgn_overlay_get_canvas(&g_video_param_list_ctx.rgn[0], &CanvasInfo);
        if (s32Ret != RK_SUCCESS) {
            printf("[%s %d] error: rv1106_rgn_overlay_get_canvas ret:0x%X\n", __func__, __LINE__, s32Ret);
            break;
        }

        g_graphics_image.width = CanvasInfo.u32VirWidth;
        g_graphics_image.height = CanvasInfo.u32VirHeight;
        g_graphics_image.buf = (uint8_t*)(uint32_t)(CanvasInfo.u64VirAddr);

        switch (CanvasInfo.enPixelFmt) {
            case RK_FMT_BGRA5551: {
                g_graphics_image.fmt = GD_FMT_BGRA5551;
                g_graphics_image.line_length = g_graphics_image.width * 2;
            } break;
            default:
                s32Ret = -2;
                printf("[%s %d] error: graphics fmt unsupport\n", __func__, __LINE__);
                return -1;
        }

        graphics_full(&g_graphics_image, graphics_Clear);

        // graphics_rectangle(&g_graphics_image, 0, 0, 2304, 1296, graphics_Red);
        // graphics_fillrectangle(&g_graphics_image, 60, 60, 300, 500, graphics_Red);
        // graphics_fillrectangle(&g_graphics_image, 310, 60, 600, 500, graphics_Green);
        // graphics_fillrectangle(&g_graphics_image, 610, 60, 900, 500, graphics_Blue);
        // graphics_fillrectangle(&g_graphics_image, 910, 60, 1200, 500, graphics_Yellow);
        // graphics_fillrectangle(&g_graphics_image, 1210, 60, 1500, 500, graphics_Cyan);
        // graphics_fillrectangle(&g_graphics_image, 1510, 60, 1800, 500, graphics_Magenta);
        // graphics_fillrectangle(&g_graphics_image, 60, 600, 300, 1000, graphics_White);
        // graphics_fillrectangle(&g_graphics_image, 310, 600, 600, 1000, graphics_Black);
        // graphics_show_string(&g_graphics_image, 330, 330, "ABCD", GD_FONT_16x32, graphics_Red);
        // graphics_show_string(&g_graphics_image, 330, 430, "ABCD", GD_FONT_16x32, graphics_Red);

        s32Ret = rv1106_rgn_overlay_set_canvas(&g_video_param_list_ctx.rgn[0], &CanvasInfo);
        if (s32Ret != RK_SUCCESS) {
            printf("[%s %d] error: rv1106_rgn_overlay_set_canvas ret:0x%X\n", __func__, __LINE__, s32Ret);
            break;
        }
        s32Ret = RK_SUCCESS;
    } while (0);

    return s32Ret;
}

// static RK_U64 TEST_COMM_GetNowUs() {
//     struct timespec time = {0, 0};
//     clock_gettime(CLOCK_MONOTONIC, &time);
//     return (RK_U64)time.tv_sec * 1000000 + (RK_U64)time.tv_nsec / 1000; /* microseconds */
// }

static int rv1106_iva_result_cb(video_iva_callback_param_t *ctx)
{
    RK_S32 s32Ret = RK_FAILURE;

    pthread_mutex_lock(&g_iva_result_ctx.mutex);

    g_iva_result_ctx.objNum = ctx->objNum;
    memcpy(g_iva_result_ctx.objInfo, ctx->objInfo, sizeof(RockIvaObjectInfo) * ctx->objNum);

    pthread_mutex_unlock(&g_iva_result_ctx.mutex);

    s32Ret = sem_post(&g_iva_result_ctx.sem);  // 释放资源

    // static uint64_t last_timestamp = 0;
    // uint64_t new_timestamp = TEST_COMM_GetNowUs();
    // printf("IVA result ---> seq:%d delay:%dms fps:%.1f\n", ctx->objInfo[0].frameId, (uint32_t)(new_timestamp - last_timestamp) / 1000, (1000.0 / ((new_timestamp - last_timestamp) / 1000)));
    // last_timestamp = new_timestamp;

    return s32Ret;
}

static void thread_cleanup(void *arg) {
    printf("执行清理: 清理资源\n");
}

static void* osd_update_thread(void* arg) {

    RK_S32 s32Ret = RK_FAILURE;
    int i;
    RK_U32 X1, Y1, X2, Y2;
    graphics_color_t color = {0};
    video_rgn_param_t *rgn = &g_video_param_list_ctx.rgn[0];
    char text_buf[32] = { 0 };
    RGN_CANVAS_INFO_S CanvasInfo = {0};

    // 设置线程为可取消的
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    // 设置取消类型为延迟取消
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    // 注册清理函数
    pthread_cleanup_push(thread_cleanup, NULL);

    while(1) {
        sem_wait(&g_iva_result_ctx.sem);  // 请求访问资源
        pthread_mutex_lock(&g_iva_result_ctx.mutex);

        g_screen_iva_ctx.objNum = g_iva_result_ctx.objNum;
        memcpy(g_screen_iva_ctx.objInfo, g_iva_result_ctx.objInfo, sizeof(RockIvaObjectInfo) * g_iva_result_ctx.objNum);

        s32Ret = rv1106_rgn_overlay_get_canvas(rgn, &CanvasInfo);
        if (s32Ret != RK_SUCCESS) {
            printf("[%s %d] error: rv1106_rgn_overlay_get_canvas ret:0x%X\n", __func__, __LINE__, s32Ret);
            // return s32Ret;
        }
        g_graphics_image.buf = (uint8_t*)(uint32_t)(CanvasInfo.u64VirAddr);
        graphics_full(&g_graphics_image, graphics_Clear);

        char *objname = "NONE";
        for (i = 0; i < g_iva_result_ctx.objNum; i++) {
            if (g_iva_result_ctx.objInfo[i].score < 30)
                continue;

            switch (g_iva_result_ctx.objInfo[i].type)
            {
                case ROCKIVA_OBJECT_TYPE_PET: {
                    objname = "PET";
                    color = graphics_Green;
                } break;
                case ROCKIVA_OBJECT_TYPE_FACE: {
                    objname = "FACE";
                    color = graphics_Blue;
                } break;
                case ROCKIVA_OBJECT_TYPE_PERSON: {
                    objname = "PERSON";
                    color = graphics_Red;
                } break;
                case ROCKIVA_OBJECT_TYPE_HEAD: {
                    objname = "HEAD";
                    color = graphics_Yellow;
                } break;
                case ROCKIVA_OBJECT_TYPE_PLATE: {
                    objname = "PLATE";
                    color = graphics_Cyan;
                } break;
                case ROCKIVA_OBJECT_TYPE_VEHICLE: {
                    objname = "VEHICLE";
                    color = graphics_Magenta;
                } break;
                case ROCKIVA_OBJECT_TYPE_NON_VEHICLE: {
                    objname = "NON_VEHICLE";
                    color = graphics_Magenta;
                } break;
                case ROCKIVA_OBJECT_TYPE_BICYCLE: {
                    objname = "BICYCLE";
                    color = graphics_Magenta;
                } break;
                case ROCKIVA_OBJECT_TYPE_MOTORCYCLE: {
                    objname = "MOTORCYCLE";
                    color = graphics_Magenta;
                } break;
                default: {
                    printf("Warning: obj type NONE %d", g_iva_result_ctx.objInfo[i].type);
                    objname = "NONE";
                } break;
            }

            X1 = ROCKIVA_RATIO_PIXEL_CONVERT(SENSOR_WIDTH, g_iva_result_ctx.objInfo[i].rect.topLeft.x);
            Y1 = ROCKIVA_RATIO_PIXEL_CONVERT(SENSOR_HEIGHT, g_iva_result_ctx.objInfo[i].rect.topLeft.y);
            X2 = ROCKIVA_RATIO_PIXEL_CONVERT(SENSOR_WIDTH, g_iva_result_ctx.objInfo[i].rect.bottomRight.x);
            Y2 = ROCKIVA_RATIO_PIXEL_CONVERT(SENSOR_HEIGHT, g_iva_result_ctx.objInfo[i].rect.bottomRight.y);

            if (X1 > SENSOR_WIDTH || Y1 > SENSOR_HEIGHT || X2 > SENSOR_WIDTH || Y2 > SENSOR_HEIGHT) {
                // printf("[%s %d] error: ---\n", __func__, __LINE__);
                // printf("obj:%d/%d %s X1:%u Y1:%u X2:%u Y2:%u\n", i + 1, g_iva_result_ctx.objNum, objname, (uint16_t)X1, (uint16_t)Y2, (uint16_t)X2, (uint16_t)Y2);
            } else {
                // printf("req:%d objNum:%d/%d %s %d%% X1:%u Y1:%u X2:%u Y2:%u\n", g_iva_result_ctx.objInfo[i].frameId, i + 1, g_iva_result_ctx.objNum, objname, g_iva_result_ctx.objInfo[i].score, (uint16_t)X1, (uint16_t)Y2, (uint16_t)X2, (uint16_t)Y2);

                snprintf(text_buf, sizeof(text_buf), "%s %d%%", objname, g_iva_result_ctx.objInfo[i].score);
                graphics_rectangle(&g_graphics_image, X1, Y1, X2, Y2, color);
                graphics_show_string(&g_graphics_image, X1 + 4, Y1 + 4, text_buf, GD_FONT_16x32B, color);
            }
        }
        pthread_mutex_unlock(&g_iva_result_ctx.mutex);

        s32Ret = rv1106_rgn_overlay_set_canvas(rgn, &CanvasInfo);
        if (s32Ret != RK_SUCCESS) {
            printf("[%s %d] error: rv1106_rgn_overlay_set_canvas ret:0x%X\n", __func__, __LINE__, s32Ret);
            // return s32Ret;
        }
    }

    // 移除清理函数
    pthread_cleanup_pop(1);
    return NULL;
}

static void *iva_push_frame_thread(void *pArgs)
{
    int video_ret = -1;
    frameInfo_vi_t fvi_info = {0};

    // // 设置线程为可取消的
    // pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    // // 设置取消类型为延迟取消
    // pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    // // 注册清理函数
    // pthread_cleanup_push(thread_cleanup, NULL);

    printf("[%s %d] Start iva push stream thread......\n", __FILE__, __LINE__);

    while (g_thread_quit) {
        video_ret = video_GetFrame(GET_IVA_FRAME, &fvi_info);
        if (!video_ret) {
            // static uint64_t last_timestamp = 0;
            // printf("GET IVA ---> seq:%d w:%d h:%d fmt:%d size:%lld delay:%dms fps:%.1f\n", fvi_info.frame_seq, fvi_info.width, fvi_info.height, fvi_info.PixelFormat, fvi_info.frame_size, (uint32_t)(fvi_info.timestamp - last_timestamp) / 1000, (1000.0 / ((fvi_info.timestamp - last_timestamp) / 1000)));
            // last_timestamp = fvi_info.timestamp;
        }
        usleep(10 * 1000);
    }

    // 移除清理函数
    // pthread_cleanup_pop(1);

    return RK_NULL;
}

int video_init(void)
{
    RK_S32 s32Ret = RK_FAILURE;
    s32Ret = rv1106_video_init(&g_video_param_list_ctx);
    if (s32Ret != RK_SUCCESS) {
        printf("[%s %d] error: rv1106_video_init fail: ret:0x%x\n", __func__, __LINE__, s32Ret);
        return s32Ret;
    }

    if (g_video_param_list_ctx.rgn[0].enable) {
        s32Ret = osd_init();
        if (s32Ret != RK_SUCCESS) {
            printf("[%s %d] error: osd_init fail: ret:0x%x\n", __func__, __LINE__, s32Ret);
            return s32Ret;
        }

        s32Ret = sem_init(&g_iva_result_ctx.sem, 0, 0);  // 初始化信号量，允许两个线程同时访问
        if (s32Ret != RK_SUCCESS) {
            printf("error: sem_init fail: ret:0x%x\n", s32Ret);
            return s32Ret;
        }

        s32Ret = pthread_mutex_init(&g_iva_result_ctx.mutex, NULL);
        if (s32Ret != RK_SUCCESS) {
            printf("error: pthread_mutex_init fail: ret:0x%x\n", s32Ret);
            return s32Ret;
        }

        pthread_create(&g_iva_result_ctx.threadId, NULL, osd_update_thread, NULL);
    }

    if (g_video_param_list_ctx.iva[0].enable) {
        pthread_create(&g_iva_thread_id, 0, iva_push_frame_thread, NULL);
    }

    return s32Ret;
}

int video_deinit(void)
{
    RK_S32 s32Ret = RK_FAILURE;

    g_thread_quit = false;

    if (g_video_param_list_ctx.iva[0].enable) {
        if (g_iva_thread_id) {
            printf("[%s %d] wait iva thread joid\n", __FILE__, __LINE__);
            // pthread_cancel(g_iva_thread_id);
            pthread_join(g_iva_thread_id, NULL);
        }
    }

    if (g_video_param_list_ctx.rgn[0].enable) {
        pthread_cancel(g_iva_result_ctx.threadId);
        pthread_join(g_iva_result_ctx.threadId, NULL);
        pthread_mutex_destroy(&g_iva_result_ctx.mutex);

        s32Ret = sem_destroy(&g_iva_result_ctx.sem);  // 销毁信号量
        if (s32Ret != RK_SUCCESS) {
            printf("error: sem_destroy fail: ret:0x%x\n", s32Ret);
            return s32Ret;
        }
    }

    return rv1106_video_deinit(&g_video_param_list_ctx);
}

int video_GetFrame(get_frame_type_t type, frameInfo_vi_t *Fvi_info)
{
    RK_S32 s32Ret = RK_FAILURE;

    if (Fvi_info == NULL) {
        fprintf(stderr, "parame error\n");
        return s32Ret;
    }

    if (type == GET_VENC_FRAME) {
        s32Ret = rv1106_venc_GetStream(&g_video_param_list_ctx.venc[0], Fvi_info);
        if (s32Ret != RK_SUCCESS) {
            printf("error: GET_VENC_FRAME fail: ret:0x%x\n", s32Ret);
            return s32Ret;
        }
    } else if (type == GET_RTSP_FRAME) {
        s32Ret = rv1106_venc_GetStream(&g_video_param_list_ctx.venc[0], Fvi_info);
        if (s32Ret != RK_SUCCESS) {
            printf("error: GET_RTSP_FRAME fail: ret:0x%x\n", s32Ret);
            return s32Ret;
        }
    } else if (type == GET_SCREEN_FRAME) {
        graphics_image_t screen_graphics_image = {0};
        RK_U32 X1, Y1, X2, Y2;
        char *objname = "NONE";
        graphics_color_t color = {0};
        char text_buf[32] = { 0 };

        s32Ret = rv1106_vpss_GetStream(&g_video_param_list_ctx.vpss[0], g_video_param_list_ctx.vpss[0].chn[0].VpssChnID, Fvi_info);
        if (s32Ret != RK_SUCCESS) {
            printf("error: GET_SCREEN_FRAME fail: vpss grp:%d ret:0x%X\n", g_video_param_list_ctx.vpss[0].VpssGrpID, s32Ret);
            return s32Ret;
        }

        screen_graphics_image.width = Fvi_info->width;
        screen_graphics_image.height = Fvi_info->height;
        screen_graphics_image.fmt = GD_FMT_RGB888;
        screen_graphics_image.line_length = Fvi_info->width * 3;
        screen_graphics_image.buf = Fvi_info->frame_data;

        for (int i = 0; i < g_screen_iva_ctx.objNum; i++) {
            if (g_screen_iva_ctx.objInfo[i].score < 30)
                continue;

            switch (g_screen_iva_ctx.objInfo[i].type)
            {
                case ROCKIVA_OBJECT_TYPE_PET: {
                    objname = "PET";
                    color = graphics_Green;
                } break;
                case ROCKIVA_OBJECT_TYPE_FACE: {
                    objname = "FACE";
                    color = graphics_Blue;
                } break;
                case ROCKIVA_OBJECT_TYPE_PERSON: {
                    objname = "PERSON";
                    color = graphics_Red;
                } break;
                case ROCKIVA_OBJECT_TYPE_HEAD: {
                    objname = "HEAD";
                    color = graphics_Yellow;
                } break;
                case ROCKIVA_OBJECT_TYPE_PLATE: {
                    objname = "PLATE";
                    color = graphics_Cyan;
                } break;
                case ROCKIVA_OBJECT_TYPE_VEHICLE: {
                    objname = "VEHICLE";
                    color = graphics_Magenta;
                } break;
                case ROCKIVA_OBJECT_TYPE_NON_VEHICLE: {
                    objname = "NON_VEHICLE";
                    color = graphics_Magenta;
                } break;
                case ROCKIVA_OBJECT_TYPE_BICYCLE: {
                    objname = "BICYCLE";
                    color = graphics_Magenta;
                } break;
                case ROCKIVA_OBJECT_TYPE_MOTORCYCLE: {
                    objname = "MOTORCYCLE";
                    color = graphics_Magenta;
                } break;
                default: {
                    printf("Warning: obj type NONE %d", g_screen_iva_ctx.objInfo[i].type);
                    objname = "NONE";
                } break;
            }

            X1 = ROCKIVA_RATIO_PIXEL_CONVERT(Fvi_info->width, g_screen_iva_ctx.objInfo[i].rect.topLeft.x);
            Y1 = ROCKIVA_RATIO_PIXEL_CONVERT(Fvi_info->height, g_screen_iva_ctx.objInfo[i].rect.topLeft.y);
            X2 = ROCKIVA_RATIO_PIXEL_CONVERT(Fvi_info->width, g_screen_iva_ctx.objInfo[i].rect.bottomRight.x);
            Y2 = ROCKIVA_RATIO_PIXEL_CONVERT(Fvi_info->height, g_screen_iva_ctx.objInfo[i].rect.bottomRight.y);

            if (X1 > Fvi_info->width || Y1 > Fvi_info->height || X2 > Fvi_info->width || Y2 > Fvi_info->height) {
                // printf("[%s %d] error: ---\n", __func__, __LINE__);
                // printf("obj:%d/%d %s X1:%u Y1:%u X2:%u Y2:%u\n", i + 1, g_screen_iva_ctx.objNum, objname, (uint16_t)X1, (uint16_t)Y2, (uint16_t)X2, (uint16_t)Y2);
            } else {
                // printf("req:%d objNum:%d/%d %s %d%% X1:%u Y1:%u X2:%u Y2:%u\n", g_screen_iva_ctx.objInfo[i].frameId, i + 1, g_screen_iva_ctx.objNum, objname, g_screen_iva_ctx.objInfo[i].score, (uint16_t)X1, (uint16_t)Y2, (uint16_t)X2, (uint16_t)Y2);

                snprintf(text_buf, sizeof(text_buf), "%s %d%%", objname, g_screen_iva_ctx.objInfo[i].score);
                graphics_rectangle(&screen_graphics_image, X1, Y1, X2, Y2, color);
                graphics_show_string(&screen_graphics_image, X1 + 4, Y1 + 4, text_buf, GD_FONT_16x32B, color);
            }
        }
    } else if (type == GET_IVA_FRAME) {

        if (g_video_param_list_ctx.iva[0].enable) {
            s32Ret = rv1106_vichn_GetStream_fd(&g_video_param_list_ctx.vi_chn[1], Fvi_info);
            if (s32Ret != RK_SUCCESS) {
                printf("error: rv1106_vichn_GetStream_fd fail: ret:0x%X\n", s32Ret);
                return s32Ret;
            }

            rv1106_iva_push_frame_fd(&g_video_param_list_ctx.iva[0], Fvi_info);

            s32Ret = rv1106_vichn_ReleaseStream_fd(&g_video_param_list_ctx.vi_chn[1], Fvi_info);
            if (s32Ret != RK_SUCCESS) {
                printf("error: rv1106_vichn_ReleaseStream_fd fail: ret:0x%X\n", s32Ret);
                return s32Ret;
            }
        } else {
            s32Ret = rv1106_vichn_GetStream(&g_video_param_list_ctx.vi_chn[1], Fvi_info);
            if (s32Ret != RK_SUCCESS) {
                printf("error: GET_IVA_FRAME fail: ret:0x%X\n", s32Ret);
                return s32Ret;
            }
        }
    }

    return s32Ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
