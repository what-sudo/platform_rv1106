#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#ifndef _RV1106_IVA_H_
#define _RV1106_IVA_H_

typedef struct {
  uint32_t frameId;                               /* 帧ID */
  uint32_t objNum;                                /* 目标个数 */
  const RockIvaObjectInfo *objInfo; /* 各目标检测信息 */
} video_iva_callback_param_t;

typedef int(*rv1106_iva_result_callbake)(video_iva_callback_param_t *ctx);

typedef struct {
    int enable;
    const char *models_path;
    int width;
    int height;
    RockIvaImageFormat IvaPixelFormat;
    rv1106_iva_result_callbake result_cb;

    /************************/
    RockIvaHandle handle;
} video_iva_param_t;

#define iva_red_color        0xFFFF0000    // 红色
#define iva_green_color      0xFF00FF00    // 绿色
#define iva_blue_color       0xFF0000FF    // 蓝色
#define iva_yellow_color     0xFFFFFF00    // 黄色
#define iva_cyan_color       0xFF00FFFF    // 青色
#define iva_magenta_color    0xFFFF00FF    // 品红
#define iva_orange_color     0xFFFFA500    // 橙色
#define iva_purple_color     0xFF800080    // 紫色
#define iva_white_color      0xFFFFFFFF    // 白色
#define iva_black_color      0xFF000000    // 黑色

extern const char *iva_object_name[ROCKIVA_OBJECT_TYPE_MAX];
extern const uint32_t iva_object_color[ROCKIVA_OBJECT_TYPE_MAX];

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
