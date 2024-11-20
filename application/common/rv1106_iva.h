#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#ifndef _RV1106_IVA_H_
#define _RV1106_IVA_H_

typedef struct {
  uint32_t objNum;                                /* 目标个数 */
  const RockIvaObjectInfo *objInfo; /* 各目标检测信息 */
} video_iva_callback_param_t;

typedef int(*rv1106_iva_result_callbake)(video_iva_callback_param_t *ctx);

typedef struct {
    int enable;
    char *models_path;
    int width;
    int height;
    RockIvaImageFormat IvaPixelFormat;
    rv1106_iva_result_callbake result_cb;

    /************************/
    RockIvaHandle handle;
    RockIvaInitParam Params;
    pthread_mutex_t mutex;
} video_iva_param_t;

int rv1106_iva_init(video_iva_param_t *iva);
int rv1106_iva_deinit(video_iva_param_t *iva);
int rv1106_iva_push_frame(video_iva_param_t *iva, frameInfo_vi_t *Fvi_info);
int rv1106_iva_push_frame_fd(video_iva_param_t *iva, frameInfo_vi_t *Fvi_info);

#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
