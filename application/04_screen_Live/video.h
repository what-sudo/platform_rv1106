#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */
#ifndef _VIDEO_H
#define _VIDEO_H

#include "rk_debug.h"
#include "rk_defines.h"
#include "rk_mpi_adec.h"
#include "rk_mpi_aenc.h"
#include "rk_mpi_ai.h"
#include "rk_mpi_ao.h"
#include "rk_mpi_avs.h"
#include "rk_mpi_cal.h"
#include "rk_mpi_ivs.h"
#include "rk_mpi_mb.h"
#include "rk_mpi_rgn.h"
#include "rk_mpi_sys.h"
#include "rk_mpi_tde.h"
#include "rk_mpi_vdec.h"
#include "rk_mpi_venc.h"
#include "rk_mpi_vi.h"
#include "rk_mpi_vo.h"
#include "rk_mpi_vpss.h"
#ifdef ROCKIVA
#include "rockiva/rockiva_ba_api.h"
#include "rockiva/rockiva_common.h"
#include "rockiva/rockiva_det_api.h"
#include "rockiva/rockiva_face_api.h"
#include "rockiva/rockiva_image.h"
#endif

#define RKAIQ

#ifdef RKAIQ
#include "rk_aiq_comm.h"
#endif

typedef enum {
    GET_VENC_FRAME = 0,
    GET_SCREEN_FRAME = 1,
} get_frame_type_t;

typedef struct {
    int width;
    int height;
    uint64_t frame_size;
    PIXEL_FORMAT_E PixelFormat;
    uint8_t *frame_data;
} frameInfo_vi_t;

int video_init(void);
int video_deinit(void);
int video_GetFrame(get_frame_type_t type, frameInfo_vi_t *Fvi_info);

#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
