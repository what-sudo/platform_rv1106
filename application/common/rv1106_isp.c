#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <time.h>
#include <assert.h>

#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/prctl.h>

#include <errno.h>
#include <pthread.h>
#include <sys/poll.h>
#include <stdatomic.h>

#include <rk_aiq_user_api2_camgroup.h>
#include <rk_aiq_user_api2_imgproc.h>
#include <rk_aiq_user_api2_sysctl.h>

#include "rv1106_common.h"
#include "rv1106_isp.h"

#define MAX_AIQ_CTX 8

static rk_aiq_sys_ctx_t *g_aiq_ctx[MAX_AIQ_CTX];
rk_aiq_working_mode_t g_WDRMode[MAX_AIQ_CTX];

static atomic_int g_sof_cnt = 0;
static atomic_bool g_should_quit = false;

static XCamReturn SAMPLE_COMM_ISP_SofCb(rk_aiq_metas_t *meta) {
	g_sof_cnt++;
	if (g_sof_cnt <= 2)
		printf("=== %u ===\n", meta->frame_id);
	return XCAM_RETURN_NO_ERROR;
}

RK_S32 SAMPLE_COMM_ISP_GetSofCnt(void) { return g_sof_cnt; }

static XCamReturn SAMPLE_COMM_ISP_ErrCb(rk_aiq_err_msg_t *msg) {
	if (msg->err_code == XCAM_RETURN_BYPASS)
		g_should_quit = true;
	return XCAM_RETURN_NO_ERROR;
}

RK_BOOL SAMPLE_COMM_ISP_ShouldQuit() { return g_should_quit; }

RK_S32 RV1106_ISP_init(video_isp_param_t *isp_ctx)
{
    RK_S32 ret = -1;
    rk_aiq_sys_ctx_t *aiq_ctx;
    rk_aiq_static_info_t aiq_static_info = { 0 };

    if (isp_ctx->enable == 0) {
        printf("error: param not enable\n");
        return ret;
    }

    if (isp_ctx->ViDevId >= MAX_AIQ_CTX) {
        printf("%s : ViDevId is over %d or not init\n", __FUNCTION__, MAX_AIQ_CTX);
        return ret;
    }

    if (isp_ctx->iq_file_dir == NULL) {
        printf("error: iq_file_dir is null\n");
        return ret;
    }

    if (access(isp_ctx->iq_file_dir, F_OK)) {
        printf("error: not found %s\n", isp_ctx->iq_file_dir);
        return ret;
    }

    g_WDRMode[isp_ctx->ViDevId] = isp_ctx->hdr_mode;

    do {
        rk_aiq_uapi2_sysctl_enumStaticMetasByPhyId(isp_ctx->ViDevId, &aiq_static_info);

        printf("ID: %d, sensor_name is %s, iqfiles is %s\n", isp_ctx->ViDevId,
            aiq_static_info.sensor_info.sensor_name, isp_ctx->iq_file_dir);
        rk_aiq_uapi2_sysctl_preInit_devBufCnt(aiq_static_info.sensor_info.sensor_name,
                                            "rkraw_rx", 2);

        aiq_ctx = rk_aiq_uapi2_sysctl_init(aiq_static_info.sensor_info.sensor_name, isp_ctx->iq_file_dir,
                                SAMPLE_COMM_ISP_ErrCb, SAMPLE_COMM_ISP_SofCb);

        // if (ctx->bMultictx)
        //     rk_aiq_uapi2_sysctl_setMulCamConc(aiq_ctx, true);

        g_aiq_ctx[isp_ctx->ViDevId] = aiq_ctx;

        if (rk_aiq_uapi2_sysctl_prepare(g_aiq_ctx[isp_ctx->ViDevId], 0, 0, g_WDRMode[isp_ctx->ViDevId])) {
            printf("rkaiq engine prepare failed !\n");
            g_aiq_ctx[isp_ctx->ViDevId] = NULL;
            break;
        }

        printf("rk_aiq_uapi2_sysctl_init/prepare succeed\n");
        if (rk_aiq_uapi2_sysctl_start(g_aiq_ctx[isp_ctx->ViDevId])) {
            printf("rk_aiq_uapi2_sysctl_start  failed\n");
            break;
        }
        printf("rk_aiq_uapi2_sysctl_start succeed\n");

        ret = 0;
    } while (0);

    return ret;
}

RK_S32 RV1106_ISP_deinit(video_isp_param_t *isp_ctx)
{
    if (isp_ctx->ViDevId >= MAX_AIQ_CTX || !g_aiq_ctx[isp_ctx->ViDevId]) {
        printf("%s : CamId is over %d or not init g_aiq_ctx[%d] = %p\n", __FUNCTION__, MAX_AIQ_CTX,
            isp_ctx->ViDevId, g_aiq_ctx[isp_ctx->ViDevId]);
        return -1;
    }
    printf("rk_aiq_uapi2_sysctl_stop enter\n");
    rk_aiq_uapi2_sysctl_stop(g_aiq_ctx[isp_ctx->ViDevId], false);
    printf("rk_aiq_uapi2_sysctl_deinit enter\n");
    rk_aiq_uapi2_sysctl_deinit(g_aiq_ctx[isp_ctx->ViDevId]);
    printf("rk_aiq_uapi2_sysctl_deinit exit\n");
    g_aiq_ctx[isp_ctx->ViDevId] = NULL;
    return 0;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
