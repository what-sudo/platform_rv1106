#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#ifndef _RV1106_VPSS_H_
#define _RV1106_VPSS_H_

typedef struct {
    int enable;

    VPSS_CHN VpssChnID;
    int outWidth;
    int outHeight;

    int SrcFrameRate;
    int DstFrameRate;

    PIXEL_FORMAT_E outPixelFormat;
    RK_BOOL bMirror;
    RK_BOOL bFlip;

} video_vpss_chn_param_t;

typedef struct {
    int enable;
    VPSS_GRP VpssGrpID;
    int inWidth;
    int inHeight;

    int SrcFrameRate;
    int DstFrameRate;

    PIXEL_FORMAT_E inPixelFormat;

    video_vpss_chn_param_t chn[4];

    MPP_CHN_S bindSrcChn;
} video_vpss_param_t;

int rv1106_vpss_init(video_vpss_param_t *vpss);
int rv1106_vpss_deinit(video_vpss_param_t *vpss);

int rv1106_vpss_GetStream(video_vpss_param_t *vpss, int chn_index, frameInfo_vi_t *Fvi_info);

#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
