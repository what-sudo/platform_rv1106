#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */
#ifndef _VIDEO_H
#define _VIDEO_H

#include "rv1106_common.h"

typedef enum {
    GET_VENC_FRAME = 0,
    GET_SCREEN_FRAME = 1,
} get_frame_type_t;

int video_init(void);
int video_deinit(void);
int video_GetFrame(get_frame_type_t type, frameInfo_vi_t *Fvi_info);

#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
