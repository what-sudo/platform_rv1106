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
#include <pthread.h>
#include <semaphore.h>

#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/prctl.h>

#include <errno.h>
#include <pthread.h>
#include <sys/poll.h>
#include <stdatomic.h>

#include "rv1106_common.h"
#include "rv1106_rgn.h"

int rv1106_rgn_init(video_rgn_param_t *ctx)
{
    RK_S32 s32Ret = RK_FAILURE;
    RGN_ATTR_S stRgnAttr = {0};
    RGN_CHN_ATTR_S stRgnChnAttr = {0};

    if (ctx->enable == 0) {
        printf("[%s %d] error: not enable\n", __func__, __LINE__);
        return s32Ret;
    }

    if (ctx->width % 8 || ctx->height % 8
        || ctx->X % 2 || ctx->Y % 2)
    {
        printf("[%s %d] error: must be aligned\n", __func__, __LINE__);
        return s32Ret;
    }

    switch (ctx->type) {
        case OVERLAY_EX_RGN: {
            stRgnAttr.enType = OVERLAY_EX_RGN;
            stRgnAttr.unAttr.stOverlay.enPixelFmt = ctx->overlay.format;
            stRgnAttr.unAttr.stOverlay.stSize.u32Width = ctx->width;
            stRgnAttr.unAttr.stOverlay.stSize.u32Height = ctx->height;

            stRgnChnAttr.bShow = ctx->show;
            stRgnChnAttr.enType = OVERLAY_EX_RGN;
            stRgnChnAttr.unChnAttr.stOverlayChn.u32Layer = ctx->layer;
            stRgnChnAttr.unChnAttr.stOverlayChn.stPoint.s32X = ctx->X;
            stRgnChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y = ctx->Y;
            stRgnChnAttr.unChnAttr.stOverlayChn.u32BgAlpha = ctx->overlay.u32BgAlpha;
            stRgnChnAttr.unChnAttr.stOverlayChn.u32FgAlpha = ctx->overlay.u32FgAlpha;
        } break;
        case OVERLAY_RGN: {
            stRgnAttr.enType = OVERLAY_RGN;
            stRgnAttr.unAttr.stOverlay.enPixelFmt = ctx->overlay.format;
            stRgnAttr.unAttr.stOverlay.stSize.u32Width = ctx->width;
            stRgnAttr.unAttr.stOverlay.stSize.u32Height = ctx->height;

            stRgnChnAttr.bShow = ctx->show;
            stRgnChnAttr.enType = OVERLAY_RGN;
            stRgnChnAttr.unChnAttr.stOverlayChn.u32Layer = ctx->layer;
            stRgnChnAttr.unChnAttr.stOverlayChn.stPoint.s32X = ctx->X;
            stRgnChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y = ctx->Y;
            stRgnChnAttr.unChnAttr.stOverlayChn.u32BgAlpha = ctx->overlay.u32BgAlpha;
            stRgnChnAttr.unChnAttr.stOverlayChn.u32FgAlpha = ctx->overlay.u32FgAlpha;
        } break;
        case COVER_RGN: {
            stRgnAttr.enType = COVER_RGN;

            stRgnChnAttr.bShow = ctx->show;
            stRgnChnAttr.enType = COVER_RGN;
            stRgnChnAttr.unChnAttr.stCoverChn.u32Layer = ctx->layer;
            stRgnChnAttr.unChnAttr.stCoverChn.stRect.s32X = ctx->X;
            stRgnChnAttr.unChnAttr.stCoverChn.stRect.s32Y = ctx->Y;
            stRgnChnAttr.unChnAttr.stCoverChn.stRect.u32Width = ctx->width;
            stRgnChnAttr.unChnAttr.stCoverChn.stRect.u32Height = ctx->height;
            stRgnChnAttr.unChnAttr.stCoverChn.u32Color = ctx->cover.u32Color;

        } break;
        case MOSAIC_RGN: {
            stRgnAttr.enType = MOSAIC_RGN;

            stRgnChnAttr.bShow = ctx->show;
            stRgnChnAttr.enType = MOSAIC_RGN;
            stRgnChnAttr.unChnAttr.stMosaicChn.u32Layer = ctx->layer;
            stRgnChnAttr.unChnAttr.stMosaicChn.stRect.s32X = ctx->X;
            stRgnChnAttr.unChnAttr.stMosaicChn.stRect.s32Y = ctx->Y;
            stRgnChnAttr.unChnAttr.stMosaicChn.stRect.u32Width = ctx->width;
            stRgnChnAttr.unChnAttr.stMosaicChn.stRect.u32Height = ctx->height;
            stRgnChnAttr.unChnAttr.stMosaicChn.enBlkSize = ctx->mosaic.blkSize;

        } break;
        default:
            printf("[%s %d] error: unsupport type:%d\n", __func__, __LINE__, ctx->type);
            return RK_FAILURE;
    }

    do {
        s32Ret = RK_MPI_RGN_Create(ctx->rgnHandle, &stRgnAttr);
        if (RK_SUCCESS != s32Ret) {
            printf("[%s %d] error: RK_MPI_RGN_Create (%d) failed with %#x!\n", __func__, __LINE__, ctx->rgnHandle, s32Ret);
            break;
        }

        s32Ret = RK_MPI_RGN_AttachToChn(ctx->rgnHandle, &ctx->mppChn, &stRgnChnAttr);
        if (RK_SUCCESS != s32Ret) {
            printf("[%s %d] error: RK_MPI_RGN_AttachToChn (%d) failed with %#x!\n", __func__, __LINE__, ctx->rgnHandle, s32Ret);
            break;
        }

        s32Ret = rv1106_rgn_update_chnAttr(ctx);
        if (RK_SUCCESS != s32Ret) {
            printf("[%s %d] error: rv1106_rgn_update_chnAttr (%d) failed with %#x!\n", __func__, __LINE__, ctx->rgnHandle, s32Ret);
            break;
        }

        s32Ret = sem_init(&ctx->sem1, 0, 1);
        if (s32Ret != 0) {
            printf("[%s %d] error: sem_init failure\n", __func__, __LINE__);
            break;
        }

        s32Ret = sem_init(&ctx->sem2, 0, 0);
        if (s32Ret != 0) {
            printf("[%s %d] error: sem_init failure\n", __func__, __LINE__);
            break;
        }

        // if (ctx->type == OVERLAY_RGN || ctx->type == OVERLAY_EX_RGN) {
        //     s32Ret = rv1106_rgn_overlay_test(ctx);
        //     if (RK_SUCCESS != s32Ret) {
        //         printf("[%s %d] error: rv1106_rgn_overlay_test (%d) failed with %#x!\n", __func__, __LINE__, ctx->rgnHandle, s32Ret);
        //         break;
        //     }
        // }

        s32Ret = 0;
    } while(0);
    return s32Ret;
}

RK_U64 TEST_COMM_GetNowUs() {
    struct timespec time = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &time);
    return (RK_U64)time.tv_sec * 1000000 + (RK_U64)time.tv_nsec / 1000; /* microseconds */
}

int rv1106_rgn_overlay_test(video_rgn_param_t *ctx)
{
    RK_S32 s32Ret = RK_FAILURE;
    if (ctx->enable == 0) {
        printf("[%s %d] error: not enable\n", __func__, __LINE__);
        return s32Ret;
    }

    if (ctx->type != OVERLAY_RGN && ctx->type != OVERLAY_EX_RGN) {
        printf("[%s %d] error: unsupport this function\n", __func__, __LINE__);
        return s32Ret;
    }

    RK_S64 s64ShowBmpStart = TEST_COMM_GetNowUs();
    RGN_CANVAS_INFO_S stCanvasInfo;
    memset(&stCanvasInfo, 0, sizeof(RGN_CANVAS_INFO_S));

    sem_wait(&ctx->sem1);
    do {
        s32Ret = RK_MPI_RGN_GetCanvasInfo(ctx->rgnHandle, &stCanvasInfo);
        if (s32Ret != RK_SUCCESS) {
            RK_LOGE("RK_MPI_RGN_GetCanvasInfo failed with %#x!", s32Ret);
            break;
        }
        sem_post(&ctx->sem2);

        memset((uint8_t*)(uint32_t)(stCanvasInfo.u64VirAddr), 0xaa, stCanvasInfo.u32VirWidth * stCanvasInfo.u32VirHeight);

        sem_wait(&ctx->sem2);
        s32Ret = RK_MPI_RGN_UpdateCanvas(ctx->rgnHandle);
        if (s32Ret != RK_SUCCESS) {
            RK_LOGE("RK_MPI_RGN_UpdateCanvas failed with %#x!", s32Ret);
            break;
        }
    } while (0);

    sem_post(&ctx->sem1);
    RK_S64 s64ShowBmpEnd = TEST_COMM_GetNowUs();
    printf("Handle:%d, space time %lld us, update canvas success!\n", ctx->rgnHandle, s64ShowBmpEnd - s64ShowBmpStart);

    return s32Ret;
}

int rv1106_rgn_overlay_get_canvas(video_rgn_param_t *ctx, RGN_CANVAS_INFO_S *CanvasInfo)
{
    RK_S32 s32Ret = RK_FAILURE;
    if (ctx->enable == 0) {
        printf("[%s %d] error: not enable\n", __func__, __LINE__);
        return s32Ret;
    }

    if (CanvasInfo == NULL) {
        printf("[%s %d] error: CanvasInfo is null\n", __func__, __LINE__);
        return s32Ret;
    }

    if (ctx->type != OVERLAY_RGN && ctx->type != OVERLAY_EX_RGN) {
        printf("[%s %d] error: unsupport this function\n", __func__, __LINE__);
        return s32Ret;
    }

    sem_wait(&ctx->sem1);
    do {
        s32Ret = RK_MPI_RGN_GetCanvasInfo(ctx->rgnHandle, CanvasInfo);
        if (s32Ret != RK_SUCCESS) {
            RK_LOGE("RK_MPI_RGN_GetCanvasInfo failed with %#x!", s32Ret);
            break;;
        }
        sem_post(&ctx->sem2);
    } while (0);

    if (s32Ret) {
        sem_post(&ctx->sem1);
    }

    return s32Ret;
}

int rv1106_rgn_overlay_set_canvas(video_rgn_param_t *ctx, RGN_CANVAS_INFO_S *CanvasInfo)
{
    RK_S32 s32Ret = RK_FAILURE;
    if (ctx->enable == 0) {
        printf("[%s %d] error: not enable\n", __func__, __LINE__);
        return s32Ret;
    }

    if (CanvasInfo == NULL) {
        printf("[%s %d] error: CanvasInfo is null\n", __func__, __LINE__);
        return s32Ret;
    }

    if (ctx->type != OVERLAY_RGN && ctx->type != OVERLAY_EX_RGN) {
        printf("[%s %d] error: unsupport this function\n", __func__, __LINE__);
        return s32Ret;
    }

    sem_wait(&ctx->sem2);
    do {
        s32Ret = RK_MPI_RGN_UpdateCanvas(ctx->rgnHandle);
        if (s32Ret != RK_SUCCESS) {
            RK_LOGE("RK_MPI_RGN_UpdateCanvas failed with %#x!", s32Ret);
            break;;
        }
        sem_post(&ctx->sem1);
    } while (0);

    if (s32Ret) {
        sem_post(&ctx->sem2);
    }

    return s32Ret;
}

int rv1106_rgn_overlay_setBitmap(video_rgn_param_t *ctx, BITMAP_S *bitmap)
{
    RK_S32 s32Ret = RK_FAILURE;

    if (ctx->enable == 0) {
        printf("[%s %d] error: not enable\n", __func__, __LINE__);
        return s32Ret;
    }

    if (ctx->type != OVERLAY_RGN && ctx->type != OVERLAY_EX_RGN) {
        printf("[%s %d] error: unsupport this function\n", __func__, __LINE__);
        return s32Ret;
    }

    if (bitmap->pData == NULL) {
        printf("[%s %d] error: data is null\n", __func__, __LINE__);
        return s32Ret;
    }

    if (bitmap->enPixelFormat != ctx->overlay.format) {
        printf("[%s %d] error: format err\n", __func__, __LINE__);
        return s32Ret;
    }

    s32Ret = RK_MPI_RGN_SetBitMap(ctx->rgnHandle, bitmap);
    if (s32Ret != RK_SUCCESS) {
        printf("[%s %d] error: RK_MPI_RGN_SetBitMap failed with %#x!\n", __func__, __LINE__, s32Ret);
        return s32Ret;
    }

    return s32Ret;
}

int rv1106_rgn_update_rgnAttr(video_rgn_param_t *ctx)
{
    RK_S32 s32Ret = RK_FAILURE;
    return s32Ret;
}

int rv1106_rgn_update_chnAttr(video_rgn_param_t *ctx)
{
    RK_S32 s32Ret = RK_FAILURE;
    RGN_CHN_ATTR_S stRgnChnAttr = {0};

    if (ctx->enable == 0) {
        printf("[%s %d] error: not enable\n", __func__, __LINE__);
        return s32Ret;
    }

    if (ctx->width % 8 || ctx->height % 8
        || ctx->X % 2 || ctx->Y % 2)
    {
        printf("[%s %d] error: must be 16 aligned\n", __func__, __LINE__);
        // printf("%d %d %d %d / %d %d %d %d", ctx->width, ctx->height, ctx->X, ctx->Y,
        //     ctx->width % 16, ctx->height % 16, ctx->X % 16, ctx->Y % 16);
        return s32Ret;
    }

    s32Ret = RK_MPI_RGN_GetDisplayAttr(ctx->rgnHandle, &ctx->mppChn, &stRgnChnAttr);
    if (RK_SUCCESS != s32Ret) {
        printf("[%s %d] error: RK_MPI_RGN_GetDisplayAttr (%d)) failed with %#x!\n", __func__, __LINE__, ctx->rgnHandle, s32Ret);
        return s32Ret;
    }

    switch (ctx->type) {
        case OVERLAY_EX_RGN: {
            stRgnChnAttr.bShow = ctx->show;
            stRgnChnAttr.enType = OVERLAY_EX_RGN;
            stRgnChnAttr.unChnAttr.stOverlayChn.u32Layer = ctx->layer;
            stRgnChnAttr.unChnAttr.stOverlayChn.stPoint.s32X = ctx->X;
            stRgnChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y = ctx->Y;
            stRgnChnAttr.unChnAttr.stOverlayChn.u32BgAlpha = ctx->overlay.u32BgAlpha;
            stRgnChnAttr.unChnAttr.stOverlayChn.u32FgAlpha = ctx->overlay.u32FgAlpha;
        } break;
        case OVERLAY_RGN: {
            stRgnChnAttr.bShow = ctx->show;
            stRgnChnAttr.enType = OVERLAY_RGN;
            stRgnChnAttr.unChnAttr.stOverlayChn.u32Layer = ctx->layer;
            stRgnChnAttr.unChnAttr.stOverlayChn.stPoint.s32X = ctx->X;
            stRgnChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y = ctx->Y;
            stRgnChnAttr.unChnAttr.stOverlayChn.u32BgAlpha = ctx->overlay.u32BgAlpha;
            stRgnChnAttr.unChnAttr.stOverlayChn.u32FgAlpha = ctx->overlay.u32FgAlpha;

        } break;
        case COVER_RGN: {
            stRgnChnAttr.bShow = ctx->show;
            stRgnChnAttr.enType = COVER_RGN;
            stRgnChnAttr.unChnAttr.stCoverChn.u32Layer = ctx->layer;
            stRgnChnAttr.unChnAttr.stCoverChn.stRect.s32X = ctx->X;
            stRgnChnAttr.unChnAttr.stCoverChn.stRect.s32Y = ctx->Y;
            stRgnChnAttr.unChnAttr.stCoverChn.stRect.u32Width = ctx->width;
            stRgnChnAttr.unChnAttr.stCoverChn.stRect.u32Height = ctx->height;

            stRgnChnAttr.unChnAttr.stCoverChn.u32Color = ctx->cover.u32Color;

        } break;
        case MOSAIC_RGN: {
            stRgnChnAttr.bShow = ctx->show;
            stRgnChnAttr.enType = MOSAIC_RGN;
            stRgnChnAttr.unChnAttr.stMosaicChn.u32Layer = ctx->layer;
            stRgnChnAttr.unChnAttr.stMosaicChn.stRect.s32X = ctx->X;
            stRgnChnAttr.unChnAttr.stMosaicChn.stRect.s32Y = ctx->Y;
            stRgnChnAttr.unChnAttr.stMosaicChn.stRect.u32Width = ctx->width;
            stRgnChnAttr.unChnAttr.stMosaicChn.stRect.u32Height = ctx->height;
            stRgnChnAttr.unChnAttr.stMosaicChn.enBlkSize = ctx->mosaic.blkSize;

        } break;
        default:
            printf("[%s %d] error: unsupport type:%d\n", __func__, __LINE__, ctx->type);
            return RK_FAILURE;
    }

    s32Ret = RK_MPI_RGN_SetDisplayAttr(ctx->rgnHandle, &ctx->mppChn, &stRgnChnAttr);
    if (RK_SUCCESS != s32Ret) {
        printf("[%s %d] error: RK_MPI_RGN_SetDisplayAttr (%d)) failed with %#x!\n", __func__, __LINE__, ctx->rgnHandle, s32Ret);
        return s32Ret;
    }

    return s32Ret;
}

int rv1106_rgn_deinit(video_rgn_param_t *ctx)
{
    RK_S32 s32Ret = RK_FAILURE;

    if (ctx->enable == 0) {
        printf("[%s %d] error: not enable\n", __func__, __LINE__);
        return s32Ret;
    }

    s32Ret = sem_destroy(&ctx->sem1);
    if (s32Ret != 0) {
        printf("[%s %d] error: sem_destroy failure\n", __func__, __LINE__);
        return RK_FAILURE;
    }

    s32Ret = sem_destroy(&ctx->sem2);
    if (s32Ret != 0) {
        printf("[%s %d] error: sem_destroy failure\n", __func__, __LINE__);
        return RK_FAILURE;
    }

    s32Ret = RK_MPI_RGN_DetachFromChn(ctx->rgnHandle, &ctx->mppChn);
    if (RK_SUCCESS != s32Ret) {
        printf("[%s %d] error: RK_MPI_RGN_DetachFromChn  (%d) failed with %#x!\n", __func__, __LINE__, ctx->rgnHandle, s32Ret);
        return RK_FAILURE;
    }

    s32Ret = RK_MPI_RGN_Destroy(ctx->rgnHandle);
    if (RK_SUCCESS != s32Ret) {
        printf("[%s %d] error: RK_MPI_RGN_Destroy  (%d) failed with %#x!\n", __func__, __LINE__, ctx->rgnHandle, s32Ret);
        return RK_FAILURE;
    }

    return s32Ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
