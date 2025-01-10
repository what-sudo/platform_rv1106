#ifndef __RKNN_APP_H__
#define __RKNN_APP_H__

#include "rknn_api.h"

// #define OBJ_NAME_MAX_SIZE 64
#define OBJ_NUMB_MAX_SIZE 128
#define OBJ_CLASS_NUM 80
#define NMS_THRESH 0.45
#define BOX_THRESH 0.25
#define PROP_BOX_SIZE (5 + OBJ_CLASS_NUM)

typedef struct {
    int left;
    int top;
    int right;
    int bottom;
} image_rect_t;

typedef struct {
    image_rect_t box;
    float prop;
    int cls_id;
} object_detect_result;

typedef struct {
    int id;
    int count;
    object_detect_result results[OBJ_NUMB_MAX_SIZE];
} object_detect_result_list;

typedef struct {
    const char *model_path;

    rknn_context rknn_ctx;

    rknn_input_output_num io_num;
    rknn_tensor_attr* input_attrs;
    rknn_tensor_attr* output_attrs;
    rknn_tensor_mem* net_mem;

    rknn_tensor_mem* input_mems[1];
    rknn_tensor_mem* output_mems[3];
    // rknn_dma_buf img_dma_buf;

    int model_channel;
    int model_width;
    int model_height;
    bool is_quant;

} rknn_app_ctx_t;

int rknn_app_init(rknn_app_ctx_t *app_ctx);
int rknn_app_deinit(rknn_app_ctx_t *app_ctx);

int inference_model(rknn_app_ctx_t *app_ctx, object_detect_result_list *od_results);

#endif
