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

#include "video.h"

#include "rv1106_isp.h"
#include "rv1106_vi.h"
#include "rv1106_vpss.h"
#include "rv1106_venc.h"

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
} video_init_param_t;

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
            .PixelFormat = RK_FMT_YUV420SP,
        },
        {
            .enable = 0,
            .ViPipe = 0,
            .viChnId = 1,
            .width = 800,
            .height = 480,
            .PixelFormat = RK_FMT_XBGR8888,
        },
    },
    .vpss = {
        {
            .enable = 1,
            .VpssGrpID = 0,
            .inWidth = SENSOR_WIDTH,
            .inHeight = SENSOR_HEIGHT,
            .inPixelFormat = RK_FMT_YUV420SP,
            .bindSrcChn = {RK_ID_VI, 0, 0},
            .chn = {
                {
                    .enable = 1,
                    .VpssChnID = 0,
                    .outWidth = 800,
                    .outHeight = 480,
                    .outPixelFormat = RK_FMT_RGB888,
                    .bMirror = true,
                    .bFlip = true,
                },
                {
                    .enable = 1,
                    .VpssChnID = 1,
                    .outWidth = SENSOR_WIDTH,
                    .outHeight = SENSOR_HEIGHT,
                    .outPixelFormat = RK_FMT_YUV420SP,
                    .bMirror = false,
                    .bFlip = false,
                },
            },
        },
    },
    .venc = {
        {
            .enable = 1,
            .enType = RK_VIDEO_ID_AVC,
            .vencChnId = 0,
            .width = SENSOR_WIDTH,
            .height = SENSOR_HEIGHT,
            .PixelFormat = RK_FMT_YUV420SP,
            .bindSrcChn = {RK_ID_VPSS, 0, 1},
        },
        {
            .enable = 0
        },
    },
};

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
                break;
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
            printf("error: rv1106_venc_GetStream fail: venc chn:%d ret:0x%x\n", g_video_param_list_ctx.venc[0].vencChnId, s32Ret);
            return s32Ret;
        }
    } else if (type == GET_SCREEN_FRAME) {
        // s32Ret = rv1106_vichn_GetStream(&g_video_param_list_ctx.vi_chn[1], Fvi_info);
        // if (s32Ret != RK_SUCCESS) {
        //     printf("error: rv1106_vichn_GetStream fail: vi chn:%d ret:0x%x\n", g_video_param_list_ctx.vi_chn[1].viChnId, s32Ret);
        //     return s32Ret;
        // }

        s32Ret = rv1106_vpss_GetStream(&g_video_param_list_ctx.vpss[0], g_video_param_list_ctx.vpss[0].chn[0].VpssChnID, Fvi_info);
        if (s32Ret != RK_SUCCESS) {
            printf("error: rv1106_vpss_GetStream fail: vpss grp:%d ret:0x%X\n", g_video_param_list_ctx.vpss[0].VpssGrpID, s32Ret);
            return s32Ret;
        }

    } else if (type == GET_test1_FRAME) {
        s32Ret = rv1106_vpss_GetStream(&g_video_param_list_ctx.vpss[0], g_video_param_list_ctx.vpss[0].chn[0].VpssChnID, Fvi_info);
        if (s32Ret != RK_SUCCESS) {
            printf("error: rv1106_vpss_GetStream fail: vpss grp:%d ret:0x%X\n", g_video_param_list_ctx.vpss[0].VpssGrpID, s32Ret);
            return s32Ret;
        }
    } else if (type == GET_test2_FRAME) {
        s32Ret = rv1106_vpss_GetStream(&g_video_param_list_ctx.vpss[0], g_video_param_list_ctx.vpss[0].chn[1].VpssChnID, Fvi_info);
        if (s32Ret != RK_SUCCESS) {
            printf("error: rv1106_vpss_GetStream fail: vpss grp:%d ret:0x%X\n", g_video_param_list_ctx.vpss[0].VpssGrpID, s32Ret);
            return s32Ret;
        }
    }

    return s32Ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
