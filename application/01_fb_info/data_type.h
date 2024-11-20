#ifndef __DATA_TYPE_H__
#define __DATA_TYPE_H__

typedef struct {
    int width;                       //LCD X分辨率
    int height;                      //LCD Y分辨率
    unsigned char *screen_base;     //映射后的显存基地址
    unsigned long line_length;       //LCD一行的长度（字节为单位）
    unsigned int screen_size;        // 显存总大小
} screen_info_t;

#endif
