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
#include "rv1106_venc.h"

int rv1106_venc_init(video_venc_param_t *venc)
{
    RK_S32 s32Ret = RK_FAILURE;
    VENC_RECV_PIC_PARAM_S stRecvParam;
    VENC_CHN_ATTR_S stAttr;

    if (venc->enable == 0) {
        printf("error: venc not enable, venv chn:%d\n", venc->vencChnId);
        return s32Ret;
    }

    memset(&stAttr, 0, sizeof(VENC_CHN_ATTR_S));

    do {
        stAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
        stAttr.stRcAttr.stH264Cbr.u32BitRate = 100 * 1024;
        stAttr.stRcAttr.stH264Cbr.u32Gop = 60;

        stAttr.stVencAttr.enType = venc->enType;
        stAttr.stVencAttr.enPixelFormat = venc->PixelFormat;
        stAttr.stVencAttr.u32Profile = H264E_PROFILE_HIGH;
        stAttr.stVencAttr.u32MaxPicWidth = venc->width;
        stAttr.stVencAttr.u32MaxPicHeight = venc->height;
        stAttr.stVencAttr.u32PicWidth = venc->width;
        stAttr.stVencAttr.u32PicHeight = venc->height;
        stAttr.stVencAttr.u32VirWidth = RK_ALIGN_2(venc->width);
        stAttr.stVencAttr.u32VirHeight = RK_ALIGN_2(venc->height);
        stAttr.stVencAttr.u32StreamBufCnt = 3;
        stAttr.stVencAttr.u32BufSize = venc->width * venc->height * 3 / 2;
        stAttr.stVencAttr.enMirror = MIRROR_NONE;

        s32Ret = RK_MPI_VENC_CreateChn(venc->vencChnId, &stAttr);
        if (s32Ret != RK_SUCCESS) {
            printf("error: RK_MPI_VENC_CreateChn failed, venv chn:%d, s32Ret: 0x%x\n", venc->vencChnId, s32Ret);
            break;
        }

        // stRecvParam.s32RecvPicNum = 100;		//recv 100 slice
        // RK_MPI_VENC_StartRecvFrame(chnId, &stRecvParam);

        memset(&stRecvParam, 0, sizeof(VENC_RECV_PIC_PARAM_S));
        stRecvParam.s32RecvPicNum = -1;
        s32Ret = RK_MPI_VENC_StartRecvFrame(venc->vencChnId, &stRecvParam);
        if (s32Ret != RK_SUCCESS) {
            printf("error: RK_MPI_VENC_StartRecvFrame failed, venv chn:%d, s32Ret: 0x%x\n", venc->vencChnId, s32Ret);
            break;
        }

        if (venc->bindSrcChn.enModId != RK_ID_BUTT) {
            MPP_CHN_S bindDestChn = {RK_ID_VENC, 0, venc->vencChnId};
            printf("====RK_MPI_SYS_Bind Venc chn:%d ====\n", venc->vencChnId);
            s32Ret = RK_MPI_SYS_Bind(&venc->bindSrcChn, &bindDestChn);
            if (s32Ret) {
                printf("ERROR: RK_MPI_SYS_Bind fail ! ret=0x%X\n", s32Ret);
                break;
            }
        }

        s32Ret = 0;
    } while(0);
    return s32Ret;
}

int rv1106_venc_deinit(video_venc_param_t *venc)
{
    RK_S32 s32Ret = RK_FAILURE;

    if (venc->enable == 0) {
        printf("error: venc not enable, venv chn:%d\n", venc->vencChnId);
        return s32Ret;
    }

    if (venc->bindSrcChn.enModId != RK_ID_BUTT) {
        MPP_CHN_S bindDestChn = {RK_ID_VENC, 0, venc->vencChnId};
        printf("====RK_MPI_SYS_UnBind venc chn: %d ====\n", venc->vencChnId);
        s32Ret = RK_MPI_SYS_UnBind(&venc->bindSrcChn, &bindDestChn);
        if (s32Ret != RK_SUCCESS) {
            printf("error: RK_MPI_SYS_UnBind fail! venc chn: %d s32Ret=%d\n", venc->vencChnId, s32Ret);
            return s32Ret;
        }
    }

    s32Ret = RK_MPI_VENC_StopRecvFrame(venc->vencChnId);
    if (s32Ret != RK_SUCCESS) {
        printf("error: RK_MPI_VENC_StopRecvFrame fail! venc chn: %d s32Ret=%d\n", venc->vencChnId, s32Ret);
        return s32Ret;
    }
    s32Ret = RK_MPI_VENC_DestroyChn(venc->vencChnId);
    if (s32Ret != RK_SUCCESS) {
        printf("error: RK_MPI_VENC_DestroyChn fail! venc chn: %d s32Ret=%d\n", venc->vencChnId, s32Ret);
        return s32Ret;
    }
    return s32Ret;
}

int rv1106_venc_GetStream(video_venc_param_t *venc, frameInfo_vi_t *Fvi_info)
{
    RK_S32 s32Ret = RK_FAILURE;
    void *frame_data = NULL;
    VENC_STREAM_S stFrame;

    if (venc->enable == 0) {
        printf("error: venc not enable, chn:%d\n", venc->vencChnId);
        return s32Ret;
    }

    stFrame.pstPack = malloc(sizeof(VENC_PACK_S));

    do {
        s32Ret = RK_MPI_VENC_GetStream(venc->vencChnId, &stFrame, 1000);
        if (s32Ret == RK_SUCCESS) {
            frame_data = RK_MPI_MB_Handle2VirAddr(stFrame.pstPack->pMbBlk);

            Fvi_info->frame_size = stFrame.pstPack->u32Len;
            Fvi_info->width = venc->width;
            Fvi_info->height = venc->height;
            Fvi_info->PixelFormat = venc->PixelFormat;
            Fvi_info->frame_seq = stFrame.u32Seq;
            Fvi_info->timestamp = stFrame.pstPack->u64PTS;
            memmove(Fvi_info->frame_data, frame_data, Fvi_info->frame_size);

            // printf("venc get:enc->seq:%d len:%d pts=%lld\n",
            //         stFrame.u32Seq, stFrame.pstPack->u32Len,
            //         stFrame.pstPack->u64PTS);

            s32Ret = RK_MPI_VENC_ReleaseStream(venc->vencChnId, &stFrame);
            if (s32Ret != RK_SUCCESS) {
                printf("error: RK_MPI_VENC_ReleaseStream fail chn:%d %x\n", venc->vencChnId, s32Ret);
                break;
            }
        } else {
            printf("error: RK_MPI_VENC_GetStream timeout chn:%d %x\n", venc->vencChnId, s32Ret);
            break;
        }
        s32Ret = RK_SUCCESS;
    } while (0);

    if (stFrame.pstPack) free(stFrame.pstPack);

    return s32Ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
