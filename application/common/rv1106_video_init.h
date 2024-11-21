#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */
#ifndef _RV1106_VIDEO_INIT_H_
#define _RV1106_VIDEO_INIT_H_

#include "rv1106_common.h"

#include "rv1106_isp.h"
#include "rv1106_vi.h"
#include "rv1106_vpss.h"
#include "rv1106_venc.h"
#include "rv1106_iva.h"
#include "rv1106_rgn.h"

#include "graphics_Draw.h"

typedef struct {
    video_isp_param_t isp[1];
    video_vi_dev_param_t vi_dev[1];
    video_vi_chn_param_t vi_chn[2];
    video_vpss_param_t vpss[1];
    video_venc_param_t venc[2];
    video_rgn_param_t rgn[8];
    video_iva_param_t iva[1];
} rv1106_video_init_param_t;

int rv1106_video_init(rv1106_video_init_param_t *video_param);
int rv1106_video_deinit(rv1106_video_init_param_t *video_param);

#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
