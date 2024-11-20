#ifndef __DATA_TYPE_H__
#define __DATA_TYPE_H__

#include <linux/fb.h>
#include <sys/mman.h>
#include <linux/videodev2.h>

#define DEVICE_INFO_BUF_MAX 10

typedef struct {
    int dev_fd;
    struct fb_fix_screeninfo fb_fix;
    struct fb_var_screeninfo fb_var;

    int width;                    //LCD X分辨率
    int height;                   //LCD Y分辨率
    unsigned char *screen_base;   //映射后的显存基地址
    unsigned long line_length;    //LCD一行的长度（字节为单位）
    unsigned int screen_size;     // 显存总大小
    int bits_pixel;               // 像素深度
} screen_info_t;

#endif
