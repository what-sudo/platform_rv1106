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

#include "rknn_app.h"
#include "postprocess.h"

#define RKNN_ENABLE 1

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

typedef struct {
    float X;
    float Y;
    float W;
    float H;
    float prop;
    int cls_id;
    char name[32];
} rknn_detect_result_t;

typedef struct {
    pthread_rwlock_t rwlock;
    int id;
    int objNum;
    rknn_detect_result_t results[OBJ_NUMB_MAX_SIZE];
} rknn_detect_info_t;

static screen_text_list_t g_screen_text_list = {0};
static bool g_thread_run = true;

#if RKNN_ENABLE
static rknn_detect_info_t g_rknn_detect_info = {0};
static rknn_app_ctx_t g_rknn_app_ctx = { 0 };
static pthread_t rknn_push_thread_id;
#endif

#if IVA_ENABLE
static iva_detect_info_t g_iva_detect_info = {0};

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
#endif

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

#if RKNN_ENABLE
static void *rknn_push_frame_thread(void *pArgs)
{
    int s32Ret = RK_FAILURE;
    printf("[%s %d] Start rknn push stream thread......\n", __FILE__, __LINE__);

    object_detect_result_list od_results = {0};

    VIDEO_FRAME_INFO_S stViFrame;
    video_vi_chn_param_t *vi_chn = get_vi_chn_param();
    vi_chn = &vi_chn[2];

    pthread_rwlock_init(&g_rknn_detect_info.rwlock, NULL);

    int screen_buf_size = 640 * 640 * 3;
    rga_buffer_handle_t rga_rknn_handle = importbuffer_fd(g_rknn_app_ctx.input_mems[0]->fd, screen_buf_size);
    rga_buffer_t rga_rknn_img = wrapbuffer_handle(rga_rknn_handle, 640, 640, RK_FORMAT_BGR_888);

    while (g_thread_run) {

        s32Ret = RK_MPI_VI_GetChnFrame(vi_chn->ViPipe, vi_chn->viChnId, &stViFrame, 1000);
        if (s32Ret != RK_SUCCESS) {
            printf("[%s %d] error: RK_MPI_VI_GetChnFrame fail: ret:0x%X\n", __func__, __LINE__, s32Ret);
        }

#if 0
        static uint64_t last_timestamp = 0;
        printf("GET RKNN Frame ---> seq:%d w:%d h:%d fmt:%d size:%lld delay:%dms fps:%.1f\n", stViFrame.stVFrame.u32TimeRef, stViFrame.stVFrame.u32Width, stViFrame.stVFrame.u32Height, stViFrame.stVFrame.enPixelFormat, stViFrame.stVFrame.u64PrivateData, (uint32_t)(stViFrame.stVFrame.u64PTS - last_timestamp) / 1000, (1000.0 / ((stViFrame.stVFrame.u64PTS - last_timestamp) / 1000)));
        last_timestamp = stViFrame.stVFrame.u64PTS;
#endif

        int mpi_fd = RK_MPI_MB_Handle2Fd(stViFrame.stVFrame.pMbBlk);
        rga_buffer_handle_t src_handle = importbuffer_fd(mpi_fd, stViFrame.stVFrame.u64PrivateData);
        rga_buffer_t src_img = wrapbuffer_handle(src_handle, stViFrame.stVFrame.u32Width, stViFrame.stVFrame.u32Height, RK_FORMAT_YCbCr_420_SP);

        // s32Ret = imcheck(rga_rknn_img, src_img, {}, {});
        // if (IM_STATUS_NOERROR != s32Ret) {
        //     printf("%d, check error! %s", __LINE__, imStrError((IM_STATUS)s32Ret));
        //     break;
        // }

        s32Ret = imcvtcolor(src_img, rga_rknn_img, RK_FORMAT_YCbCr_420_SP, RK_FORMAT_BGR_888);
        if (s32Ret != IM_STATUS_SUCCESS) {
            printf("%s running failed, %s\n", "imcvtcolor", imStrError((IM_STATUS)s32Ret));
        }

        inference_model(&g_rknn_app_ctx, &od_results);

        pthread_rwlock_wrlock(&g_rknn_detect_info.rwlock);
        g_rknn_detect_info.id = stViFrame.stVFrame.u32TimeRef;
        g_rknn_detect_info.objNum = od_results.count;
        if (od_results.count > 0) {
            // printf("od_results.count:%d\n", od_results.count);

            for (int i = 0; i < od_results.count; i++) {
                // printf("od_results.results[%d].cls_id:%d\n", i, od_results.results[i].cls_id);

                object_detect_result *det_result = &(od_results.results[i]);
                g_rknn_detect_info.results[i].X = det_result->box.left / 640.0;
                g_rknn_detect_info.results[i].Y = det_result->box.top / 640.0;
                g_rknn_detect_info.results[i].W = det_result->box.right / 640.0 - g_rknn_detect_info.results[i].X;
                g_rknn_detect_info.results[i].H = det_result->box.bottom / 640.0 - g_rknn_detect_info.results[i].Y;

                g_rknn_detect_info.results[i].cls_id = det_result->cls_id;
                g_rknn_detect_info.results[i].prop = det_result->prop;
                strncpy(g_rknn_detect_info.results[i].name, coco_cls_to_name(det_result->cls_id), sizeof(g_rknn_detect_info.results[i].name));

                // printf("%s @ (%d %d %d %d) %.3f\n", coco_cls_to_name(det_result->cls_id),
                //         x, y, w, h,
                //         det_result->prop);
            }
        }
        pthread_rwlock_unlock(&g_rknn_detect_info.rwlock);

        // video_save_file(stViFrame.stVFrame.u32TimeRef, screen_buf_size, (uint8_t *)g_rknn_app_ctx.input_mems[0]->virt_addr);

        if (src_handle > 0)
            releasebuffer_handle(src_handle);

        s32Ret = RK_MPI_VI_ReleaseChnFrame(vi_chn->ViPipe, vi_chn->viChnId, &stViFrame);
        if (s32Ret != RK_SUCCESS) {
            printf("error: RK_MPI_VI_ReleaseChnFrame fail chn:%d 0x%X\n", vi_chn->viChnId, s32Ret);
        }

        usleep(10 * 1000);
    }

    return RK_NULL;
}

#endif

int video_init(void)
{
    RK_S32 s32Ret = RK_FAILURE;

    init_video_param_list();

    s32Ret = rv1106_video_init(get_video_param_list());
    if (s32Ret != RK_SUCCESS) {
        printf("[%s %d] error: rv1106_video_init fail: ret:0x%x\n", __func__, __LINE__, s32Ret);
        return s32Ret;
    }

#if IVA_ENABLE
    video_iva_param_t *iva = get_iva_param();
    if (iva->enable) {
        iva->result_cb = rv1106_iva_result_cb;
        for (int i = 0; i < (int)(sizeof(g_iva_detect_info.result) / sizeof(g_iva_detect_info.result[0])); i++) {
            pthread_rwlock_init(&g_iva_detect_info.result[i].rwlock, NULL);
        }
        pthread_create(&g_iva_detect_info.push_thread_id, 0, iva_push_frame_thread, NULL);
    }
#endif

#if RKNN_ENABLE
    g_rknn_app_ctx.model_path = "/oem/rknn_model/yolov5.rknn";
    s32Ret = rknn_app_init(&g_rknn_app_ctx);
    if (s32Ret != RK_SUCCESS) {
        printf("[%s %d] error: rknn_app_init fail: ret:0x%x\n", __func__, __LINE__, s32Ret);
        return s32Ret;
    }

    s32Ret = init_post_process("/oem/rknn_model/coco_80_labels_list.txt");
    if (s32Ret != RK_SUCCESS) {
        printf("[%s %d] error: init_post_process fail: ret:0x%x\n", __func__, __LINE__, s32Ret);
        return s32Ret;
    }

    pthread_create(&rknn_push_thread_id, 0, rknn_push_frame_thread, NULL);
#endif

    return s32Ret;
}

int video_deinit(void)
{
    RK_S32 s32Ret = RK_FAILURE;

    g_thread_run = false;

#if IVA_ENABLE
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
#endif

#if RKNN_ENABLE
    if (rknn_push_thread_id) {
        printf("[%s %d] wait rknn thread joid\n", __FILE__, __LINE__);
        pthread_join(rknn_push_thread_id, NULL);
    }

    deinit_post_process();
    rknn_app_deinit(&g_rknn_app_ctx);
#endif

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
    im_rect obj_rect[128] = {};

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

        int X, Y, W, H;
#if IVA_ENABLE
        uint32_t X1, Y1, X2, Y2;
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
#endif

#if RKNN_ENABLE
        pthread_rwlock_rdlock(&g_rknn_detect_info.rwlock);

        if (g_rknn_detect_info.objNum > 0) {
            for (int i = 0; i < g_rknn_detect_info.objNum; i++) {
                // printf("g_rknn_detect_info.results[%d].cls_id:%d\n", i, g_rknn_detect_info.results[i].cls_id);
                X = g_rknn_detect_info.results[i].X * stViFrame.stVFrame.u32Width;
                Y = g_rknn_detect_info.results[i].Y * stViFrame.stVFrame.u32Height;
                W = g_rknn_detect_info.results[i].W * stViFrame.stVFrame.u32Width;
                H = g_rknn_detect_info.results[i].H * stViFrame.stVFrame.u32Height;

                X = X % 2 ? X + 1 : X;
                Y = Y % 2 ? Y + 1 : Y;
                W = W % 2 ? W + 1 : W;
                H = H % 2 ? H + 1 : H;

                g_screen_text_list.list[g_screen_text_list.count].X = X > crop_rect.x ? X - crop_rect.x : 0;
                g_screen_text_list.list[g_screen_text_list.count].Y = Y > crop_rect.y ? Y - crop_rect.y : 0;
                g_screen_text_list.list[g_screen_text_list.count].color = 0xff00ff00;

                snprintf(g_screen_text_list.list[g_screen_text_list.count].text, sizeof(g_screen_text_list.list[g_screen_text_list.count].text), "%s %d%%", g_rknn_detect_info.results[i].name, (int)(g_rknn_detect_info.results[i].prop * 100));

                obj_rect[object_number] = {X, Y, W, H};

                g_screen_text_list.count = g_screen_text_list.count == (sizeof(g_screen_text_list.list) / sizeof(g_screen_text_list.list[0])) - 1 ? g_screen_text_list.count : g_screen_text_list.count + 1;
                object_number++;
            }
        }

        pthread_rwlock_unlock(&g_rknn_detect_info.rwlock);
#endif

        if (object_number > 0) {
            // s32Ret = imcheck({}, src_img, {}, obj_rect[0], IM_COLOR_FILL);
            // if (IM_STATUS_NOERROR != s32Ret) {
            //     printf("%d, check error! %s\n", __LINE__, imStrError((IM_STATUS)s32Ret));
            //     break;
            // }

            s32Ret = imrectangleArray(src_img, obj_rect, object_number, 0xffff0000, 2);
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
            for (int i = 0; i < g_screen_text_list.count; i++) {
                graphics_show_string(&graphics_image, g_screen_text_list.list[i].X, g_screen_text_list.list[i].Y, g_screen_text_list.list[i].text, GD_FONT_16x32B, g_screen_text_list.list[i].color, flip);
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
