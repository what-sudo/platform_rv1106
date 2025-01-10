#ifndef __POSTPROCESS_H__
#define __POSTPROCESS_H__

#define OBJ_CLASS_NUM 80

int init_post_process(const char *label_name_txt_path);
void deinit_post_process(void);
int post_process(rknn_app_ctx_t *app_ctx, void *outputs,  float conf_threshold, float nms_threshold, object_detect_result_list *od_results);

char *coco_cls_to_name(int cls_id);

#endif
