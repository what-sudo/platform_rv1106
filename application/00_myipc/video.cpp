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
#include <semaphore.h>

#include "im2d.h"
#include "RgaUtils.h"

#include "video.h"
#include "rv1106_video_init.h"
#include "init_param.h"
#include "dma_alloc.h"

#include "main.h"

typedef struct {
    uint32_t X;
    uint32_t Y;
    uint32_t color;
    char text[32];
} screen_text_info_t;

typedef struct {
    int count;
    screen_text_info_t list[32];
} screen_text_list_t;

typedef struct {
    pthread_rwlock_t rwlock;
    uint32_t frameId;
    uint32_t objNum;                /* 目标个数 */
    RockIvaObjectInfo objInfo[128]; /* 各目标检测信息 */
} iva_detect_result_t;

typedef struct {
    iva_detect_result_t result[2];
    pthread_t push_thread_id;
} iva_detect_info_t;

static iva_detect_info_t g_iva_detect_info = {0};
static screen_text_list_t g_screen_text_list = {0};

static bool g_thread_run = true;

iva_detect_result_t *get_iva_result_buffer(int write_flag)
{
    int min_id = 0;
    if (g_iva_detect_info.result[0].frameId <= g_iva_detect_info.result[1].frameId) {
        min_id = 0;
    } else {
        min_id = 1;
    }

    if (write_flag == 0) {
        min_id = 1 - min_id;
        pthread_rwlock_rdlock(&g_iva_detect_info.result[min_id].rwlock);
    } else {
        pthread_rwlock_wrlock(&g_iva_detect_info.result[min_id].rwlock);
    }

    return &g_iva_detect_info.result[min_id];
}

// static RK_U64 TEST_COMM_GetNowUs() {
//     struct timespec time = {0, 0};
//     clock_gettime(CLOCK_MONOTONIC, &time);
//     return (RK_U64)time.tv_sec * 1000000 + (RK_U64)time.tv_nsec / 1000; /* microseconds */
// }

static int rv1106_iva_result_cb(video_iva_callback_param_t *ctx)
{
    RK_S32 s32Ret = RK_FAILURE;

    if (!g_thread_run) {
        return 0;
    }

    iva_detect_result_t *result = get_iva_result_buffer(1);
    result->objNum = ctx->objNum;
    result->frameId = ctx->frameId;
    memcpy(result->objInfo, ctx->objInfo, sizeof(RockIvaObjectInfo) * ctx->objNum);
    pthread_rwlock_unlock(&result->rwlock);

    // static uint64_t last_timestamp = 0;
    // uint64_t new_timestamp = TEST_COMM_GetNowUs();
    // printf("IVA result ---> seq:%d delay:%dms fps:%.1f\n", ctx->objInfo[0].frameId, (uint32_t)(new_timestamp - last_timestamp) / 1000, (1000.0 / ((new_timestamp - last_timestamp) / 1000)));
    // last_timestamp = new_timestamp;

    return s32Ret;
}

#if 0
static int osd_init(video_rgn_param_t *rgn)
{
    RK_S32 s32Ret = RK_FAILURE;
    RGN_CANVAS_INFO_S CanvasInfo = {0};
    do {
        s32Ret = rv1106_rgn_overlay_get_canvas(&rgn[0], &CanvasInfo);
        if (s32Ret != RK_SUCCESS) {
            printf("[%s %d] error: rv1106_rgn_overlay_get_canvas ret:0x%X\n", __func__, __LINE__, s32Ret);
            break;
        }

        g_graphics_image.width = CanvasInfo.u32VirWidth;
        g_graphics_image.height = CanvasInfo.u32VirHeight;
        g_graphics_image.buf = (uint8_t*)(uint32_t)(CanvasInfo.u64VirAddr);

        switch (CanvasInfo.enPixelFmt) {
            case RK_FMT_BGRA5551: {
                g_graphics_image.fmt = GD_FMT_BGRA5551;
                g_graphics_image.line_length = g_graphics_image.width * 2;
            } break;
            default:
                s32Ret = -2;
                printf("[%s %d] error: graphics fmt unsupport\n", __func__, __LINE__);
                return -1;
        }

        graphics_full(&g_graphics_image, graphics_Clear);

        s32Ret = rv1106_rgn_overlay_set_canvas(&rgn[0], &CanvasInfo);
        if (s32Ret != RK_SUCCESS) {
            printf("[%s %d] error: rv1106_rgn_overlay_set_canvas ret:0x%X\n", __func__, __LINE__, s32Ret);
            break;
        }
        s32Ret = RK_SUCCESS;
    } while (0);

    return s32Ret;
}

static void thread_cleanup(void *arg) {
    printf("执行清理: 清理资源\n");
}

static void* osd_update_thread(void* arg) {

    RK_S32 s32Ret = RK_FAILURE;
    uint32_t i;
    RK_U32 X1, Y1, X2, Y2;
    graphics_color_t color = {0};
    video_rgn_param_t *rgn = get_rgn_param();
    char text_buf[32] = { 0 };
    RGN_CANVAS_INFO_S CanvasInfo = {0};

    // 设置线程为可取消的
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    // 设置取消类型为延迟取消
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    // 注册清理函数
    pthread_cleanup_push(thread_cleanup, NULL);

    while(1) {
        sem_wait(&g_iva_result_ctx.sem);  // 请求访问资源
        pthread_mutex_lock(&g_iva_result_ctx.mutex);

        g_screen_iva_ctx.objNum = g_iva_result_ctx.objNum;
        memcpy(g_screen_iva_ctx.objInfo, g_iva_result_ctx.objInfo, sizeof(RockIvaObjectInfo) * g_iva_result_ctx.objNum);

        s32Ret = rv1106_rgn_overlay_get_canvas(rgn, &CanvasInfo);
        if (s32Ret != RK_SUCCESS) {
            printf("[%s %d] error: rv1106_rgn_overlay_get_canvas ret:0x%X\n", __func__, __LINE__, s32Ret);
            // return s32Ret;
        }
        g_graphics_image.buf = (uint8_t*)(uint32_t)(CanvasInfo.u64VirAddr);
        graphics_full(&g_graphics_image, graphics_Clear);

        for (i = 0; i < g_iva_result_ctx.objNum; i++) {
            if (g_iva_result_ctx.objInfo[i].score < 40)
                continue;

            X1 = ROCKIVA_RATIO_PIXEL_CONVERT(SENSOR_WIDTH, g_iva_result_ctx.objInfo[i].rect.topLeft.x);
            Y1 = ROCKIVA_RATIO_PIXEL_CONVERT(SENSOR_HEIGHT, g_iva_result_ctx.objInfo[i].rect.topLeft.y);
            X2 = ROCKIVA_RATIO_PIXEL_CONVERT(SENSOR_WIDTH, g_iva_result_ctx.objInfo[i].rect.bottomRight.x);
            Y2 = ROCKIVA_RATIO_PIXEL_CONVERT(SENSOR_HEIGHT, g_iva_result_ctx.objInfo[i].rect.bottomRight.y);

            if (X1 > SENSOR_WIDTH || Y1 > SENSOR_HEIGHT || X2 > SENSOR_WIDTH || Y2 > SENSOR_HEIGHT) {
                // printf("[%s %d] error: ---\n", __func__, __LINE__);
                // printf("obj:%d/%d %s X1:%u Y1:%u X2:%u Y2:%u\n", i + 1, g_iva_result_ctx.objNum, objname, (uint16_t)X1, (uint16_t)Y2, (uint16_t)X2, (uint16_t)Y2);
            } else {
                // printf("req:%d objNum:%d/%d %s %d%% X1:%u Y1:%u X2:%u Y2:%u\n", g_iva_result_ctx.objInfo[i].frameId, i + 1, g_iva_result_ctx.objNum, objname, g_iva_result_ctx.objInfo[i].score, (uint16_t)X1, (uint16_t)Y2, (uint16_t)X2, (uint16_t)Y2);

                color = graphics_Red;
                snprintf(text_buf, sizeof(text_buf), "%s %d%%", iva_object_name[g_iva_result_ctx.objInfo[i].type], g_iva_result_ctx.objInfo[i].score);
                graphics_rectangle(&g_graphics_image, X1, Y1, X2, Y2, color, 0);
                graphics_show_string(&g_graphics_image, X1 + 4, Y1 + 4, text_buf, GD_FONT_16x32B, color, 0);
            }
        }
        pthread_mutex_unlock(&g_iva_result_ctx.mutex);

        s32Ret = rv1106_rgn_overlay_set_canvas(rgn, &CanvasInfo);
        if (s32Ret != RK_SUCCESS) {
            printf("[%s %d] error: rv1106_rgn_overlay_set_canvas ret:0x%X\n", __func__, __LINE__, s32Ret);
            // return s32Ret;
        }
    }

    // 移除清理函数
    pthread_cleanup_pop(1);
    return NULL;
}
#endif

static void *iva_push_frame_thread(void *pArgs)
{
    int video_ret = -1;

    // // 设置线程为可取消的
    // pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    // // 设置取消类型为延迟取消
    // pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    // // 注册清理函数
    // pthread_cleanup_push(thread_cleanup, NULL);

    printf("[%s %d] Start iva push stream thread......\n", __FILE__, __LINE__);

    while (g_thread_run) {
        video_ret = video_GetFrame(GET_IVA_FRAME, NULL, NULL);
        if (!video_ret) {
            // static uint64_t last_timestamp = 0;
            // printf("GET IVA ---> seq:%d w:%d h:%d fmt:%d size:%lld delay:%dms fps:%.1f\n", fvi_info.frame_seq, fvi_info.width, fvi_info.height, fvi_info.PixelFormat, fvi_info.frame_size, (uint32_t)(fvi_info.timestamp - last_timestamp) / 1000, (1000.0 / ((fvi_info.timestamp - last_timestamp) / 1000)));
            // last_timestamp = fvi_info.timestamp;
        }
        usleep(10 * 1000);
    }

    // 移除清理函数
    // pthread_cleanup_pop(1);

    return RK_NULL;
}

int video_save_file(uint32_t frame_seq, uint64_t frame_size, uint8_t *frame_data)
{
    static int frame_index = 0;
    static FILE* save_file = NULL;

    int start_frame = 10;
    int save_num = 5;

    if (!save_file && frame_index < (start_frame + save_num)) {
        printf("open file\n");
        save_file = fopen("./frame.bin", "w");
    }

    if (frame_index++ < (start_frame + save_num) && frame_index > start_frame) {
        printf("write seq:%d index:%d\n", frame_seq, frame_index - 10);
        fwrite(frame_data, 1, frame_size, save_file);
        fflush(save_file);
    }

    if (save_file && frame_index > (start_frame + save_num)) {
        printf("close file\n");
        fclose(save_file);
        save_file = NULL;
    }

    return 0;
}

int video_init(void)
{
    RK_S32 s32Ret = RK_FAILURE;

    init_video_param_list();

    s32Ret = rv1106_video_init(get_video_param_list());
    if (s32Ret != RK_SUCCESS) {
        printf("[%s %d] error: rv1106_video_init fail: ret:0x%x\n", __func__, __LINE__, s32Ret);
        return s32Ret;
    }

    video_iva_param_t *iva = get_iva_param();
    if (iva->enable) {
        iva->result_cb = rv1106_iva_result_cb;
        for (int i = 0; i < (int)(sizeof(g_iva_detect_info.result) / sizeof(g_iva_detect_info.result[0])); i++) {
            pthread_rwlock_init(&g_iva_detect_info.result[i].rwlock, NULL);
        }
        pthread_create(&g_iva_detect_info.push_thread_id, 0, iva_push_frame_thread, NULL);
    }

    // video_rgn_param_t *rgn = get_rgn_param();
    // if (rgn->enable) {
    //     s32Ret = osd_init(rgn);
    //     if (s32Ret != RK_SUCCESS) {
    //         printf("[%s %d] error: osd_init fail: ret:0x%x\n", __func__, __LINE__, s32Ret);
    //         return s32Ret;
    //     }
    // }

    return s32Ret;
}

int video_deinit(void)
{
    RK_S32 s32Ret = RK_FAILURE;

    g_thread_run = false;

    video_iva_param_t *iva = get_iva_param();
    if (iva->enable) {
        if (g_iva_detect_info.push_thread_id) {
            printf("[%s %d] wait iva thread joid\n", __FILE__, __LINE__);
            // pthread_cancel(g_iva_detect_info.push_thread_id);
            pthread_join(g_iva_detect_info.push_thread_id, NULL);
        }

        for (int i = 0; i < (int)(sizeof(g_iva_detect_info.result) / sizeof(g_iva_detect_info.result[0])); i++) {
            pthread_rwlock_destroy(&g_iva_detect_info.result[i].rwlock);
        }
    }

    // video_rgn_param_t *rgn = get_rgn_param();
    // if (rgn->enable) {
    //     pthread_cancel(g_iva_result_ctx.threadId);
    //     pthread_join(g_iva_result_ctx.threadId, NULL);
    //     pthread_mutex_destroy(&g_iva_result_ctx.mutex);

    //     s32Ret = sem_destroy(&g_iva_result_ctx.sem);  // 销毁信号量
    //     if (s32Ret != RK_SUCCESS) {
    //         printf("error: sem_destroy fail: ret:0x%x\n", s32Ret);
    //         return s32Ret;
    //     }
    // }

    rv1106_video_init_param_t *video_param = get_video_param_list();
    s32Ret = rv1106_video_deinit(video_param);
    if (s32Ret != RK_SUCCESS) {
        printf("[%s %d] error: rv1106_video_deinit fail: ret:0x%x\n", __func__, __LINE__, s32Ret);
        return s32Ret;
    }

    return s32Ret;
}

static void update_system_status(void)
{
    system_status_t *system_status = get_system_status();

    g_screen_text_list.list[g_screen_text_list.count].X = 0;
    g_screen_text_list.list[g_screen_text_list.count].Y = 0;
    g_screen_text_list.list[g_screen_text_list.count].color = 0xff00ff00;
    snprintf(g_screen_text_list.list[g_screen_text_list.count].text, sizeof(g_screen_text_list.list[g_screen_text_list.count].text), "CPU:%.1f%% TEMP:%.1f.C NPU:%.1f%%", (float)(system_status->cpu_usage / 10), (float)(system_status->cpu_temp / 10), (float)(system_status->npu_usage / 10));

    g_screen_text_list.count++;
}

int video_update_screen(uint8_t *screen_buf, int flip)
{
    RK_S32 s32Ret = RK_FAILURE;
    VIDEO_FRAME_INFO_S stViFrame;

    static uint8_t *dst_buf = NULL;
    static int dst_dma_fd = 0;
    static rga_buffer_handle_t dst_handle = 0;
    static rga_buffer_t dst_img = {0};
    static im_rect crop_rect;

    static graphics_image_t graphics_image = {0};

    int dst_width = 800;
    int dst_height = 480;

    int object_number = 0;
    im_rect obj_rect[16] = {};

    video_vi_chn_param_t *vi_chn = get_vi_chn_param();
    vi_chn = &vi_chn[1];

    if (screen_buf == NULL) {
        printf("[%s %d] error: screen_buf is NULL\n", __func__, __LINE__);
        return s32Ret;
    }

    s32Ret = RK_MPI_VI_GetChnFrame(vi_chn->ViPipe, vi_chn->viChnId, &stViFrame, 1000);
    if (s32Ret != RK_SUCCESS) {
        printf("[%s %d] error: RK_MPI_VI_GetChnFrame fail: ret:0x%X\n", __func__, __LINE__, s32Ret);
    }

#if 0
    static uint64_t last_timestamp = 0;
    printf("GET SCREEN ---> seq:%d w:%d h:%d fmt:%d size:%lld delay:%dms fps:%.1f\n", stViFrame.stVFrame.u32TimeRef, stViFrame.stVFrame.u32Width, stViFrame.stVFrame.u32Height, stViFrame.stVFrame.enPixelFormat, stViFrame.stVFrame.u64PrivateData, (uint32_t)(stViFrame.stVFrame.u64PTS - last_timestamp) / 1000, (1000.0 / ((stViFrame.stVFrame.u64PTS - last_timestamp) / 1000)));
    last_timestamp = stViFrame.stVFrame.u64PTS;
#endif

    int mpi_fd = RK_MPI_MB_Handle2Fd(stViFrame.stVFrame.pMbBlk);

    if (dst_buf == NULL) {
        int dst_buf_size = dst_width * dst_height * get_bpp_from_format(RK_FORMAT_YCbCr_420_SP);
        printf("alloc dst dma_heap buffer\n");
        s32Ret = dma_buf_alloc(RV1106_CMA_HEAP_PATH, dst_buf_size, &dst_dma_fd, (void **)&dst_buf);
        if (s32Ret < 0) {
            printf("[%s %d] error: alloc dst dma_heap buffer failed! ret:%d\n", __func__, __LINE__, s32Ret);
            return s32Ret;
        }
        printf("dst_dma_fd:%d dst_buf:%p dst_buf_size:%d\n", dst_dma_fd, dst_buf, dst_buf_size);

        dst_handle = importbuffer_fd(dst_dma_fd, dst_buf_size);
        dst_img = wrapbuffer_handle(dst_handle, dst_width, dst_height, RK_FORMAT_YCbCr_420_SP);

        crop_rect.x = (stViFrame.stVFrame.u32Width - dst_width) / 2;
        crop_rect.y = (stViFrame.stVFrame.u32Height - dst_height) / 2;
        crop_rect.width = dst_width;
        crop_rect.height = dst_height;
        crop_rect.x = crop_rect.x % 2 ? crop_rect.x + 1 : crop_rect.x;
        crop_rect.y = crop_rect.y % 2 ? crop_rect.y + 1 : crop_rect.y;
    }

    rga_buffer_handle_t src_handle = importbuffer_fd(mpi_fd, stViFrame.stVFrame.u64PrivateData);
    rga_buffer_t src_img = wrapbuffer_handle(src_handle, stViFrame.stVFrame.u32Width, stViFrame.stVFrame.u32Height, RK_FORMAT_YCbCr_420_SP);

    // imsetColorSpace(&src_img, IM_YUV_TO_RGB_BT601_LIMIT);
    // imsetColorSpace(&dst_img, IM_RGB_FULL);

    do {
        // s32Ret = imcheck(src_img, dst_img, {}, {});
        // if (IM_STATUS_NOERROR != s32Ret) {
        //     printf("%d, check error! %s \n", __LINE__, imStrError((IM_STATUS)s32Ret));
        //     s32Ret = -1;
        //     break;
        // }

        uint32_t X1, Y1, X2, Y2;
        int X, Y, W, H;
        iva_detect_result_t *result = get_iva_result_buffer(0);
        for (int i = 0; i < (int)result->objNum; i++) {
            // if (result->objInfo[i].score < 30)
            //     continue;

            X1 = ROCKIVA_RATIO_PIXEL_CONVERT(stViFrame.stVFrame.u32Width, result->objInfo[i].rect.topLeft.x);
            Y1 = ROCKIVA_RATIO_PIXEL_CONVERT(stViFrame.stVFrame.u32Height, result->objInfo[i].rect.topLeft.y);
            X2 = ROCKIVA_RATIO_PIXEL_CONVERT(stViFrame.stVFrame.u32Width, result->objInfo[i].rect.bottomRight.x);
            Y2 = ROCKIVA_RATIO_PIXEL_CONVERT(stViFrame.stVFrame.u32Height, result->objInfo[i].rect.bottomRight.y);

            if (X1 < stViFrame.stVFrame.u32Width && Y1 < stViFrame.stVFrame.u32Height && X2 < stViFrame.stVFrame.u32Width && Y2 < stViFrame.stVFrame.u32Height) {
                X = X1;
                Y = Y1;
                W = X2 - X1;
                H = Y2 - Y1;
                X = X % 2 ? X + 1 : X;
                Y = Y % 2 ? Y + 1 : Y;
                W = W % 2 ? W + 1 : W;
                H = H % 2 ? H + 1 : H;

                if (object_number < (int)(sizeof(obj_rect) / sizeof(obj_rect[0]))) {
                    g_screen_text_list.list[g_screen_text_list.count].X = X1 > (uint32_t)crop_rect.x ? X1 - (uint32_t)crop_rect.x : 0;
                    g_screen_text_list.list[g_screen_text_list.count].Y = Y1 > (uint32_t)crop_rect.y ? Y1 - (uint32_t)crop_rect.y : 0;
                    g_screen_text_list.list[g_screen_text_list.count].color = iva_object_color[result->objInfo[i].type];

                    snprintf(g_screen_text_list.list[g_screen_text_list.count].text, sizeof(g_screen_text_list.list[g_screen_text_list.count].text), "%s %d%%", iva_object_name[result->objInfo[i].type], result->objInfo[i].score);
                    obj_rect[object_number] = {X, Y, W, H};
                }
                g_screen_text_list.count = g_screen_text_list.count == (sizeof(g_screen_text_list.list) / sizeof(g_screen_text_list.list[0])) - 1 ? g_screen_text_list.count : g_screen_text_list.count + 1;

                object_number++;
            }
        }
        pthread_rwlock_unlock(&result->rwlock);

        if (object_number > 0) {
            // s32Ret = imcheck({}, src_img, {}, obj_rect[0], IM_COLOR_FILL);
            // if (IM_STATUS_NOERROR != s32Ret) {
            //     printf("%d, check error! %s", __LINE__, imStrError((IM_STATUS)s32Ret));
            //     break;
            // }

            s32Ret = imrectangleArray(src_img, obj_rect, object_number, 0xff00ff00, 2);
            if (s32Ret != IM_STATUS_SUCCESS) {
                printf("%d imrectangleArray running failed, %s\n", __LINE__, imStrError((IM_STATUS)s32Ret));
                s32Ret = -1;
                break;
            }
        }

        s32Ret = imcrop(src_img, dst_img, crop_rect);
        if (s32Ret != IM_STATUS_SUCCESS) {
            printf("%d imcrop running failed, %s\n", __LINE__, imStrError((IM_STATUS)s32Ret));
            s32Ret = -1;
            break;
        }

        if (flip) {
            IM_USAGE rotate = IM_HAL_TRANSFORM_FLIP_H;
            if (flip == 1)
                rotate = IM_HAL_TRANSFORM_FLIP_H;
            else if (flip == 2)
                rotate = IM_HAL_TRANSFORM_FLIP_V;
            else if (flip == 3)
                rotate = IM_HAL_TRANSFORM_FLIP_H_V;

            s32Ret = imrotate(dst_img, src_img, rotate);
            if (s32Ret != IM_STATUS_SUCCESS) {
                printf("%d imrotate running failed, %s\n", __LINE__, imStrError((IM_STATUS)s32Ret));
                s32Ret = -1;
                break;
            }
        }

        // video_save_file(stViFrame.stVFrame.u32TimeRef, dst_buf_size, dst_buf);

        int screen_buf_size = dst_width * dst_height * get_bpp_from_format(RK_FORMAT_BGRA_8888);
        rga_buffer_handle_t screen_handle = importbuffer_virtualaddr(screen_buf, screen_buf_size);

        rga_buffer_t screen_img = wrapbuffer_handle(screen_handle, dst_width, dst_height, RK_FORMAT_BGRA_8888);

        s32Ret = imcvtcolor(src_img, screen_img, RK_FORMAT_YCbCr_420_SP, RK_FORMAT_BGRA_8888);
        if (s32Ret != IM_STATUS_SUCCESS) {
            printf("%s running failed, %s\n", "imcvtcolor", imStrError((IM_STATUS)s32Ret));
        }

        update_system_status();

        graphics_image.width = dst_width;
        graphics_image.height = dst_height;
        graphics_image.fmt = GD_FMT_BGRA8888;
        graphics_image.line_length = graphics_image.width * 4;
        graphics_image.buf = (uint8_t*)screen_buf;

        if (g_screen_text_list.count > 0) {
            graphics_color_t color;
            for (int i = 0; i < g_screen_text_list.count; i++) {
                color.a = (g_screen_text_list.list[i].color & 0xff000000) >> 24;
                color.r = (g_screen_text_list.list[i].color & 0xff0000) >> 16;
                color.g = (g_screen_text_list.list[i].color & 0x00ff00) >> 8;
                color.b = g_screen_text_list.list[i].color & 0x0000ff;
                graphics_show_string(&graphics_image, g_screen_text_list.list[i].X, g_screen_text_list.list[i].Y, g_screen_text_list.list[i].text, GD_FONT_16x32B, color, flip);
            }
            g_screen_text_list.count = 0;
        }

        if (screen_handle > 0)
            releasebuffer_handle(screen_handle);

        // video_save_file(stViFrame.stVFrame.u32TimeRef, screen_buf_size, (uint8_t *)screen_buf);

        s32Ret = 0;
    } while (0);

    if (src_handle > 0)
        releasebuffer_handle(src_handle);

    // if (dst_handle > 0)
    //     releasebuffer_handle(dst_handle);

    // dma_buf_free(dst_buf_size, &dst_dma_fd, dst_buf);

    s32Ret = RK_MPI_VI_ReleaseChnFrame(vi_chn->ViPipe, vi_chn->viChnId, &stViFrame);
    if (s32Ret != RK_SUCCESS) {
        printf("error: RK_MPI_VI_ReleaseChnFrame fail chn:%d 0x%X\n", vi_chn->viChnId, s32Ret);
    }

    return s32Ret;
}

int video_GetFrame(get_frame_type_t type, frameInfo_vi_t *fvi_info, void *arg)
{
    RK_S32 s32Ret = RK_FAILURE;

    if (type == GET_SCREEN_FRAME) {
        if (fvi_info == NULL) {
            printf("[%s %d] error: fvi_info is NULL\n", __func__, __LINE__);
            return s32Ret;
        }

        video_vi_chn_param_t *vi_chn = get_vi_chn_param();
        vi_chn = &vi_chn[1];

        s32Ret = rv1106_vichn_GetStream(vi_chn, fvi_info);
        if (s32Ret != RK_SUCCESS) {
            printf("error: GET_SCREEN_FRAME fail: ret:0x%x\n", s32Ret);
            return s32Ret;
        }
        return s32Ret;
    } else if (type == GET_VENC_FRAME) {
        video_venc_param_t *venc = get_venc_param();
        s32Ret = rv1106_venc_GetStream(&venc[0], fvi_info);
        if (s32Ret != RK_SUCCESS) {
            printf("error: GET_VENC_FRAME fail: ret:0x%x\n", s32Ret);
            return s32Ret;
        }
    } else if (type == GET_RTSP_FRAME) {
        video_venc_param_t *venc = get_venc_param();
        s32Ret = rv1106_venc_GetStream(&venc[0], fvi_info);
        if (s32Ret != RK_SUCCESS) {
            printf("error: GET_RTSP_FRAME fail: ret:0x%x\n", s32Ret);
            return s32Ret;
        }
    } else if (type == GET_IVA_FRAME) {
        video_iva_param_t *iva = get_iva_param();
        video_vi_chn_param_t *vi_chn = get_vi_chn_param();

        iva = &iva[0];
        vi_chn = &vi_chn[2];

        if (iva->enable) {
            frameInfo_vi_t fvi_info;

            s32Ret = rv1106_vichn_GetStream_fd(vi_chn, &fvi_info);
            if (s32Ret != RK_SUCCESS) {
                printf("error: rv1106_vichn_GetStream_fd fail: ret:0x%X\n", s32Ret);
                return s32Ret;
            }

            rv1106_iva_push_frame_fd(iva, &fvi_info);

            s32Ret = rv1106_vichn_ReleaseStream_fd(vi_chn, &fvi_info);
            if (s32Ret != RK_SUCCESS) {
                printf("error: rv1106_vichn_ReleaseStream_fd fail: ret:0x%X\n", s32Ret);
                return s32Ret;
            }
        } else {
            if (fvi_info == NULL) {
                printf("[%s %d] error: fvi_info is NULL\n", __func__, __LINE__);
                return s32Ret;
            }

            s32Ret = rv1106_vichn_GetStream(vi_chn, fvi_info);
            if (s32Ret != RK_SUCCESS) {
                printf("error: GET_IVA_FRAME fail: ret:0x%X\n", s32Ret);
                return s32Ret;
            }
        }
    }

    return s32Ret;
}
