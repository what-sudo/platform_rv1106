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

#include "rv1106_isp.h"
#include "rv1106_vi.h"
#include "rv1106_vpss.h"
#include "rv1106_venc.h"
#include "rv1106_iva.h"
#include "rv1106_rgn.h"

#include "graphics_Draw.h"

#define SENSOR_WIDTH     2304
#define SENSOR_HEIGHT    1296

#define VIDEO_INIT_CHNNEL_MAX   2
#define VIDEO_INIT_VENC_MAX   2
typedef struct {
    video_isp_param_t isp[1];
    video_vi_dev_param_t vi_dev[1];
    video_vi_chn_param_t vi_chn[VIDEO_INIT_CHNNEL_MAX];
    video_vpss_param_t vpss[1];
    video_venc_param_t venc[VIDEO_INIT_VENC_MAX];
    video_rgn_param_t rgn[8];
} video_init_param_t;

int rv1106_iva_result_cb(video_iva_callback_param_t *ctx);

video_init_param_t g_video_param_list_ctx = {
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
            .enable = 0,
            .ViPipe = 0,
            .viChnId = 1,
            .width = 960,
            .height = 540,
            .SrcFrameRate = -1,
            .DstFrameRate = -1,
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
            .enable = 0,
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
            .enable = 0,
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
    }
};

#define RK_RGN 1
#define RK_IVA 1

#if RK_IVA
static video_iva_param_t iva = {
    .enable = 0,
    .models_path = "/userdata/rockiva_data/",
    .width = 960,
    .height = 540,
    .IvaPixelFormat = ROCKIVA_IMAGE_FORMAT_YUV420SP_NV12,
    // .result_cb = rv1106_iva_result_cb,
};
#endif

graphics_image_t g_graphics_image = {0};

extern const unsigned char ascii_8x16[][16];
extern const unsigned char ascii_16x32[][64];

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

int rv1106_iva_result_cb(video_iva_callback_param_t *ctx)
{
    int i;
    RK_S32 s32Ret = RK_FAILURE;
    RGN_CANVAS_INFO_S CanvasInfo = {0};
    video_rgn_param_t *rgn = &g_video_param_list_ctx.rgn[0];
    RK_U32 X1, Y1, X2, Y2;
    uint32_t color = {0};
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
                printf("[%s %d] error: ---\n", __func__, __LINE__);
                printf("obj:%d/%d %s X1:%u Y1:%u X2:%u Y2:%u\n", i + 1, ctx->objNum, objname, (uint16_t)X1, (uint16_t)Y2, (uint16_t)X2, (uint16_t)Y2);
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
    int i;

    printf("#CameraIdx: %d\n", g_video_param_list_ctx.vi_dev[0].ViDevId);
    printf("#IQ Path: %s\n", g_video_param_list_ctx.isp[0].iq_file_dir);

    s32Ret = RV1106_ISP_init(&g_video_param_list_ctx.isp[0]);
    if (s32Ret != RK_SUCCESS) {
        printf("error: RK_MPI_SYS_Init fail, s32Ret:%d\n", s32Ret);
        return s32Ret;
    }

    do {
        s32Ret = RK_MPI_SYS_Init();
        if (s32Ret != RK_SUCCESS) {
            printf("error: RK_MPI_SYS_Init fail, s32Ret:%d\n", s32Ret);
            break;
        }

        if (rv1106_vi_dev_init(g_video_param_list_ctx.vi_dev)) {
            printf("error: vi_dev_init fail \n");
            break;
        }

        for (i = 0; i < VIDEO_INIT_CHNNEL_MAX; i++) {
            if (g_video_param_list_ctx.vi_chn[i].enable) {
                printf(">>>>>>>>>>>>>>>>>>>>>>> rv1106_vi_chn_init index:%d \n", i);
                s32Ret = rv1106_vi_chn_init(&g_video_param_list_ctx.vi_chn[i]);
                if (s32Ret != RK_SUCCESS) {
                    printf("error: rv1106_vi_chn_init fail chn:%d s32Ret:0x%X\n", g_video_param_list_ctx.vi_chn[i].viChnId, s32Ret);
                    break;
                }
                printf(">>> rv1106_vi_chn_init index:%d OK\n", i);
            }
        }
        if (s32Ret) break;

        for (i = 0; i < sizeof(g_video_param_list_ctx.vpss) / sizeof(video_vpss_param_t); i++) {
            if (g_video_param_list_ctx.vpss[i].enable) {
                printf(">>>>>>>>>>>>>>>>>>>>>>> rv1106_vpss_init index:%d \n", i);
                s32Ret = rv1106_vpss_init(&g_video_param_list_ctx.vpss[i]);
                if (s32Ret != RK_SUCCESS) {
                    printf("error: rv1106_vpss_init fail grp:%d s32Ret:0x%X\n", g_video_param_list_ctx.vpss[i].VpssGrpID, s32Ret);
                    break;
                }
                printf(">>> rv1106_vpss_init index:%d OK\n", i);
            }
        }
        if (s32Ret) break;

        for (i = 0; i < VIDEO_INIT_VENC_MAX; i++) {
            if (g_video_param_list_ctx.venc[i].enable) {
                printf(">>>>>>>>>>>>>>>>>>>>>>> rv1106_venc_init index:%d \n", i);
                s32Ret = rv1106_venc_init(&g_video_param_list_ctx.venc[i]);
                if (s32Ret != RK_SUCCESS) {
                    printf("error: rv1106_venc_init fail chn:%d s32Ret:0x%X\n", g_video_param_list_ctx.venc[i].vencChnId, s32Ret);
                    break;
                }
                printf(">>> rv1106_venc_init index:%d OK\n", i);
            }
        }
        if (s32Ret) break;
#if RK_RGN
        for (i = 0; i < sizeof(g_video_param_list_ctx.rgn) / sizeof(video_rgn_param_t); i++) {
            if (g_video_param_list_ctx.rgn[i].enable) {
                printf(">>> rv1106_rgn_init index:%d \n", i);
                s32Ret = rv1106_rgn_init(&g_video_param_list_ctx.rgn[i]);
                if (s32Ret != RK_SUCCESS) {
                    printf("[%s %d] error: rv1106_rgn_init ret:0x%X\n", __func__, __LINE__, s32Ret);
                    break;
                }
                printf("rv1106_rgn_init OK\n");
            }
        }
        if (s32Ret) break;

        if (g_video_param_list_ctx.rgn[0].enable) {
            s32Ret = osd_init();
            if (s32Ret != RK_SUCCESS) {
                printf("[%s %d] error: rv1106_rgn_init ret:0x%X\n", __func__, __LINE__, s32Ret);
                break;
            }
        }
#endif

#if RK_IVA
        if (iva.enable) {
            s32Ret = rv1106_iva_init(&iva);
            if (s32Ret != RK_SUCCESS) {
                printf("[%s %d] error: rv1106_iva_init s32Ret:0x%X\n", __func__, __LINE__, s32Ret);
                break;
            }
        }
#endif

        printf("%s initial finish\n", __func__);
        s32Ret = 0;
    } while (0);

    if (s32Ret) {
        printf("error: s32Ret:0x%X\n", s32Ret);
    }

    return s32Ret;
}

int video_deinit(void)
{
    int i;
    RK_S32 s32Ret = RK_FAILURE;

#if RK_IVA
        if (iva.enable) {
            s32Ret = rv1106_iva_deinit(&iva);
            if (s32Ret != RK_SUCCESS) {
                printf("[%s %d] error: rv1106_iva_deinit s32Ret:0x%X\n", __func__, __LINE__, s32Ret);
                return s32Ret;
            }
        }
#endif

#if RK_RGN
    for (i = 0; i < sizeof(g_video_param_list_ctx.rgn) / sizeof(video_rgn_param_t); i++) {
        if (g_video_param_list_ctx.rgn[i].enable) {
            printf(">>> rv1106_rgn_deinit index:%d \n", i);
            s32Ret = rv1106_rgn_deinit(&g_video_param_list_ctx.rgn[i]);
            if (s32Ret != RK_SUCCESS) {
                printf("[%s %d] error: rv1106_rgn_deinit ret:0x%X\n", __func__, __LINE__, s32Ret);
                break;
            }
            printf("rv1106_rgn_deinit OK\n");
        }
    }
#endif

    for (i = 0; i < VIDEO_INIT_VENC_MAX; i++) {
        if (g_video_param_list_ctx.venc[i].enable) {
            printf(">>> rv1106_venc_deinit index:%d \n", i);
            s32Ret = rv1106_venc_deinit(&g_video_param_list_ctx.venc[i]);
            if (s32Ret != RK_SUCCESS) {
                printf("error: rv1106_venc_deinit fail! chn: %d s32Ret=%d\n", g_video_param_list_ctx.venc[i].vencChnId, s32Ret);
                return s32Ret;
            }
            printf("rv1106_venc_deinit OK\n");
        }
    }

    for (i = 0; i < sizeof(g_video_param_list_ctx.vpss) / sizeof(video_vpss_param_t); i++) {
        if (g_video_param_list_ctx.vpss[i].enable) {
            printf(">>> rv1106_vpss_deinit index:%d \n", i);
            s32Ret = rv1106_vpss_deinit(&g_video_param_list_ctx.vpss[i]);
            if (s32Ret != RK_SUCCESS) {
                printf("error: rv1106_vpss_deinit fail grp:%d s32Ret:0x%X\n", g_video_param_list_ctx.vpss[i].VpssGrpID, s32Ret);
                return s32Ret;
            }
            printf("rv1106_vpss_deinit OK\n");
        }
    }

    for (i = 0; i < VIDEO_INIT_CHNNEL_MAX; i++) {
        if (g_video_param_list_ctx.vi_chn[i].enable) {
            printf(">>> rv1106_vi_chn_deinit index:%d \n", i);
            s32Ret = rv1106_vi_chn_deinit(&g_video_param_list_ctx.vi_chn[i]);
            if (s32Ret != RK_SUCCESS) {
                printf("error: rv1106_venc_deinit fail! chn: %d s32Ret=%d\n", g_video_param_list_ctx.vi_chn[i].viChnId, s32Ret);
                return s32Ret;
            }
            printf("rv1106_vi_chn_deinit OK\n");
        }
    }

    printf(">>> rv1106_vi_dev_deinit index:%d \n", 0);
    s32Ret = rv1106_vi_dev_deinit(&g_video_param_list_ctx.vi_dev[0]);
    if (s32Ret != RK_SUCCESS) {
        printf("error: rv1106_vi_dev_deinit fail! vi dev: %d s32Ret=%d\n", g_video_param_list_ctx.vi_dev[0].ViDevId, s32Ret);
        return s32Ret;
    }
    printf("rv1106_vi_dev_deinit OK\n");

    printf(">>> RK_MPI_SYS_Exit \n");
    s32Ret = RK_MPI_SYS_Exit();
    if (s32Ret != RK_SUCCESS) {
        printf("error: RK_MPI_SYS_Exit fail, s32Ret:%d\n", s32Ret);
        return s32Ret;
    }
    printf("RK_MPI_SYS_Exit OK\n");

    printf(">>> RV1106_ISP_deinit \n");
    RV1106_ISP_deinit(&g_video_param_list_ctx.isp[0]);
    printf("RV1106_ISP_deinit OK\n");

    return s32Ret;
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

#if RK_IVA
        s32Ret = rv1106_vichn_GetStream_fd(&g_video_param_list_ctx.vi_chn[1], Fvi_info);
        if (s32Ret != RK_SUCCESS) {
            printf("error: rv1106_vichn_GetStream_fd fail: ret:0x%X\n", s32Ret);
            return s32Ret;
        }

        rv1106_iva_push_frame_fd(&iva, Fvi_info);

        s32Ret = rv1106_vichn_ReleaseStream_fd(&g_video_param_list_ctx.vi_chn[1], Fvi_info);
        if (s32Ret != RK_SUCCESS) {
            printf("error: rv1106_vichn_ReleaseStream_fd fail: ret:0x%X\n", s32Ret);
            return s32Ret;
        }
#else
        s32Ret = rv1106_vichn_GetStream(&g_video_param_list_ctx.vi_chn[1], Fvi_info);
        if (s32Ret != RK_SUCCESS) {
            printf("error: GET_IVA_FRAME fail: ret:0x%X\n", s32Ret);
            return s32Ret;
        }

#endif

    }

    return s32Ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
