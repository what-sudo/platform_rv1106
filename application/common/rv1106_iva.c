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
#include "rv1106_iva.h"

void FrameReleaseCallback(const RockIvaReleaseFrames* releaseFrames, void* userdata)
{
    // video_iva_param_t* iva_ctx = (video_iva_param_t*)userdata;
    // printf("FrameReleaseCallback count=%d\n", releaseFrames->count);
}

void DetResultCallback(const RockIvaDetectResult* result, const RockIvaExecuteStatus status, void* userdata)
{
    video_iva_param_t* iva_ctx = (video_iva_param_t*)userdata;
    int x1, y1, x2, y2;

    if (iva_ctx->result_cb) {
        video_iva_callback_param_t ctx = {.objNum = result->objNum, .objInfo = result->objInfo};
        iva_ctx->result_cb(&ctx);
    } else {
        int i;
        printf("\nDetResultCallback frameId %d objNum:%d\n", result->frameId, result->objNum);
        if (result->objNum) {
            for (i = 0; i < result->objNum; i++) {
                x1 = result->objInfo[i].rect.topLeft.x;
                y1 = result->objInfo[i].rect.topLeft.y;
                x2 = result->objInfo[i].rect.bottomRight.x;
                y2 = result->objInfo[i].rect.bottomRight.y;

                char *objname = "NONE";
                switch (result->objInfo[i].type)
                {
                    case ROCKIVA_OBJECT_TYPE_PET: {
                    } break;
                    case ROCKIVA_OBJECT_TYPE_FACE: {
                        objname = "FACE";
                    } break;
                    case ROCKIVA_OBJECT_TYPE_PERSON: {
                        objname = "PERSON";
                    } break;
                    case ROCKIVA_OBJECT_TYPE_HEAD: {
                        objname = "HEAD";
                    } break;
                    case ROCKIVA_OBJECT_TYPE_PLATE: {
                        objname = "PLATE";
                    } break;
                    case ROCKIVA_OBJECT_TYPE_VEHICLE: {
                        objname = "VEHICLE";
                    } break;
                    case ROCKIVA_OBJECT_TYPE_NON_VEHICLE: {
                        objname = "NON_VEHICLE";
                    } break;
                    case ROCKIVA_OBJECT_TYPE_BICYCLE: {
                        objname = "BICYCLE";
                    } break;
                    case ROCKIVA_OBJECT_TYPE_MOTORCYCLE: {
                        objname = "MOTORCYCLE";
                    } break;
                    default: {
                        printf("Warning: obj type NONE %d", result->objInfo[i].type);
                        objname = "NONE";
                    } break;
                }

                if (x1 >= 10000 || y1 >= 10000 || x2 >= 10000 || y2 >= 10000) {
                    printf("[%s %d] error: --- ", __func__, __LINE__);
                }
                printf("%s x1:%d y1:%d x2:%d y2:%d\n", objname, x1, y1, x2, y2);
            }
        }
    }
}

int rv1106_iva_init(video_iva_param_t *iva)
{
    RockIvaRetCode s32Ret = RK_FAILURE;
    RockIvaDetTaskParams detParams;
    RockIvaInitParam ivaParams;

    char version[32] = {0};

    if (iva->enable == 0) {
        printf("[%s %d] error: iva not enable\n", __func__, __LINE__);
        return s32Ret;
    }

    s32Ret = ROCKIVA_GetVersion(sizeof(version), version);
    if (s32Ret != ROCKIVA_RET_SUCCESS) {
        printf("[%s %d] ROCKIVA_SetFrameReleaseCallback error: ret:%d\n", __func__, __LINE__, s32Ret);
        return s32Ret;
    }

    printf("ROCKIVA Version:%*.s\n", sizeof(version), version);

    memset(&ivaParams, 0, sizeof(RockIvaInitParam));
    memset(&detParams, 0, sizeof(RockIvaDetTaskParams));

    // 配置模型路径
    snprintf(ivaParams.modelPath, ROCKIVA_PATH_LENGTH, iva->models_path);
    ivaParams.coreMask = 0x04;
    ivaParams.logLevel = ROCKIVA_LOG_ERROR;

    ivaParams.detModel = ROCKIVA_DET_MODEL_PFP;

    ivaParams.imageInfo.width = iva->width;
    ivaParams.imageInfo.height = iva->height;
    ivaParams.imageInfo.format = iva->IvaPixelFormat;
    ivaParams.imageInfo.transformMode = ROCKIVA_IMAGE_TRANSFORM_NONE;

    do {
        s32Ret = ROCKIVA_Init(&iva->handle, ROCKIVA_MODE_VIDEO, &ivaParams, iva);
        if (s32Ret != ROCKIVA_RET_SUCCESS) {
            printf("[%s %d] ROCKIVA_Init error: ret:%d\n", __func__, __LINE__, s32Ret);
            break;
        }

        s32Ret = ROCKIVA_SetFrameReleaseCallback(iva->handle, FrameReleaseCallback);
        if (s32Ret != ROCKIVA_RET_SUCCESS) {
            printf("[%s %d] ROCKIVA_SetFrameReleaseCallback error: ret:%d\n", __func__, __LINE__, s32Ret);
            break;
        }

        // // 设置上报目标类型
        // detParams.detObjectType |= ROCKIVA_OBJECT_TYPE_BITMASK(ROCKIVA_OBJECT_TYPE_PERSON);
        // detParams.detObjectType |= ROCKIVA_OBJECT_TYPE_BITMASK(ROCKIVA_OBJECT_TYPE_FACE);
        // detParams.detObjectType |= ROCKIVA_OBJECT_TYPE_BITMASK(ROCKIVA_OBJECT_TYPE_PET);

        // // 设置检测分数阈值，只设置第 0 个可以对所有类别生效
        // detParams.scores[0] = 30;
        // detParams.min_det_count = 2;

        s32Ret = ROCKIVA_DETECT_Init(iva->handle, &detParams, DetResultCallback);
        if (s32Ret != ROCKIVA_RET_SUCCESS) {
            printf("[%s %d] ROCKIVA_DETECT_Init error: ret:%d\n", __func__, __LINE__, s32Ret);
            break;
        }

        pthread_mutex_init(&(iva->mutex), NULL);

        s32Ret = 0;
    } while(0);
    return s32Ret;
}

int rv1106_iva_deinit(video_iva_param_t *iva)
{
    RockIvaRetCode s32Ret = RK_FAILURE;

    if (iva->enable == 0) {
        printf("[%s %d] error: iva not enable\n", __func__, __LINE__);
        return s32Ret;
    }
    s32Ret = ROCKIVA_WaitFinish(iva->handle, -1, 3000);
    if (s32Ret != ROCKIVA_RET_SUCCESS) {
        printf("[%s %d] ROCKIVA_WaitFinish error: ret:%d\n", __func__, __LINE__, s32Ret);
        return s32Ret;
    }

    s32Ret = ROCKIVA_DETECT_Release(iva->handle);
    if (s32Ret != ROCKIVA_RET_SUCCESS) {
        printf("[%s %d] ROCKIVA_DETECT_Release error: ret:%d\n", __func__, __LINE__, s32Ret);
        return s32Ret;
    }

    s32Ret = ROCKIVA_Release(iva->handle);
    if (s32Ret != ROCKIVA_RET_SUCCESS) {
        printf("[%s %d] ROCKIVA_Release error: ret:%d\n", __func__, __LINE__, s32Ret);
        return s32Ret;
    }

    return s32Ret;
}

int rv1106_iva_push_frame(video_iva_param_t *iva, frameInfo_vi_t *Fvi_info)
{
    uint64_t frame_size = 0;
    RockIvaRetCode s32Ret = RK_FAILURE;
    RockIvaImage image = {0};

    do {

        image.channelId = 0;
        image.frameId = Fvi_info->frame_seq;
        image.info.width = Fvi_info->width;
        image.info.height = Fvi_info->height;
        image.info.transformMode = ROCKIVA_IMAGE_TRANSFORM_NONE;

        switch (Fvi_info->PixelFormat) {
            case RK_FMT_RGB888: image.info.format = ROCKIVA_IMAGE_FORMAT_RGB888; frame_size = Fvi_info->width * Fvi_info->height * 3; break;
            case RK_FMT_BGR888: image.info.format = ROCKIVA_IMAGE_FORMAT_BGR888; frame_size = Fvi_info->width * Fvi_info->height * 3; break;
            case RK_FMT_RGBA8888: image.info.format = ROCKIVA_IMAGE_FORMAT_RGBA8888; frame_size = Fvi_info->width * Fvi_info->height * 4; break;
            case RK_FMT_BGRA8888: image.info.format = ROCKIVA_IMAGE_FORMAT_BGRA8888; frame_size = Fvi_info->width * Fvi_info->height * 4;  break;
            case RK_FMT_YUV420SP: image.info.format = ROCKIVA_IMAGE_FORMAT_YUV420SP_NV12; frame_size = Fvi_info->width * Fvi_info->height * 1.5;  break;
            default: {
                printf("[%s %d] unsupport PixelFormat:%d\n", __func__, __LINE__, Fvi_info->PixelFormat);
                s32Ret = ROCKIVA_RET_UNSUPPORTED;
                break;
            }
        }
        if (s32Ret == ROCKIVA_RET_UNSUPPORTED) {
            break;
        }

        if (image.dataAddr == NULL) {
            image.dataAddr = (unsigned char *)malloc(frame_size);
            if (image.dataAddr == NULL) {
                printf("[%s %d] malloc error\n", __func__, __LINE__);
                break;
            }
        }

        image.size = frame_size;
        memmove(image.dataAddr, Fvi_info->frame_data, frame_size);

        s32Ret = ROCKIVA_PushFrame(iva->handle, &image, NULL);
        if (s32Ret != ROCKIVA_RET_SUCCESS) {
            printf("[%s %d] ROCKIVA_PushFrame error: ret:%d\n", __func__, __LINE__, s32Ret);
            break;
        }
    } while (0);

    if (s32Ret != ROCKIVA_RET_SUCCESS) {
        if (image.dataAddr != NULL) {
            free(image.dataAddr);
            image.dataAddr = NULL;
        }
    }

    return s32Ret;
}

int rv1106_iva_push_frame_fd(video_iva_param_t *iva, frameInfo_vi_t *Fvi_info)
{
    RockIvaRetCode s32Ret = RK_FAILURE;
    RockIvaImage image = {0};
    // static uint64_t count = 0;
    do {

        image.channelId = 0;
        image.dataFd = Fvi_info->dataFd;
        image.frameId = Fvi_info->frame_seq;
        // image.frameId = count++;
        image.info.width = Fvi_info->width;
        image.info.height = Fvi_info->height;
        image.info.transformMode = ROCKIVA_IMAGE_TRANSFORM_NONE;

        switch (Fvi_info->PixelFormat) {
            case RK_FMT_RGB888: image.info.format = ROCKIVA_IMAGE_FORMAT_RGB888; break;
            case RK_FMT_BGR888: image.info.format = ROCKIVA_IMAGE_FORMAT_BGR888; break;
            case RK_FMT_RGBA8888: image.info.format = ROCKIVA_IMAGE_FORMAT_RGBA8888; break;
            case RK_FMT_BGRA8888: image.info.format = ROCKIVA_IMAGE_FORMAT_BGRA8888; break;
            case RK_FMT_YUV420SP: image.info.format = ROCKIVA_IMAGE_FORMAT_YUV420SP_NV12; break;
            default: {
                printf("[%s %d] unsupport PixelFormat:%d\n", __func__, __LINE__, Fvi_info->PixelFormat);
                s32Ret = ROCKIVA_RET_UNSUPPORTED;
                break;
            }
        }
        if (s32Ret == ROCKIVA_RET_UNSUPPORTED) {
            break;
        }

        s32Ret = ROCKIVA_PushFrame(iva->handle, &image, NULL);
        if (s32Ret != ROCKIVA_RET_SUCCESS) {
            printf("[%s %d] ROCKIVA_PushFrame error: ret:%d\n", __func__, __LINE__, s32Ret);
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
