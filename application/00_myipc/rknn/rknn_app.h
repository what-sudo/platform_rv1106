#ifndef __RKNN_APP_H__
#define __RKNN_APP_H__

#include "rknn_api.h"
#include "postprocess.h"

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

#endif
