#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#ifndef _RV1106_RGN_H_
#define _RV1106_RGN_H_

typedef struct {
    PIXEL_FORMAT_E format;
    RK_U32 u32BgAlpha;
    RK_U32 u32FgAlpha;
} rv1106_rgn_overlay_t;

typedef struct {
    RK_U32 u32Color;    /* RGB888 format */
} rv1106_rgn_cover_t;

typedef struct {
    MOSAIC_BLK_SIZE_E blkSize; /*block size of MOSAIC*/
} rv1106_rgn_mosaic_t;

typedef struct {
    int enable;
    RGN_HANDLE rgnHandle;
    RGN_TYPE_E type;
    uint32_t X;
    uint32_t Y;
    uint32_t width;
    uint32_t height;
    uint32_t layer;
    RK_BOOL show;
    MPP_CHN_S mppChn;
    sem_t sem1;
    sem_t sem2;

    union {
        rv1106_rgn_overlay_t overlay;
        rv1106_rgn_cover_t cover;
        rv1106_rgn_mosaic_t mosaic;
    };

} video_rgn_param_t;

int rv1106_rgn_init(video_rgn_param_t *ctx);
int rv1106_rgn_deinit(video_rgn_param_t *ctx);
int rv1106_rgn_update_chnAttr(video_rgn_param_t *ctx);

/* 必须搭配使用 */
int rv1106_rgn_overlay_get_canvas(video_rgn_param_t *ctx, RGN_CANVAS_INFO_S *CanvasInfo);
int rv1106_rgn_overlay_set_canvas(video_rgn_param_t *ctx, RGN_CANVAS_INFO_S *CanvasInfo);

/* 推荐使用 get/set canvas接口*/
int rv1106_rgn_overlay_setBitmap(video_rgn_param_t *ctx, BITMAP_S *bitmap);


int rv1106_rgn_overlay_test(video_rgn_param_t *ctx);
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
