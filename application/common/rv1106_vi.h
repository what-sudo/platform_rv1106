#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#ifndef _RV1106_VI_H_
#define _RV1106_VI_H_

typedef struct {
    int enable;
    VI_PIPE ViPipe;
    VI_CHN viChnId;
    int width;
    int height;
    int SrcFrameRate;
    int DstFrameRate;
    PIXEL_FORMAT_E PixelFormat;
} video_vi_chn_param_t;

typedef struct {
    int enable;
    VI_DEV ViDevId;
    VI_DEV_BIND_PIPE_S BindPipe;
} video_vi_dev_param_t;

int rv1106_vi_dev_init(video_vi_dev_param_t *vi_dev);
int rv1106_vi_chn_init(video_vi_chn_param_t *vi_chn);
int rv1106_vi_chn_deinit(video_vi_chn_param_t *vi_chn);
int rv1106_vi_dev_deinit(video_vi_dev_param_t *vi_dev);

int rv1106_vichn_GetStream(video_vi_chn_param_t *vi_chn, frameInfo_vi_t *Fvi_info);
int rv1106_vichn_GetStream_fd(video_vi_chn_param_t *vi_chn, frameInfo_vi_t *Fvi_info);
int rv1106_vichn_ReleaseStream_fd(video_vi_chn_param_t *vi_chn, frameInfo_vi_t *Fvi_info);

#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
