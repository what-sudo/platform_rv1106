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

#ifdef RKAIQ
#include "rv1106_isp.h"
#endif

#define VIDEO_INIT_CHNNEL_MAX   2

typedef struct {
    int enable;
    VENC_CHN vencChnId;
    RK_CODEC_ID_E enType;
    int width;
    int height;
} video_enc_param_t;

typedef struct {
    int enable;
    VI_CHN viChnId;
    int width;
    int height;
    PIXEL_FORMAT_E PixelFormat;
    video_enc_param_t venc;
} video_vi_chn_param_t;

typedef struct {
    RK_S32 DevId;
    video_vi_chn_param_t vi_chn[VIDEO_INIT_CHNNEL_MAX];
    char *iq_file_dir;
} video_init_param_t;

video_init_param_t g_video_param_list_ctx = {
    .DevId = 0,
    .vi_chn = {
        {
            .enable = 0,
            .viChnId = 0,
            .width = 1920,
            .height = 1080,
            .PixelFormat = RK_FMT_YUV420SP,
            .venc = {
                .enable = 1,
                .enType = RK_VIDEO_ID_AVC,
                .vencChnId = 0,
                .width = 1920,
                .height = 1080
            }
        },
        {
            .enable = 1,
            .viChnId = 1,
            .width = 800,
            .height = 480,
            .PixelFormat = RK_FMT_XBGR8888,
            .venc = { .enable = 0}
        }
    },
    .iq_file_dir = "/etc/iqfiles"
};

static int vi_dev_init(VI_DEV ViDevId)
{
    printf("%s\n", __func__);
    int ret = 0;

    VI_DEV_ATTR_S stDevAttr;
    VI_DEV_BIND_PIPE_S stBindPipe;
    memset(&stDevAttr, 0, sizeof(stDevAttr));
    memset(&stBindPipe, 0, sizeof(stBindPipe));
    // 0. get dev config status
    ret = RK_MPI_VI_GetDevAttr(ViDevId, &stDevAttr);
    if (ret == RK_ERR_VI_NOT_CONFIG) {
        // 0-1.config dev
        ret = RK_MPI_VI_SetDevAttr(ViDevId, &stDevAttr);
        if (ret != RK_SUCCESS) {
            printf("RK_MPI_VI_SetDevAttr %x\n", ret);
            return -1;
        }
    } else {
        printf("RK_MPI_VI_SetDevAttr already\n");
    }
    // 1.get dev enable status
    ret = RK_MPI_VI_GetDevIsEnable(ViDevId);
    if (ret != RK_SUCCESS) {
        // 1-2.enable dev
        ret = RK_MPI_VI_EnableDev(ViDevId);
        if (ret != RK_SUCCESS) {
            printf("RK_MPI_VI_EnableDev %x\n", ret);
            return -1;
        }
        // 1-3.bind dev/pipe
        stBindPipe.u32Num = 1;
        stBindPipe.PipeId[0] = ViDevId;
        ret = RK_MPI_VI_SetDevBindPipe(ViDevId, &stBindPipe);
        if (ret != RK_SUCCESS) {
            printf("RK_MPI_VI_SetDevBindPipe %x\n", ret);
            return -1;
        }
    } else {
        printf("RK_MPI_VI_EnableDev already\n");
    }

    return 0;
}

static int vi_chn_init(VI_CHN viChnId, int width, int height, PIXEL_FORMAT_E PixelFormat) {
    int ret;
    int buf_cnt = 3;
    // VI init
    VI_CHN_ATTR_S vi_chn_attr;
    memset(&vi_chn_attr, 0, sizeof(vi_chn_attr));
    vi_chn_attr.stIspOpt.u32BufCount = buf_cnt;
    vi_chn_attr.stIspOpt.enMemoryType = VI_V4L2_MEMORY_TYPE_DMABUF; // VI_V4L2_MEMORY_TYPE_MMAP;
    // if (ctx->pDeviceName) {
    //     strcpy(vi_chn_attr.stIspOpt.aEntityName, ctx->pDeviceName);
    // }

    vi_chn_attr.stSize.u32Width = width;
    vi_chn_attr.stSize.u32Height = height;
    vi_chn_attr.stFrameRate.s32SrcFrameRate = -1;
    vi_chn_attr.stFrameRate.s32DstFrameRate = -1;
    vi_chn_attr.enPixelFormat = PixelFormat;
    vi_chn_attr.enCompressMode = COMPRESS_MODE_NONE; // COMPRESS_AFBC_16x16;
    vi_chn_attr.u32Depth = 1;
    ret = RK_MPI_VI_SetChnAttr(0, viChnId, &vi_chn_attr);
    ret |= RK_MPI_VI_EnableChn(0, viChnId);
    if (ret) {
        printf("ERROR: create VI error! ret=%d\n", ret);
        return ret;
    }

    return ret;
}

static int venc_init(VENC_CHN vencChnId, RK_CODEC_ID_E enType, PIXEL_FORMAT_E PixelFormat,
        int width, int height) {
    VENC_RECV_PIC_PARAM_S stRecvParam;
    VENC_CHN_ATTR_S stAttr;
    memset(&stAttr, 0, sizeof(VENC_CHN_ATTR_S));

    stAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
    stAttr.stRcAttr.stH264Cbr.u32BitRate = 100 * 1024;
    stAttr.stRcAttr.stH264Cbr.u32Gop = 60;

    stAttr.stVencAttr.enType = enType;
    stAttr.stVencAttr.enPixelFormat = PixelFormat;
    stAttr.stVencAttr.u32Profile = H264E_PROFILE_HIGH;
    stAttr.stVencAttr.u32PicWidth = width;
    stAttr.stVencAttr.u32PicHeight = height;
    stAttr.stVencAttr.u32VirWidth = width;
    stAttr.stVencAttr.u32VirHeight = height;
    stAttr.stVencAttr.u32StreamBufCnt = 2;
    stAttr.stVencAttr.u32BufSize = width * height * 3 / 2;
    stAttr.stVencAttr.enMirror = MIRROR_NONE;

    RK_MPI_VENC_CreateChn(vencChnId, &stAttr);

    // stRecvParam.s32RecvPicNum = 100;		//recv 100 slice
    // RK_MPI_VENC_StartRecvFrame(chnId, &stRecvParam);

    memset(&stRecvParam, 0, sizeof(VENC_RECV_PIC_PARAM_S));
    stRecvParam.s32RecvPicNum = -1;
    RK_MPI_VENC_StartRecvFrame(vencChnId, &stRecvParam);

    return 0;
}

int video_init(void)
{
    RK_S32 s32Ret = RK_FAILURE;
    int i;
    MPP_CHN_S stSrcChn, stDestChn;

    printf("#CameraIdx: %d\n", g_video_param_list_ctx.DevId);
    printf("#IQ Path: %s\n", g_video_param_list_ctx.iq_file_dir);

#ifdef RKAIQ
    RV1106_ISP_init(g_video_param_list_ctx.DevId, g_video_param_list_ctx.iq_file_dir, RK_AIQ_WORKING_MODE_NORMAL);
    RV1106_ISP_Run(g_video_param_list_ctx.DevId);
#endif

    do {
        if (RK_MPI_SYS_Init() != RK_SUCCESS) {
            break;
        }

        if (vi_dev_init(g_video_param_list_ctx.DevId)) {
            printf("error: vi_dev_init fail \n");
            break;
        }

        for (i = 0; i < VIDEO_INIT_CHNNEL_MAX; i++) {
            if (g_video_param_list_ctx.vi_chn[i].enable) {
                s32Ret = vi_chn_init(g_video_param_list_ctx.vi_chn[i].viChnId, g_video_param_list_ctx.vi_chn[i].width, g_video_param_list_ctx.vi_chn[i].height, g_video_param_list_ctx.vi_chn[i].PixelFormat);
                if (s32Ret != RK_SUCCESS) {
                    printf("error: vi_chn_init fail chn:%d \n", g_video_param_list_ctx.vi_chn[i].viChnId);
                    break;
                }

                if (g_video_param_list_ctx.vi_chn[i].venc.enable) {
                    s32Ret = venc_init(g_video_param_list_ctx.vi_chn[i].venc.vencChnId,
                        g_video_param_list_ctx.vi_chn[i].venc.enType,
                        g_video_param_list_ctx.vi_chn[i].PixelFormat,
                        g_video_param_list_ctx.vi_chn[i].venc.width,
                        g_video_param_list_ctx.vi_chn[i].venc.height);
                    if (s32Ret != RK_SUCCESS) {
                    printf("error: venc_init fail chn:%d \n", g_video_param_list_ctx.vi_chn[i].venc.vencChnId);
                    break;
                }
                }
            }
        }

        if (s32Ret) {
            break;
        }

        for (i = 0; i < VIDEO_INIT_CHNNEL_MAX; i++) {
            stSrcChn.s32DevId = g_video_param_list_ctx.DevId;
            stDestChn.s32DevId = g_video_param_list_ctx.DevId;
            if (g_video_param_list_ctx.vi_chn[i].enable
                && g_video_param_list_ctx.vi_chn[i].venc.enable) {
                // bind vi to venc
                stSrcChn.enModId = RK_ID_VI;
                stSrcChn.s32ChnId = g_video_param_list_ctx.vi_chn[i].viChnId;

                stDestChn.enModId = RK_ID_VENC;
                stDestChn.s32ChnId = g_video_param_list_ctx.vi_chn[i].venc.vencChnId;

                printf("====RK_MPI_SYS_Bind vi%d to venc%d====\n", stSrcChn.s32ChnId, stDestChn.s32ChnId);
                s32Ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
                if (s32Ret != RK_SUCCESS) {
                    printf("error: bind %d ch venc failed, s32Ret: 0x%x\n", stDestChn.s32ChnId, s32Ret);
                    break;
                }
            }
        }

        if (s32Ret) {
            break;
        }

        printf("%s initial finish\n", __func__);

        s32Ret = 0;
    } while (0);

    if (s32Ret) {
        printf("error: s32Ret:0x%x\n", s32Ret);
    }

    return s32Ret;
}

int video_deinit(void)
{
    int i;
    RK_S32 s32Ret = RK_FAILURE;
    MPP_CHN_S stSrcChn, stDestChn;

    for (i = 0; i < VIDEO_INIT_CHNNEL_MAX; i++) {
        stSrcChn.s32DevId = g_video_param_list_ctx.DevId;
        stDestChn.s32DevId = g_video_param_list_ctx.DevId;
        if (g_video_param_list_ctx.vi_chn[i].enable
            && g_video_param_list_ctx.vi_chn[i].venc.enable) {
            stSrcChn.enModId = RK_ID_VI;
            stSrcChn.s32ChnId = g_video_param_list_ctx.vi_chn[i].viChnId;

            stDestChn.enModId = RK_ID_VENC;
            stDestChn.s32ChnId = g_video_param_list_ctx.vi_chn[i].venc.vencChnId;

            printf("====RK_MPI_SYS_UnBind vi%d to venc%d====\n", stSrcChn.s32ChnId, stDestChn.s32ChnId);
            s32Ret = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
            if (s32Ret != RK_SUCCESS) {
                printf("error: RK_MPI_SYS_UnBind %d ch venc failed\n", stDestChn.s32ChnId);
                break;;
            }
            s32Ret = RK_MPI_VENC_StopRecvFrame(g_video_param_list_ctx.vi_chn[i].venc.vencChnId);
            if (s32Ret != RK_SUCCESS) {
                return s32Ret;
            }
            s32Ret = RK_MPI_VENC_DestroyChn(g_video_param_list_ctx.vi_chn[i].venc.vencChnId);
            if (s32Ret != RK_SUCCESS) {
                RK_LOGE("RK_MPI_VDEC_DestroyChn fail %x", s32Ret);
            }
        }
    }

    for (i = 0; i < VIDEO_INIT_CHNNEL_MAX; i++) {
        if (g_video_param_list_ctx.vi_chn[i].enable) {
            s32Ret = RK_MPI_VI_DisableChn(0, g_video_param_list_ctx.vi_chn[i].viChnId);
            RK_LOGE("RK_MPI_VI_DisableChn %d %s\n", g_video_param_list_ctx.vi_chn[i].viChnId, s32Ret ? "Fail":"OK");
        }
    }

    s32Ret = RK_MPI_VI_DisableDev(0);
    RK_LOGE("RK_MPI_VI_DisableDev %s", s32Ret ? "Fail":"OK");

    RK_MPI_SYS_Exit();

#ifdef RKAIQ
        RV1106_ISP_Stop(g_video_param_list_ctx.DevId);
#endif
    return 0;
}

int video_GetFrame(get_frame_type_t type, frameInfo_vi_t *Fvi_info)
{
    RK_S32 s32Ret = RK_FAILURE;
    RK_S32 waitTime = 1000;
    void *frame_data = NULL;

    static RK_U64 last_frame_PTS = 0;

    if (Fvi_info == NULL) {
        fprintf(stderr, "parame error\n");
        return s32Ret;
    }

    if (type == GET_VENC_FRAME) {
        VENC_STREAM_S stFrame;

        if (g_video_param_list_ctx.vi_chn[type].enable == 0 || g_video_param_list_ctx.vi_chn[type].venc.enable == 0) {
            printf("error: GET_VENC_FRAME chnnel not enable\n");
            return -1;
        }

        stFrame.pstPack = malloc(sizeof(VENC_PACK_S));
        s32Ret = RK_MPI_VENC_GetStream(g_video_param_list_ctx.vi_chn[type].venc.vencChnId, &stFrame, waitTime);
        if (s32Ret == RK_SUCCESS) {
            frame_data = RK_MPI_MB_Handle2VirAddr(stFrame.pstPack->pMbBlk);

            Fvi_info->frame_size = stFrame.pstPack->u32Len;
            Fvi_info->width = g_video_param_list_ctx.vi_chn[type].venc.width;
            Fvi_info->height = g_video_param_list_ctx.vi_chn[type].venc.height;
            Fvi_info->PixelFormat = g_video_param_list_ctx.vi_chn[type].PixelFormat;
            memmove(Fvi_info->frame_data, frame_data, Fvi_info->frame_size);

            // printf("venc get:enc->seq:%d len:%d pts=%lld\n",
            //         stFrame.u32Seq, stFrame.pstPack->u32Len,
            //         stFrame.pstPack->u64PTS);

            s32Ret = RK_MPI_VENC_ReleaseStream(g_video_param_list_ctx.vi_chn[type].venc.vencChnId, &stFrame);
            if (s32Ret != RK_SUCCESS) {
                printf("error: RK_MPI_VENC_ReleaseStream fail %x\n", s32Ret);
            }
        } else {
            printf("error: RK_MPI_VI_GetChnFrame fail %x\n", s32Ret);
        }
        free(stFrame.pstPack);
    } else if (type == GET_SCREEN_FRAME) {
        VIDEO_FRAME_INFO_S stViFrame;

        if (g_video_param_list_ctx.vi_chn[type].enable == 0) {
            printf("error: GET_SCREEN_FRAME chnnel not enable\n");
            return -1;
        }

        s32Ret = RK_MPI_VI_GetChnFrame(0, g_video_param_list_ctx.vi_chn[type].viChnId, &stViFrame, waitTime);
        if (s32Ret == RK_SUCCESS) {
            frame_data = RK_MPI_MB_Handle2VirAddr(stViFrame.stVFrame.pMbBlk);

            Fvi_info->frame_size = stViFrame.stVFrame.u64PrivateData;
            Fvi_info->width = stViFrame.stVFrame.u32Width;
            Fvi_info->height = stViFrame.stVFrame.u32Height;
            Fvi_info->PixelFormat = stViFrame.stVFrame.enPixelFormat;
            memmove(Fvi_info->frame_data, frame_data, Fvi_info->frame_size);

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

            s32Ret = RK_MPI_VI_ReleaseChnFrame(0, g_video_param_list_ctx.vi_chn[type].viChnId, &stViFrame);
            if (s32Ret != RK_SUCCESS) {
                RK_LOGE("RK_MPI_VI_ReleaseChnFrame fail %x", s32Ret);
            }
        } else {
            RK_LOGE("RK_MPI_VI_GetChnFrame timeout %x", s32Ret);
        }
    }

    return s32Ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
