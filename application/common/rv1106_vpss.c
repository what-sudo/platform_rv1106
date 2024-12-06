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
#include <stdatomic.h>

#include "rv1106_common.h"
#include "rv1106_vpss.h"

#include "rk_mpi_vpss.h"

int rv1106_vpss_init(video_vpss_param_t *vpss)
{
    int i = 0;
    RK_S32 s32Ret = RK_FAILURE;

    video_vpss_chn_param_t *chn;

    VPSS_GRP_ATTR_S stGrpVpssAttr;
    VPSS_CHN_ATTR_S stVpssChnAttr;
    VPSS_CROP_INFO_S pstgrpCropInfo;
    VPSS_CROP_INFO_S pstchnCropInfo;

    if (vpss->enable == 0) {
        printf("error: vpss not enable, vpss Grp:%d\n", vpss->VpssGrpID);
        return s32Ret;
    }

    memset(&stGrpVpssAttr, 0, sizeof(VPSS_GRP_ATTR_S));

    chn = vpss->chn;

    do {
        stGrpVpssAttr.u32MaxW = vpss->inWidth;
        stGrpVpssAttr.u32MaxH = vpss->inHeight;
        stGrpVpssAttr.enPixelFormat = vpss->inPixelFormat;
        stGrpVpssAttr.stFrameRate.s32SrcFrameRate = vpss->SrcFrameRate;
        stGrpVpssAttr.stFrameRate.s32DstFrameRate = vpss->DstFrameRate;
        stGrpVpssAttr.enCompressMode = COMPRESS_MODE_NONE;
        stGrpVpssAttr.enDynamicRange = DYNAMIC_RANGE_SDR8;

        s32Ret = RK_MPI_VPSS_CreateGrp(vpss->VpssGrpID, &stGrpVpssAttr);
        if (s32Ret != RK_SUCCESS) {
            printf("ERROR: RK_MPI_VPSS_CreateGrp fail! group:%d ret=%d\n", vpss->VpssGrpID, s32Ret);
            break;
        }

        s32Ret = RK_MPI_VPSS_SetVProcDev(vpss->VpssGrpID, VIDEO_PROC_DEV_RGA);
        if (s32Ret != RK_SUCCESS) {
            printf("ERROR: RK_MPI_VPSS_SetVProcDev(grp:%d) failed with %#x!", vpss->VpssGrpID, s32Ret);
           break;
        }

        // VIDEO_PROC_DEV_TYPE_E enTmpVProcDevType;
        // s32Ret = RK_MPI_VPSS_GetVProcDev(vpss->VpssGrpID, &enTmpVProcDevType);
        // if (s32Ret != RK_SUCCESS) {
        //     printf("RK_MPI_VPSS_GetVProcDev(grp:%d) failed with %#x!", vpss->VpssGrpID, s32Ret);
        //     break;
        // }
        // printf("vpss Grp %d's work unit is %d\n", vpss->VpssGrpID, enTmpVProcDevType);

        s32Ret = RK_MPI_VPSS_ResetGrp(vpss->VpssGrpID);
        if (s32Ret != RK_SUCCESS) {
            RK_LOGE("RK_MPI_VPSS_ResetGrp failed with %#x!\n", s32Ret);
            break;
        }

        pstgrpCropInfo.bEnable = RK_FALSE;
        pstgrpCropInfo.enCropCoordinate = VPSS_CROP_RATIO_COOR;
        pstgrpCropInfo.stCropRect.s32X = 0;
        pstgrpCropInfo.stCropRect.s32Y = 0;
        pstgrpCropInfo.stCropRect.u32Width = vpss->inWidth * 1000 /  vpss->inWidth;
        pstgrpCropInfo.stCropRect.u32Height = vpss->inHeight  * 1000 /  vpss->inHeight;

        s32Ret = RK_MPI_VPSS_SetGrpCrop(vpss->VpssGrpID, &pstgrpCropInfo);
        if (s32Ret != RK_SUCCESS) {
            RK_LOGE("RK_MPI_VPSS_SetGrpCrop failed with %#x!\n", s32Ret);
            break;
        }
        // s32Ret = RK_MPI_VPSS_GetGrpCrop(vpss->VpssGrpID, &pstgrpCropInfo);
        // if (s32Ret != RK_SUCCESS) {
        //     RK_LOGE("RK_MPI_VPSS_GetGrpCrop failed with %#x!\n", s32Ret);
        //     break;
        // }

        for (i = 0; i < sizeof(vpss->chn) / sizeof(video_vpss_chn_param_t); i++) {
            if (chn[i].enable == 0) {
                continue;
            }

            printf("====VPSS init grp:%d channel: %d ==== \n", vpss->VpssGrpID, chn[i].VpssChnID);

           pstchnCropInfo.bEnable = chn[i].Crop;
           pstchnCropInfo.enCropCoordinate = VPSS_CROP_RATIO_COOR;
           pstchnCropInfo.stCropRect.s32X = chn[i].X;
           pstchnCropInfo.stCropRect.s32Y = chn[i].Y;
           pstchnCropInfo.stCropRect.u32Width = chn[i].outWidth * 1000 / vpss->inWidth;
           pstchnCropInfo.stCropRect.u32Height = chn[i].outHeight * 1000 / vpss->inHeight;

            s32Ret = RK_MPI_VPSS_SetChnCrop(vpss->VpssGrpID, chn[i].VpssChnID, &pstchnCropInfo);
            if (s32Ret != RK_SUCCESS) {
                RK_LOGE("RK_MPI_VPSS_SetChnCrop failed with %#x!\n", s32Ret);
                return s32Ret;
            }
            // s32Ret = RK_MPI_VPSS_GetChnCrop(vpss->VpssGrpID, chn[i].VpssChnID, &pstchnCropInfo);
            // if (s32Ret != RK_SUCCESS) {
            //     RK_LOGE("RK_MPI_VPSS_GetChnCrop failed with %#x!\n", s32Ret);
            //     return s32Ret;
            // }

            stVpssChnAttr.enChnMode = VPSS_CHN_MODE_USER;
            stVpssChnAttr.enDynamicRange = DYNAMIC_RANGE_SDR8;
            stVpssChnAttr.enPixelFormat = chn[i].outPixelFormat;
            stVpssChnAttr.stFrameRate.s32SrcFrameRate = chn[i].SrcFrameRate;
            stVpssChnAttr.stFrameRate.s32DstFrameRate = chn[i].DstFrameRate;
            stVpssChnAttr.u32Width = chn[i].outWidth;
            stVpssChnAttr.u32Height = chn[i].outHeight;
            stVpssChnAttr.enCompressMode = COMPRESS_MODE_NONE;
            stVpssChnAttr.bMirror = chn[i].bMirror;
            stVpssChnAttr.bFlip = chn[i].bFlip;
            stVpssChnAttr.u32Depth = 1;
            stVpssChnAttr.u32FrameBufCnt = 3;

            s32Ret = RK_MPI_VPSS_SetChnAttr(vpss->VpssGrpID, chn[i].VpssChnID, &stVpssChnAttr);
            if (s32Ret != RK_SUCCESS) {
                printf("ERROR: RK_MPI_VPSS_SetChnAttr fail! chn:%d ret=%d\n", chn[i].VpssChnID, s32Ret);
                break;
            }

            s32Ret = RK_MPI_VPSS_EnableChn(vpss->VpssGrpID, chn[i].VpssChnID);
            if (s32Ret != RK_SUCCESS) {
                printf("ERROR: RK_MPI_VPSS_EnableChn fail! chn:%d ret=%d\n", chn[i].VpssChnID, s32Ret);
                break;
            }
        }

        if (s32Ret != RK_SUCCESS) {
            return s32Ret;
        }

        s32Ret = RK_MPI_VPSS_StartGrp(vpss->VpssGrpID);
        if (s32Ret != RK_SUCCESS) {
            printf("ERROR: RK_MPI_VPSS_CreateGrp fail! chn:%d ret=%d\n", vpss->VpssGrpID, s32Ret);
            break;
        }

        if (vpss->bindSrcChn.enModId != RK_ID_BUTT) {
            MPP_CHN_S bindDestChn = {RK_ID_VPSS, 0, vpss->VpssGrpID};
            printf("====RK_MPI_SYS_Bind vpss group:%d ====\n", vpss->VpssGrpID);
            s32Ret = RK_MPI_SYS_Bind(&vpss->bindSrcChn, &bindDestChn);
            if (s32Ret) {
                printf("ERROR: RK_MPI_SYS_Bind fail ! ret=0x%X\n", s32Ret);
                break;
            }
        }

        s32Ret = RK_SUCCESS;
    } while(0);

    return s32Ret;
}

int rv1106_vpss_deinit(video_vpss_param_t *vpss)
{
    RK_S32 s32Ret = RK_FAILURE;
    int i = 0;

    if (vpss->enable == 0) {
        printf("error: vpss not enable, vpss group:%d\n", vpss->VpssGrpID);
        return s32Ret;
    }

    if (vpss->bindSrcChn.enModId != RK_ID_BUTT) {
        MPP_CHN_S bindDestChn = {RK_ID_VPSS, 0, vpss->VpssGrpID};
        printf("====RK_MPI_SYS_UnBind vpss group: %d ==== \n", vpss->VpssGrpID);
        s32Ret = RK_MPI_SYS_UnBind(&vpss->bindSrcChn, &bindDestChn);
        if (s32Ret != RK_SUCCESS) {
            printf("error: RK_MPI_SYS_UnBind fail! vpss group: %d s32Ret=0x%x\n", vpss->VpssGrpID, s32Ret);
            return s32Ret;
        }
        printf("OK\n");
    }

    printf("====RK_MPI_VPSS_StopGrp vpss group: %d ==== \n", vpss->VpssGrpID);
    s32Ret = RK_MPI_VPSS_StopGrp(vpss->VpssGrpID);
    if (s32Ret != RK_SUCCESS) {
        printf("error: RK_MPI_VPSS_StopGrp fail! vpss group: %d s32Ret=%d\n", vpss->VpssGrpID, s32Ret);
        return s32Ret;
    }
    printf("OK\n");

    // s32Ret = RK_MPI_VPSS_ResetGrp(vpss->VpssGrpID);
    // if (s32Ret != RK_SUCCESS) {
    //     RK_LOGE("RK_MPI_VPSS_ResetGrp failed with %#x!\n", s32Ret);
    //     return s32Ret;
    // }

    for (i = 0; i < sizeof(vpss->chn) / sizeof(video_vpss_chn_param_t); i++) {
        if (vpss->chn[i].enable == 0) {
            continue;
        }

        printf("====RK_MPI_VPSS_DisableChn vpss group: %d chn:%d ==== \n", vpss->VpssGrpID, vpss->chn[i].VpssChnID);
        s32Ret = RK_MPI_VPSS_DisableChn(vpss->VpssGrpID, vpss->chn[i].VpssChnID);
        if (s32Ret != RK_SUCCESS) {
            return s32Ret;
        }
        printf("OK\n");
    }

    printf("====RK_MPI_VPSS_DestroyGrp vpss group: %d ==== \n", vpss->VpssGrpID);
    s32Ret = RK_MPI_VPSS_DestroyGrp(vpss->VpssGrpID);
    if (s32Ret != RK_SUCCESS) {
        printf("error: RK_MPI_VPSS_DestroyGrp fail! vpss group: %d s32Ret=%d\n", vpss->VpssGrpID, s32Ret);
        return s32Ret;
    }
    printf("OK\n");

    return s32Ret;
}

int rv1106_vpss_GetStream(video_vpss_param_t *vpss, VPSS_CHN VpssChn, frameInfo_vi_t *Fvi_info)
{
    RK_S32 s32Ret = RK_FAILURE;
    VIDEO_FRAME_INFO_S stViFrame;
	PIC_BUF_ATTR_S stPicBufAttr;
	MB_PIC_CAL_S stMbPicCalResult;
    void *frame_data = NULL;

    if (vpss->enable == 0) {
        printf("error: vpss not enable, grp:%d\n", vpss->VpssGrpID);
        return s32Ret;
    }

    do {
        s32Ret = RK_MPI_VPSS_GetChnFrame(vpss->VpssGrpID, VpssChn, &stViFrame, 1000);
        if (s32Ret == RK_SUCCESS) {
            stPicBufAttr.u32Width = stViFrame.stVFrame.u32VirWidth;
            stPicBufAttr.u32Height = stViFrame.stVFrame.u32VirHeight;
            stPicBufAttr.enPixelFormat = stViFrame.stVFrame.enPixelFormat;
            stPicBufAttr.enCompMode = stViFrame.stVFrame.enCompressMode;
            s32Ret = RK_MPI_CAL_VGS_GetPicBufferSize(&stPicBufAttr, &stMbPicCalResult);
            if (s32Ret != RK_SUCCESS) {
                RK_LOGE("RK_MPI_CAL_VGS_GetPicBufferSize failed. err=0x%x", s32Ret);
                return s32Ret;
            }
            stViFrame.stVFrame.u64PrivateData = stMbPicCalResult.u32MBSize;

            RK_MPI_SYS_MmzFlushCache(stViFrame.stVFrame.pMbBlk, RK_TRUE);
            frame_data = RK_MPI_MB_Handle2VirAddr(stViFrame.stVFrame.pMbBlk);

            // printf("RK_MPI_VPSS_GetChnFrame chn:%d : pixelFormate: %d, w: %d, h: %d, seq:%d Size:%lld\n",
            //         VpssChn,
            //         stViFrame.stVFrame.enPixelFormat,
            //         stViFrame.stVFrame.u32Width,
            //         stViFrame.stVFrame.u32Height,
            //         stViFrame.stVFrame.u32TimeRef,
            //         stViFrame.stVFrame.u64PrivateData
            //         );

            Fvi_info->frame_size = stViFrame.stVFrame.u64PrivateData;
            Fvi_info->width = stViFrame.stVFrame.u32Width;
            Fvi_info->height = stViFrame.stVFrame.u32Height;
            Fvi_info->frame_seq = stViFrame.stVFrame.u32TimeRef;
            Fvi_info->PixelFormat = stViFrame.stVFrame.enPixelFormat;
            Fvi_info->timestamp = stViFrame.stVFrame.u64PTS;
            memmove(Fvi_info->frame_data, frame_data, Fvi_info->frame_size);

            s32Ret = RK_MPI_VPSS_ReleaseChnFrame(vpss->VpssGrpID, VpssChn, &stViFrame);
            if (s32Ret != RK_SUCCESS) {
                printf("error: RK_MPI_VPSS_ReleaseChnFrame fail grp:%d chn:%d ret:%X\n", vpss->VpssGrpID, VpssChn, s32Ret);
                break;
            }
        } else {
            printf("error: RK_MPI_VENC_GetStream timeout  grp:%d chn:%d ret:%X\n", vpss->VpssGrpID, VpssChn, s32Ret);
            break;
        }

        s32Ret = RK_SUCCESS;
    } while (0);

    return s32Ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
