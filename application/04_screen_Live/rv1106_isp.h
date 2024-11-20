#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#ifndef _RV1106_ISP_H_
#define _RV1106_ISP_H_

RK_S32 RV1106_ISP_init(VI_DEV ViDevId, const char *iq_file_dir, rk_aiq_working_mode_t hdr_mode);
RK_S32 RV1106_ISP_Run(VI_DEV ViDevId);
RK_S32 RV1106_ISP_Stop(VI_DEV ViDevId);

#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
