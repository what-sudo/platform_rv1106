#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#ifndef _RV1106_ISP_H_
#define _RV1106_ISP_H_

typedef struct {
    int enable;
    VI_DEV ViDevId;
    char *iq_file_dir;
    rk_aiq_working_mode_t hdr_mode;
} video_isp_param_t;


RK_S32 RV1106_ISP_init(video_isp_param_t *isp_ctx);
RK_S32 RV1106_ISP_deinit(video_isp_param_t *isp_ctx);

#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
