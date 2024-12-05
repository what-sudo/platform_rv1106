#ifndef __INIT_PARAM_H__
#define __INIT_PARAM_H__

#define SENSOR_WIDTH     2304
#define SENSOR_HEIGHT    1296

void init_video_param_list(void);

rv1106_video_init_param_t *get_video_param_list(void);

video_isp_param_t *get_isp_param(void);
video_vi_dev_param_t *get_vi_dev_param(void);
video_vi_chn_param_t *get_vi_chn_param(void);
video_vpss_param_t *get_vpss_param(void);
video_venc_param_t *get_venc_param(void);
video_rgn_param_t *get_rgn_param(void);
video_iva_param_t *get_iva_param(void);

#endif
