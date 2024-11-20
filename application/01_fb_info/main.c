#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <sys/mman.h>

#include "data_type.h"

struct fb_fix_screeninfo fb_fix;
struct fb_var_screeninfo fb_var;

screen_info_t scr_info = {0};

int display_device_info(int fd)
{
    /* 获取参数信息 */
    ioctl(fd, FBIOGET_VSCREENINFO, &fb_var);
    ioctl(fd, FBIOGET_FSCREENINFO, &fb_fix);
    printf("分辨率: %d*%d\n", fb_var.xres, fb_var.yres);
    printf("像素深度bpp: %d\n", fb_var.bits_per_pixel);
    printf("一行的字节数: %d\n", fb_fix.line_length);
    printf("显存长度：%d\n", fb_fix.smem_len);
    printf("像素格式: R<%d %d> G<%d %d> B<%d %d>\n",
        fb_var.red.offset, fb_var.red.length,
        fb_var.green.offset, fb_var.green.length,
        fb_var.blue.offset, fb_var.blue.length);
    return 0;
}

int main(int argc, char *argv[])
{
    int ret = -1;
    int fd;

    /* 传参校验 */
    if (2 != argc) {
        fprintf(stderr, "usage: %s </dev/fbX>\n", argv[0]);
        exit(-1);
    }

    /* 打开framebuffer设备 */
    if (0 > (fd = open(argv[1], O_RDWR))) {
        perror("open fb device error");
        exit(-1);
    }

    display_device_info(fd);

    scr_info.screen_size = fb_fix.smem_len;
    scr_info.line_length = fb_fix.line_length;
    scr_info.width = fb_var.xres;
    scr_info.height = fb_var.yres;

    do {

        if (scr_info.screen_base == NULL) {
            scr_info.screen_base = (unsigned char *)mmap(NULL, scr_info.screen_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            if (MAP_FAILED == (void *)scr_info.screen_base) {
                perror("mmap error");
                return -1;
            }

            memcpy(scr_info.screen_base, line_buf, min_bytes);
        }

        ret = 0;
    } while (0);

    if (scr_info.screen_base) {
        /* 退出 */
        munmap(scr_info.screen_base, scr_info.screen_size);  //取消映射
    }

    /* 关闭设备文件退出程序 */
    close(fd);
    return ret;
}
