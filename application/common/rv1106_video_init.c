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

#include "rv1106_video_init.h"

int rv1106_video_init(rv1106_video_init_param_t *video_param)
{
    RK_S32 s32Ret = RK_FAILURE;
    int i;

    printf("#CameraIdx: %d\n", video_param->vi_dev[0].ViDevId);
    printf("#IQ Path: %s\n", video_param->isp[0].iq_file_dir);


    if (video_param->isp[0].enable) {
        s32Ret = RV1106_ISP_init(&video_param->isp[0]);
        if (s32Ret != RK_SUCCESS) {
            printf("error: RK_MPI_SYS_Init fail, s32Ret:%#X\n", s32Ret);
            return s32Ret;
        }
    }

    do {
        s32Ret = RK_MPI_SYS_Init();
        if (s32Ret != RK_SUCCESS) {
            printf("error: RK_MPI_SYS_Init fail, s32Ret:%d\n", s32Ret);
            break;
        }

        for (i = 0; i < sizeof(video_param->vi_dev) / sizeof(video_vi_dev_param_t); i++) {
            if (video_param->vi_dev[i].enable) {
                if (rv1106_vi_dev_init(video_param->vi_dev)) {
                    printf("error: vi_dev_init fail \n");
                    break;
                }
            }
        }
        if (s32Ret) break;

        for (i = 0; i < sizeof(video_param->vi_chn) / sizeof(video_vi_chn_param_t); i++) {
            if (video_param->vi_chn[i].enable) {
                printf(">>>>>>>>>>>>>>>>>>>>>>> rv1106_vi_chn_init index:%d \n", i);
                s32Ret = rv1106_vi_chn_init(&video_param->vi_chn[i]);
                if (s32Ret != RK_SUCCESS) {
                    printf("error: rv1106_vi_chn_init fail chn:%d s32Ret:0x%X\n", video_param->vi_chn[i].viChnId, s32Ret);
                    break;
                }
                printf(">>> rv1106_vi_chn_init index:%d OK\n", i);
            }
        }
        if (s32Ret) break;

        for (i = 0; i < sizeof(video_param->vpss) / sizeof(video_vpss_param_t); i++) {
            if (video_param->vpss[i].enable) {
                printf(">>>>>>>>>>>>>>>>>>>>>>> rv1106_vpss_init index:%d \n", i);
                s32Ret = rv1106_vpss_init(&video_param->vpss[i]);
                if (s32Ret != RK_SUCCESS) {
                    printf("error: rv1106_vpss_init fail grp:%d s32Ret:0x%X\n", video_param->vpss[i].VpssGrpID, s32Ret);
                    break;
                }
                printf(">>> rv1106_vpss_init index:%d OK\n", i);
            }
        }
        if (s32Ret) break;

        for (i = 0; i < sizeof(video_param->venc) / sizeof(video_venc_param_t); i++) {
            if (video_param->venc[i].enable) {
                printf(">>>>>>>>>>>>>>>>>>>>>>> rv1106_venc_init index:%d \n", i);
                s32Ret = rv1106_venc_init(&video_param->venc[i]);
                if (s32Ret != RK_SUCCESS) {
                    printf("error: rv1106_venc_init fail chn:%d s32Ret:0x%X\n", video_param->venc[i].vencChnId, s32Ret);
                    break;
                }
                printf(">>> rv1106_venc_init index:%d OK\n", i);
            }
        }
        if (s32Ret) break;

        for (i = 0; i < sizeof(video_param->rgn) / sizeof(video_rgn_param_t); i++) {
            if (video_param->rgn[i].enable) {
                printf(">>> rv1106_rgn_init index:%d \n", i);
                s32Ret = rv1106_rgn_init(&video_param->rgn[i]);
                if (s32Ret != RK_SUCCESS) {
                    printf("[%s %d] error: rv1106_rgn_init ret:0x%X\n", __func__, __LINE__, s32Ret);
                    break;
                }
                printf("rv1106_rgn_init OK\n");
            }
        }
        if (s32Ret) break;

        if (video_param->iva[0].enable) {
            s32Ret = rv1106_iva_init(&video_param->iva[0]);
            if (s32Ret != RK_SUCCESS) {
                printf("[%s %d] error: rv1106_iva_init s32Ret:0x%X\n", __func__, __LINE__, s32Ret);
                break;
            }
        }

        printf("%s initial finish\n", __func__);
        s32Ret = 0;
    } while (0);

    if (s32Ret) {
        printf("error: s32Ret:0x%X\n", s32Ret);
    }

    return s32Ret;
}

int rv1106_video_deinit(rv1106_video_init_param_t *video_param)
{
    int i;
    RK_S32 s32Ret = RK_FAILURE;

    if (video_param->iva[0].enable) {
        s32Ret = rv1106_iva_deinit(&video_param->iva[0]);
        if (s32Ret != RK_SUCCESS) {
            printf("[%s %d] error: rv1106_iva_deinit s32Ret:0x%X\n", __func__, __LINE__, s32Ret);
            return s32Ret;
        }
    }

    for (i = 0; i < sizeof(video_param->rgn) / sizeof(video_rgn_param_t); i++) {
        if (video_param->rgn[i].enable) {
            printf(">>> rv1106_rgn_deinit index:%d \n", i);
            s32Ret = rv1106_rgn_deinit(&video_param->rgn[i]);
            if (s32Ret != RK_SUCCESS) {
                printf("[%s %d] error: rv1106_rgn_deinit ret:0x%X\n", __func__, __LINE__, s32Ret);
                break;
            }
            printf("rv1106_rgn_deinit OK\n");
        }
    }

    for (i = 0; i < sizeof(video_param->venc) / sizeof(video_venc_param_t); i++) {
        if (video_param->venc[i].enable) {
            printf(">>> rv1106_venc_deinit index:%d \n", i);
            s32Ret = rv1106_venc_deinit(&video_param->venc[i]);
            if (s32Ret != RK_SUCCESS) {
                printf("error: rv1106_venc_deinit fail! chn: %d s32Ret=%d\n", video_param->venc[i].vencChnId, s32Ret);
                return s32Ret;
            }
            printf("rv1106_venc_deinit OK\n");
        }
    }

    for (i = 0; i < sizeof(video_param->vpss) / sizeof(video_vpss_param_t); i++) {
        if (video_param->vpss[i].enable) {
            printf(">>> rv1106_vpss_deinit index:%d \n", i);
            s32Ret = rv1106_vpss_deinit(&video_param->vpss[i]);
            if (s32Ret != RK_SUCCESS) {
                printf("error: rv1106_vpss_deinit fail grp:%d s32Ret:0x%X\n", video_param->vpss[i].VpssGrpID, s32Ret);
                return s32Ret;
            }
            printf("rv1106_vpss_deinit OK\n");
        }
    }

    for (i = 0; i < sizeof(video_param->vi_chn) / sizeof(video_vi_chn_param_t); i++) {
        if (video_param->vi_chn[i].enable) {
            printf(">>> rv1106_vi_chn_deinit index:%d \n", i);
            s32Ret = rv1106_vi_chn_deinit(&video_param->vi_chn[i]);
            if (s32Ret != RK_SUCCESS) {
                printf("error: rv1106_venc_deinit fail! chn: %d s32Ret=%d\n", video_param->vi_chn[i].viChnId, s32Ret);
                return s32Ret;
            }
            printf("rv1106_vi_chn_deinit OK\n");
        }
    }

    for (i = 0; i < sizeof(video_param->vi_dev) / sizeof(video_vi_dev_param_t); i++) {
        if (video_param->vi_dev[i].enable) {
            printf(">>> rv1106_vi_dev_deinit index:%d \n", i);
            s32Ret = rv1106_vi_dev_deinit(&video_param->vi_dev[i]);
            if (s32Ret != RK_SUCCESS) {
                printf("error: rv1106_vi_dev_deinit fail! vi dev: %d s32Ret=%d\n", video_param->vi_dev[i].ViDevId, s32Ret);
                return s32Ret;
            }
            printf("rv1106_vi_dev_deinit OK\n");
        }
    }

    printf(">>> RK_MPI_SYS_Exit \n");
    s32Ret = RK_MPI_SYS_Exit();
    if (s32Ret != RK_SUCCESS) {
        printf("error: RK_MPI_SYS_Exit fail, s32Ret:%d\n", s32Ret);
        return s32Ret;
    }
    printf("RK_MPI_SYS_Exit OK\n");

    if (video_param->isp[0].enable) {
        printf(">>> RV1106_ISP_deinit \n");
        RV1106_ISP_deinit(&video_param->isp[0]);
        printf("RV1106_ISP_deinit OK\n");
    }

    return s32Ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
