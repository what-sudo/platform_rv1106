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
#include "rv1106_vi.h"

int rv1106_vi_dev_init(video_vi_dev_param_t *vi_dev)
{
    printf("%s\n", __func__);
    int ret = 0;

    VI_DEV_ATTR_S stDevAttr;
    memset(&stDevAttr, 0, sizeof(stDevAttr));
    // 0. get dev config status
    ret = RK_MPI_VI_GetDevAttr(vi_dev->ViDevId, &stDevAttr);
    if (ret == RK_ERR_VI_NOT_CONFIG) {
        // 0-1.config dev
        ret = RK_MPI_VI_SetDevAttr(vi_dev->ViDevId, &stDevAttr);
        if (ret != RK_SUCCESS) {
            printf("RK_MPI_VI_SetDevAttr %x\n", ret);
            return -1;
        }
    } else {
        printf("RK_MPI_VI_SetDevAttr already\n");
    }
    // 1.get dev enable status
    ret = RK_MPI_VI_GetDevIsEnable(vi_dev->ViDevId);
    if (ret != RK_SUCCESS) {
        // 1-2.enable dev
        ret = RK_MPI_VI_EnableDev(vi_dev->ViDevId);
        if (ret != RK_SUCCESS) {
            printf("RK_MPI_VI_EnableDev %x\n", ret);
            return -1;
        }
        ret = RK_MPI_VI_SetDevBindPipe(vi_dev->ViDevId, &vi_dev->BindPipe);
        if (ret != RK_SUCCESS) {
            printf("RK_MPI_VI_SetDevBindPipe %x\n", ret);
            return -1;
        }
    } else {
        printf("RK_MPI_VI_EnableDev already\n");
    }

    return 0;
}

int rv1106_vi_chn_init(video_vi_chn_param_t *vi_chn)
{
    int ret = -1;
    // VI init
    VI_CHN_ATTR_S vi_chn_attr;

    if (vi_chn->enable == 0) {
        printf("error: vi_chn not enable, vi_chn:%d\n", vi_chn->viChnId);
        return ret;
    }

    memset(&vi_chn_attr, 0, sizeof(vi_chn_attr));
    vi_chn_attr.stIspOpt.u32BufCount = 3;
    vi_chn_attr.stIspOpt.enMemoryType = VI_V4L2_MEMORY_TYPE_DMABUF; // VI_V4L2_MEMORY_TYPE_MMAP;
    vi_chn_attr.stIspOpt.stMaxSize.u32Width = vi_chn->width;
    vi_chn_attr.stIspOpt.stMaxSize.u32Height = vi_chn->height;

    vi_chn_attr.stSize.u32Width = vi_chn->width;
    vi_chn_attr.stSize.u32Height = vi_chn->height;
    vi_chn_attr.stFrameRate.s32SrcFrameRate = vi_chn->SrcFrameRate;
    vi_chn_attr.stFrameRate.s32DstFrameRate = vi_chn->DstFrameRate;
    vi_chn_attr.enPixelFormat = vi_chn->PixelFormat;
    vi_chn_attr.enCompressMode = COMPRESS_MODE_NONE; // COMPRESS_AFBC_16x16;
    vi_chn_attr.u32Depth = 1;
    ret = RK_MPI_VI_SetChnAttr(vi_chn->ViPipe, vi_chn->viChnId, &vi_chn_attr);
    ret |= RK_MPI_VI_EnableChn(vi_chn->ViPipe, vi_chn->viChnId);
    if (ret) {
        printf("ERROR: create VI error! ret=%d\n", ret);
        return ret;
    }

    return ret;
}

int rv1106_vi_chn_deinit(video_vi_chn_param_t *vi_chn)
{
    RK_S32 s32Ret = RK_FAILURE;

    if (vi_chn->enable == 0) {
        printf("error: vi_chn not enable, vi_chn chn:%d\n", vi_chn->viChnId);
        return s32Ret;
    }

    s32Ret = RK_MPI_VI_DisableChn(vi_chn->ViPipe, vi_chn->viChnId);
    if (s32Ret != RK_SUCCESS) {
        printf("error: RK_MPI_VI_DisableChn fail! vi chn: %d s32Ret=%d\n", vi_chn->viChnId, s32Ret);
        return s32Ret;
    }
    printf("RK_MPI_VI_DisableChn %d %s\n", vi_chn->viChnId, s32Ret ? "Fail":"OK");

    return s32Ret;
}

int rv1106_vi_dev_deinit(video_vi_dev_param_t *vi_dev)
{
    RK_S32 s32Ret = RK_FAILURE;
    s32Ret = RK_MPI_VI_DisableDev(vi_dev->ViDevId);
    if (s32Ret != RK_SUCCESS) {
        printf("error: RK_MPI_VI_DisableDev fail! vi dev: %d s32Ret=%d\n", vi_dev->ViDevId, s32Ret);
        return s32Ret;
    }
    printf("RK_MPI_VI_DisableDev %d %s\n", vi_dev->ViDevId, s32Ret ? "Fail":"OK");

    return s32Ret;
}

int rv1106_vichn_GetStream(video_vi_chn_param_t *vi_chn, frameInfo_vi_t *Fvi_info)
{
    RK_S32 s32Ret = RK_FAILURE;
    VIDEO_FRAME_INFO_S stViFrame;

    void *frame_data = NULL;

    if (vi_chn->enable == 0) {
        printf("error: vi_chn not enable, chn:%d\n", vi_chn->viChnId);
        return s32Ret;
    }

    do {
        s32Ret = RK_MPI_VI_GetChnFrame(vi_chn->ViPipe, vi_chn->viChnId, &stViFrame, 1000);
        if (s32Ret == RK_SUCCESS) {
            frame_data = RK_MPI_MB_Handle2VirAddr(stViFrame.stVFrame.pMbBlk);

            Fvi_info->frame_size = stViFrame.stVFrame.u64PrivateData;
            Fvi_info->width = stViFrame.stVFrame.u32Width;
            Fvi_info->height = stViFrame.stVFrame.u32Height;
            Fvi_info->PixelFormat = stViFrame.stVFrame.enPixelFormat;
            Fvi_info->frame_seq = stViFrame.stVFrame.u32TimeRef;
            Fvi_info->timestamp = stViFrame.stVFrame.u64PTS;
            memmove(Fvi_info->frame_data, frame_data, Fvi_info->frame_size);

            // static RK_U64 last_frame_PTS = 0;
            // printf(
            //     "RK_MPI_VI_GetChnFrame ok:seq:%d pts:%lld ms len=%llu fps:%.1f",
            //     stViFrame.stVFrame.u32TimeRef,
            //     stViFrame.stVFrame.u64PTS / 1000, stViFrame.stVFrame.u64PrivateData,
            //    1000.0 / ((stViFrame.stVFrame.u64PTS - last_frame_PTS) / 1000));
            // last_frame_PTS = stViFrame.stVFrame.u64PTS;

            // VI_CHN_STATUS_S stChnStatus;
            // s32Ret = RK_MPI_VI_QueryChnStatus(0, ctx->ChnId, &stChnStatus);
            // printf("ChnStatus ret %x, w:%d,h:%d,enable:%d,"
            //         "current frame id:%d,input lost:%d,output lost:%d,"
            //         "framerate:%d",
            //         s32Ret, stChnStatus.stSize.u32Width, stChnStatus.stSize.u32Height,
            //         stChnStatus.bEnable, stChnStatus.u32CurFrameID,
            //         stChnStatus.u32InputLostFrame, stChnStatus.u32OutputLostFrame,
            //         stChnStatus.u32FrameRate);


            s32Ret = RK_MPI_VI_ReleaseChnFrame(vi_chn->ViPipe, vi_chn->viChnId, &stViFrame);
            if (s32Ret != RK_SUCCESS) {
                printf("error: RK_MPI_VI_ReleaseChnFrame fail chn:%d 0x%X\n", vi_chn->viChnId, s32Ret);
                break;
            }

        } else {
            printf("error: RK_MPI_VI_GetChnFrame timeout chn:%d 0x%X\n", vi_chn->viChnId, s32Ret);
            break;
        }

        s32Ret = RK_SUCCESS;
    } while (0);

    return s32Ret;
}

int rv1106_vichn_GetStream_fd(video_vi_chn_param_t *vi_chn, frameInfo_vi_t *Fvi_info)
{
    RK_S32 s32Ret = RK_FAILURE;
    int fd;

    if (vi_chn->enable == 0) {
        printf("error: vi_chn not enable, chn:%d\n", vi_chn->viChnId);
        return s32Ret;
    }

    do {
        s32Ret = RK_MPI_VI_GetChnFrame(vi_chn->ViPipe, vi_chn->viChnId, &Fvi_info->rkViFrame, 1000);
        if (s32Ret == RK_SUCCESS) {
            //  void *frame_data = RK_MPI_MB_Handle2VirAddr(Fvi_info->rkViFrame.stVFrame.pMbBlk);
            fd = RK_MPI_MB_Handle2Fd(Fvi_info->rkViFrame.stVFrame.pMbBlk);

            // VI_CHN_STATUS_S stChnStatus;
            // s32Ret = RK_MPI_VI_QueryChnStatus(vi_chn->ViPipe, vi_chn->viChnId, &stChnStatus);
            // printf("RK_MPI_VI_QueryChnStatus ret %x, w:%d,h:%d,enable:%d,"
            //         "current frame id:%d,input lost:%d,output lost:%d,"
            //         "framerate:%d,vbfail:%d \t",
            //         s32Ret, stChnStatus.stSize.u32Width, stChnStatus.stSize.u32Height,
            //         stChnStatus.bEnable, stChnStatus.u32CurFrameID,
            //         stChnStatus.u32InputLostFrame, stChnStatus.u32OutputLostFrame,
            //         stChnStatus.u32FrameRate, stChnStatus.u32VbFail
            //         );

            Fvi_info->frame_size = Fvi_info->rkViFrame.stVFrame.u64PrivateData;
            Fvi_info->width = Fvi_info->rkViFrame.stVFrame.u32Width;
            Fvi_info->height = Fvi_info->rkViFrame.stVFrame.u32Height;

            Fvi_info->PixelFormat = Fvi_info->rkViFrame.stVFrame.enPixelFormat;
            Fvi_info->frame_seq = Fvi_info->rkViFrame.stVFrame.u32TimeRef;
            Fvi_info->timestamp = Fvi_info->rkViFrame.stVFrame.u64PTS;
            Fvi_info->dataFd = fd;
        } else {
            printf("error: RK_MPI_VI_GetChnFrame timeout chn:%d %x\n", vi_chn->viChnId, s32Ret);
            break;
        }

        s32Ret = RK_SUCCESS;
    } while (0);

    return s32Ret;
}


int rv1106_vichn_ReleaseStream_fd(video_vi_chn_param_t *vi_chn, frameInfo_vi_t *Fvi_info)
{
    RK_S32 s32Ret = RK_FAILURE;

    if (vi_chn->enable == 0) {
        printf("error: vi_chn not enable, chn:%d\n", vi_chn->viChnId);
        return s32Ret;
    }

    do {
        s32Ret = RK_MPI_VI_ReleaseChnFrame(vi_chn->ViPipe, vi_chn->viChnId, &Fvi_info->rkViFrame);
        if (s32Ret != RK_SUCCESS) {
            printf("error: RK_MPI_VI_ReleaseChnFrame fail chn:%d %x\n", vi_chn->viChnId, s32Ret);
            break;
        }

    } while (0);

    return s32Ret;
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
