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

int rv1106_iva_result_cb(video_iva_callback_param_t *ctx);

rv1106_video_init_param_t g_video_param_list_ctx = {
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

graphics_image_t g_graphics_image = {0};

int osd_init(void)
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

int rv1106_iva_result_cb(video_iva_callback_param_t *ctx)
{
    int i;
    RK_S32 s32Ret = RK_FAILURE;
    RGN_CANVAS_INFO_S CanvasInfo = {0};
    video_rgn_param_t *rgn = &g_video_param_list_ctx.rgn[0];
    RK_U32 X1, Y1, X2, Y2;
    graphics_color_t color = {0};
    char text_buf[32] = { 0 };

    s32Ret = rv1106_rgn_overlay_get_canvas(rgn, &CanvasInfo);
    if (s32Ret != RK_SUCCESS) {
        printf("[%s %d] error: rv1106_rgn_overlay_get_canvas ret:0x%X\n", __func__, __LINE__, s32Ret);
        return s32Ret;
    }
    g_graphics_image.buf = (uint8_t*)(uint32_t)(CanvasInfo.u64VirAddr);
    graphics_full(&g_graphics_image, graphics_Clear);

    if (ctx->objNum) {
        char *objname = "NONE";

        // static uint64_t last_timestamp = 0;
        // uint64_t new_timestamp = TEST_COMM_GetNowUs();
        // printf("IVA result ---> seq:%d delay:%dms fps:%.1f\n", ctx->objInfo[0].frameId, (uint32_t)(new_timestamp - last_timestamp) / 1000, (1000.0 / ((new_timestamp - last_timestamp) / 1000)));
        // last_timestamp = new_timestamp;

        for (i = 0; i < ctx->objNum; i++) {
            switch (ctx->objInfo[i].type)
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
                    printf("Warning: obj type NONE %d", ctx->objInfo[i].type);
                    objname = "NONE";
                } break;
            }

            X1 = ROCKIVA_RATIO_PIXEL_CONVERT(SENSOR_WIDTH, ctx->objInfo[i].rect.topLeft.x);
            Y1 = ROCKIVA_RATIO_PIXEL_CONVERT(SENSOR_HEIGHT, ctx->objInfo[i].rect.topLeft.y);
            X2 = ROCKIVA_RATIO_PIXEL_CONVERT(SENSOR_WIDTH, ctx->objInfo[i].rect.bottomRight.x);
            Y2 = ROCKIVA_RATIO_PIXEL_CONVERT(SENSOR_HEIGHT, ctx->objInfo[i].rect.bottomRight.y);

            if (X1 > SENSOR_WIDTH || Y1 > SENSOR_HEIGHT || X2 > SENSOR_WIDTH || Y2 > SENSOR_HEIGHT) {
                // printf("[%s %d] error: ---\n", __func__, __LINE__);
                // printf("obj:%d/%d %s X1:%u Y1:%u X2:%u Y2:%u\n", i + 1, ctx->objNum, objname, (uint16_t)X1, (uint16_t)Y2, (uint16_t)X2, (uint16_t)Y2);
            } else {
                printf("req:%d objNum:%d/%d %s X1:%u Y1:%u X2:%u Y2:%u\n", ctx->objInfo[i].frameId, i + 1, ctx->objNum, objname, (uint16_t)X1, (uint16_t)Y2, (uint16_t)X2, (uint16_t)Y2);
                graphics_rectangle(&g_graphics_image, X1, Y1, X2, Y2, color);
                snprintf(text_buf, sizeof(text_buf), "%s %d%%", objname, ctx->objInfo[i].score);
                graphics_show_string(&g_graphics_image, X1 + 4, Y1 + 4, text_buf, GD_FONT_16x32B, color);
            }
        }
    }

    s32Ret = rv1106_rgn_overlay_set_canvas(rgn, &CanvasInfo);
    if (s32Ret != RK_SUCCESS) {
        printf("[%s %d] error: rv1106_rgn_overlay_set_canvas ret:0x%X\n", __func__, __LINE__, s32Ret);
        return s32Ret;
    }
    return s32Ret;
}

int video_init(void)
{
    RK_S32 s32Ret = RK_FAILURE;
    s32Ret = rv1106_video_init(&g_video_param_list_ctx);
    if (s32Ret != RK_SUCCESS) {
        printf("[%s %d] error: rv1106_video_init fail: ret:0x%x\n", __func__, __LINE__, s32Ret);
        return s32Ret;
    }

    s32Ret = osd_init();
    if (s32Ret != RK_SUCCESS) {
        printf("[%s %d] error: osd_init fail: ret:0x%x\n", __func__, __LINE__, s32Ret);
        return s32Ret;
    }

    return s32Ret;
}

int video_deinit(void)
{
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
        s32Ret = rv1106_vpss_GetStream(&g_video_param_list_ctx.vpss[0], g_video_param_list_ctx.vpss[0].chn[0].VpssChnID, Fvi_info);
        if (s32Ret != RK_SUCCESS) {
            printf("error: GET_SCREEN_FRAME fail: vpss grp:%d ret:0x%X\n", g_video_param_list_ctx.vpss[0].VpssGrpID, s32Ret);
            return s32Ret;
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
