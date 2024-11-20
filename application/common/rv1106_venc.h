#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#ifndef _RV1106_VENC_H_
#define _RV1106_VENC_H_

typedef struct {
    int enable;
    VENC_CHN vencChnId;
    RK_CODEC_ID_E enType;
    int width;
    int height;
    PIXEL_FORMAT_E PixelFormat;
    MPP_CHN_S bindSrcChn;
} video_venc_param_t;

int rv1106_venc_init(video_venc_param_t *venc);
int rv1106_venc_deinit(video_venc_param_t *venc);
int rv1106_venc_GetStream(video_venc_param_t *venc, frameInfo_vi_t *Fvi_info);

#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
