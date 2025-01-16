#ifndef __MAIN_H__
#define __MAIN_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int cpu_usage;
    int cpu_temp;
    int npu_usage;
} system_status_t;

system_status_t *get_system_status(void);

#ifdef __cplusplus
}
#endif

#endif
