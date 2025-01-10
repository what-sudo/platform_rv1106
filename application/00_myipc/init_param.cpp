#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>

#include "video.h"
#include "rv1106_video_init.h"

#include "init_param.h"

static rv1106_video_init_param_t g_video_param_list_ctx = { 0 };

void init_video_param_list(void)
{
    memset(&g_video_param_list_ctx, 0, sizeof(rv1106_video_init_param_t));

    g_video_param_list_ctx.isp[0].enable = 1;
    g_video_param_list_ctx.isp[0].ViDevId = 0;
    g_video_param_list_ctx.isp[0].iq_file_dir = "/etc/iqfiles";
    g_video_param_list_ctx.isp[0].hdr_mode = RK_AIQ_WORKING_MODE_NORMAL;

    g_video_param_list_ctx.vi_dev[0].enable = 1;
    g_video_param_list_ctx.vi_dev[0].ViDevId = 0;
    g_video_param_list_ctx.vi_dev[0].BindPipe.u32Num = 1;
    g_video_param_list_ctx.vi_dev[0].BindPipe.PipeId[0] = 0;

#if VI_CHN0_ENABLE
    g_video_param_list_ctx.vi_chn[0].enable = VI_CHN0_ENABLE;
    g_video_param_list_ctx.vi_chn[0].ViPipe = 0;
    g_video_param_list_ctx.vi_chn[0].viChnId = 0;
    g_video_param_list_ctx.vi_chn[0].width = SENSOR_WIDTH;
    g_video_param_list_ctx.vi_chn[0].height = SENSOR_HEIGHT;
    g_video_param_list_ctx.vi_chn[0].SrcFrameRate = -1;
    g_video_param_list_ctx.vi_chn[0].DstFrameRate = -1;
    g_video_param_list_ctx.vi_chn[0].PixelFormat = RK_FMT_YUV420SP;
    g_video_param_list_ctx.vi_chn[0].bMirror = RK_FALSE;
    g_video_param_list_ctx.vi_chn[0].bFlip = RK_FALSE;
#endif

#if VI_CHN1_ENABLE
    g_video_param_list_ctx.vi_chn[1].enable = VI_CHN1_ENABLE;
    g_video_param_list_ctx.vi_chn[1].ViPipe = 0;
    g_video_param_list_ctx.vi_chn[1].viChnId = 1;
    g_video_param_list_ctx.vi_chn[1].width = 864;
    g_video_param_list_ctx.vi_chn[1].height = 486;
    g_video_param_list_ctx.vi_chn[1].SrcFrameRate = -1;
    g_video_param_list_ctx.vi_chn[1].DstFrameRate = -1;
    g_video_param_list_ctx.vi_chn[1].PixelFormat = RK_FMT_YUV420SP;
    g_video_param_list_ctx.vi_chn[1].bMirror = RK_FALSE;
    g_video_param_list_ctx.vi_chn[1].bFlip = RK_FALSE;
#endif

#if VI_CHN2_ENABLE
    g_video_param_list_ctx.vi_chn[2].enable = VI_CHN2_ENABLE;
    g_video_param_list_ctx.vi_chn[2].ViPipe = 0;
    g_video_param_list_ctx.vi_chn[2].viChnId = 2;
    g_video_param_list_ctx.vi_chn[2].width = 640;
    g_video_param_list_ctx.vi_chn[2].height = 640;
    g_video_param_list_ctx.vi_chn[2].SrcFrameRate = 25;
    g_video_param_list_ctx.vi_chn[2].DstFrameRate = 10;
    g_video_param_list_ctx.vi_chn[2].PixelFormat = RK_FMT_YUV420SP;
    g_video_param_list_ctx.vi_chn[2].bMirror = RK_FALSE;
    g_video_param_list_ctx.vi_chn[2].bFlip = RK_FALSE;
#endif

#if VPSS_ENABLE
    g_video_param_list_ctx.vpss[0].enable = VPSS_ENABLE;
    g_video_param_list_ctx.vpss[0].VpssGrpID = 0;
    g_video_param_list_ctx.vpss[0].inWidth = SENSOR_WIDTH;
    g_video_param_list_ctx.vpss[0].inHeight = SENSOR_HEIGHT;
    g_video_param_list_ctx.vpss[0].inPixelFormat = RK_FMT_YUV420SP;
    g_video_param_list_ctx.vpss[0].bindSrcChn.enModId = RK_ID_VI;
    g_video_param_list_ctx.vpss[0].bindSrcChn.s32DevId = 0;
    g_video_param_list_ctx.vpss[0].bindSrcChn.s32ChnId = 0;
#endif

#if VPSS_CHN0_ENABLE
    g_video_param_list_ctx.vpss[0].chn[0].enable = VPSS_CHN0_ENABLE;
    g_video_param_list_ctx.vpss[0].chn[0].VpssChnID = 0;
    g_video_param_list_ctx.vpss[0].chn[0].outWidth = 960;
    g_video_param_list_ctx.vpss[0].chn[0].outHeight = 540;
    g_video_param_list_ctx.vpss[0].chn[0].SrcFrameRate = 25;
    g_video_param_list_ctx.vpss[0].chn[0].DstFrameRate = 25;
    g_video_param_list_ctx.vpss[0].chn[0].outPixelFormat = RK_FMT_RGB888;
    g_video_param_list_ctx.vpss[0].chn[0].bMirror = RK_FALSE;
    g_video_param_list_ctx.vpss[0].chn[0].bFlip = RK_FALSE;
#endif

#if VPSS_CHN1_ENABLE
    g_video_param_list_ctx.vpss[0].chn[1].enable = VPSS_CHN1_ENABLE;
    g_video_param_list_ctx.vpss[0].chn[1].VpssChnID = 1;
    g_video_param_list_ctx.vpss[0].chn[1].outWidth = SENSOR_WIDTH;
    g_video_param_list_ctx.vpss[0].chn[1].outHeight = SENSOR_HEIGHT;
    g_video_param_list_ctx.vpss[0].chn[1].SrcFrameRate = 25;
    g_video_param_list_ctx.vpss[0].chn[1].DstFrameRate = 25;
    g_video_param_list_ctx.vpss[0].chn[1].outPixelFormat = RK_FMT_YUV420SP;
    g_video_param_list_ctx.vpss[0].chn[1].bMirror = RK_FALSE;
    g_video_param_list_ctx.vpss[0].chn[1].bFlip = RK_FALSE;
#endif

#if VENC_CHN0_ENABLE
    g_video_param_list_ctx.venc[0].enable = VENC_CHN0_ENABLE;
    g_video_param_list_ctx.venc[0].enType = RK_VIDEO_ID_AVC;
    g_video_param_list_ctx.venc[0].vencChnId = 0;
    g_video_param_list_ctx.venc[0].width = SENSOR_WIDTH;
    g_video_param_list_ctx.venc[0].height = SENSOR_HEIGHT;
    g_video_param_list_ctx.venc[0].PixelFormat = RK_FMT_YUV420SP;
    g_video_param_list_ctx.venc[0].bindSrcChn.enModId = RK_ID_VI;
    g_video_param_list_ctx.venc[0].bindSrcChn.s32DevId = 0;
    g_video_param_list_ctx.venc[0].bindSrcChn.s32ChnId = 0;
#endif

#if RGN_CHN0_ENABLE
    g_video_param_list_ctx.rgn[0].enable = RGN_CHN0_ENABLE;
    g_video_param_list_ctx.rgn[0].rgnHandle = 0;
    g_video_param_list_ctx.rgn[0].layer = 0;
    g_video_param_list_ctx.rgn[0].type = OVERLAY_RGN;
    g_video_param_list_ctx.rgn[0].X = 0;
    g_video_param_list_ctx.rgn[0].Y = 0;
    g_video_param_list_ctx.rgn[0].width = SENSOR_WIDTH;
    g_video_param_list_ctx.rgn[0].height = SENSOR_HEIGHT;
    g_video_param_list_ctx.rgn[0].show = RK_TRUE;
    g_video_param_list_ctx.rgn[0].mppChn.enModId = RK_ID_VENC;
    g_video_param_list_ctx.rgn[0].mppChn.s32DevId = 0;
    g_video_param_list_ctx.rgn[0].mppChn.s32ChnId = 0;
    g_video_param_list_ctx.rgn[0].overlay.format = RK_FMT_BGRA5551;
    g_video_param_list_ctx.rgn[0].overlay.u32FgAlpha = 255;
    g_video_param_list_ctx.rgn[0].overlay.u32BgAlpha = 0;
#endif

#if IVA_ENABLE
    g_video_param_list_ctx.iva[0].enable = IVA_ENABLE;
    g_video_param_list_ctx.iva[0].models_path = "/oem/rockiva_data/";
    g_video_param_list_ctx.iva[0].width = 576;
    g_video_param_list_ctx.iva[0].height = 324;
    g_video_param_list_ctx.iva[0].IvaPixelFormat = ROCKIVA_IMAGE_FORMAT_YUV420SP_NV12;
    // g_video_param_list_ctx.iva[0].result_cb = rv1106_iva_result_cb;
#endif
}



rv1106_video_init_param_t *get_video_param_list(void)
{
    return &g_video_param_list_ctx;
}

video_isp_param_t *get_isp_param(void)
{
    return &g_video_param_list_ctx.isp[0];
}

video_vi_dev_param_t *get_vi_dev_param(void)
{
    return &g_video_param_list_ctx.vi_dev[0];
}

video_vi_chn_param_t *get_vi_chn_param(void)
{
    return &g_video_param_list_ctx.vi_chn[0];
}

video_vpss_param_t *get_vpss_param(void)
{
    return &g_video_param_list_ctx.vpss[0];
}

video_venc_param_t *get_venc_param(void)
{
    return &g_video_param_list_ctx.venc[0];
}

video_rgn_param_t *get_rgn_param(void)
{
    return &g_video_param_list_ctx.rgn[0];
}

video_iva_param_t *get_iva_param(void)
{
    return &g_video_param_list_ctx.iva[0];
}
