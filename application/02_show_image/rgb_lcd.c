#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <sys/mman.h>

#include "data_type.h"

#define argb8888_to_rgb565(color) ({ \
unsigned int temp = (color); \
((temp & 0xF80000UL) >> 8) | \
((temp & 0xFC00UL) >> 5) | \
((temp & 0xF8UL) >> 3); \
})

static int display_device_info(screen_info_t *scr_dev)
{
    /* 获取参数信息 */
    printf("分辨率: %d*%d\n", scr_dev->fb_var.xres, scr_dev->fb_var.yres);
    printf("像素深度bpp: %d\n", scr_dev->fb_var.bits_per_pixel);
    printf("一行的字节数: %d\n", scr_dev->fb_fix.line_length);
    printf("显存长度：%d\n", scr_dev->fb_fix.smem_len);
    printf("像素格式: R<%d %d> G<%d %d> B<%d %d>\n",
        scr_dev->fb_var.red.offset, scr_dev->fb_var.red.length,
        scr_dev->fb_var.green.offset, scr_dev->fb_var.green.length,
        scr_dev->fb_var.blue.offset, scr_dev->fb_var.blue.length);
    return 0;
}

int rgb_lcd_clear_screen(screen_info_t *scr_dev)
{
    memset(scr_dev->screen_base, 0, scr_dev->screen_size);
    return 0;
}

int rgb_lcd_show_point(screen_info_t *scr_dev, uint32_t x, uint32_t y, uint32_t color)
{
    uint32_t x_index;
    unsigned long temp;
    if (x >= scr_dev->width || y >= scr_dev->height) {
        printf("error: x or y too long\n");
        return -1;
    }

    x_index = x * (scr_dev->bits_pixel >> 3);
    temp = y * scr_dev->line_length + x_index;
    *(uint32_t*)&scr_dev->screen_base[temp] = color;

    return 0;
}

/********************************************************************
 * 函数名称： lcd_draw_line
 * 功能描述： 画线（水平或垂直线）
 * 输入参数： x, y, dir, length, color
 * 返 回 值： 无
 ********************************************************************/
int rgb_lcd_show_line(screen_info_t *scr_dev, uint32_t x, uint32_t y, uint32_t dir, uint32_t length, uint32_t color)
{
    unsigned int end;
    unsigned long temp;
    uint32_t x_index;

    // rgb_lcd_show_point(scr_dev, x, y, color);
    /* 对传入参数的校验 */
    if (x >= scr_dev->width)
        x = scr_dev->width - 1;
    if (y >= scr_dev->height)
        y = scr_dev->height - 1;

    /* 填充颜色 */
    if (dir) {  //水平线
        end = x + length - 1;
        if (end >= scr_dev->width)
            end = scr_dev->width - 1;

        temp = y * scr_dev->line_length;
        for ( ; x <= end; x++) {
            x_index = x * (scr_dev->bits_pixel >> 3);
            *(uint32_t*)&scr_dev->screen_base[temp + x_index] = color;
        }
    }
    else {  //垂直线
        end = y + length - 1;
        if (end >= scr_dev->height)
            end = scr_dev->height - 1;

        x_index = x * (scr_dev->bits_pixel >> 3);
        for ( ; y <= end; y++) {
            temp = y * scr_dev->line_length;
            *(uint32_t*)&scr_dev->screen_base[temp + x_index] = color;
        }
    }
    return 0;
}

int rgb_lcd_show_rectangle(screen_info_t *scr_dev, uint32_t start_x, uint32_t start_y, uint32_t end_x, uint32_t end_y, uint32_t color)
{
    int x_len = end_x - start_x + 1;
    int y_len = end_y - start_y - 1;

    rgb_lcd_show_line(scr_dev, start_x, start_y, 1, x_len, color);//上边
    rgb_lcd_show_line(scr_dev, start_x, end_y, 1, x_len, color); //下边
    rgb_lcd_show_line(scr_dev, start_x, start_y + 1, 0, y_len, color);//左边
    rgb_lcd_show_line(scr_dev, end_x, start_y + 1, 0, y_len, color);//右边
    return 0;
}

/********************************************************************
 * 函数名称： lcd_fill
 * 功能描述： 将一个矩形区域填充为参数color所指定的颜色
 * 输入参数： start_x, end_x, start_y, end_y, color
 * 返 回 值： 无
 ********************************************************************/
int rgb_lcd_show_fillrectangle(screen_info_t *scr_dev, uint32_t start_x, uint32_t start_y, uint32_t end_x, uint32_t end_y, uint32_t color)
{
    unsigned long temp;
    unsigned int x;
    uint32_t x_index;

    /* 对传入参数的校验 */
    if (end_x >= scr_dev->width)
        end_x = scr_dev->width - 1;
    if (end_y >= scr_dev->height)
        end_y = scr_dev->height - 1;

    /* 填充颜色 */
    temp = start_y * scr_dev->line_length; //定位到起点行首
    for ( ; start_y <= end_y; start_y++, temp += scr_dev->line_length) {
        for (x = start_x; x <= end_x; x++) {
            x_index = x * (scr_dev->bits_pixel >> 3);
            *(uint32_t*)&scr_dev->screen_base[temp + x_index] = color;
        }
    }
    return 0;
}

int show_image2lcd_gray(screen_info_t *scr_dev, uint32_t x, uint32_t y, int width, int height, uint8_t *buf)
{

    uint32_t end_x;
    uint32_t end_y;
    uint32_t start_x;
    uint32_t start_y;
    uint32_t x_index;
    uint32_t y_index;
    uint32_t temp;

    end_x = x + width;
    end_y = y + height;

    if (end_x > scr_dev->width)
        end_x = scr_dev->width;
    if (end_y > scr_dev->height)
        end_y = scr_dev->height;

    for (start_y = y; start_y < end_y; start_y++) {
        y_index = start_y * scr_dev->line_length;
        for (start_x = x; start_x < end_x; start_x++) {
            x_index = start_x * (scr_dev->bits_pixel >> 3);
// YUYV
#if 1
            temp = buf[((start_y - y) * width + start_x) * 2];
#endif
            *(uint32_t*)&scr_dev->screen_base[y_index + x_index] = temp | temp << 8 | temp << 16;
        }
    }

    return 0;
}

int show_image2lcd_rgb565(screen_info_t *scr_dev, uint32_t x, uint32_t y, int width, int height, uint8_t *buf)
{

    uint32_t end_x;
    uint32_t end_y;
    uint32_t start_x;
    uint32_t start_y;
    uint32_t x_index;
    uint32_t y_index;
    uint16_t temp;
    uint8_t r,g,b;

    end_x = x + width;
    end_y = y + height;

    if (end_x > scr_dev->width)
        end_x = scr_dev->width;
    if (end_y > scr_dev->height)
        end_y = scr_dev->height;

    for (start_y = y; start_y < end_y; start_y++) {
        y_index = start_y * scr_dev->line_length;
        for (start_x = x; start_x < end_x; start_x++) {
            x_index = start_x * (scr_dev->bits_pixel >> 3);
            temp = *(uint16_t*)&buf[((start_y - y) * width + start_x) * 2];
            b = (temp & 0x1f) << 3;
            g = ((temp >> 5) & 0x3f) << 2;
            r = (temp >> 11) << 3;
            *(uint32_t*)&scr_dev->screen_base[y_index + x_index] = b | g << 8 | r << 16;
        }
    }

    return 0;
}

int show_image2lcd_rgb888(screen_info_t *scr_dev, uint32_t x, uint32_t y, int width, int height, uint8_t *buf)
{
    uint32_t end_x;
    uint32_t end_y;
    uint32_t start_x;
    uint32_t start_y;
    uint32_t x_index;
    uint32_t y_index;
    uint8_t *temp;
    uint8_t r,g,b;

    end_x = x + width;
    end_y = y + height;

    if (end_x > scr_dev->width)
        end_x = scr_dev->width;
    if (end_y > scr_dev->height)
        end_y = scr_dev->height;

    for (start_y = y; start_y < end_y; start_y++) {
        y_index = start_y * scr_dev->line_length;
        for (start_x = x; start_x < end_x; start_x++) {
            x_index = start_x * (scr_dev->bits_pixel >> 3);
            temp = &buf[((start_y - y) * width + start_x) * 3];
            b = temp[0];
            g = temp[1];
            r = temp[2];
            *(uint32_t*)&scr_dev->screen_base[y_index + x_index] = b | g << 8 | r << 16;
        }
    }
    return 0;
}

int rgb_lcd_init(const char *dev_node, screen_info_t *scr_dev)
{
    int ret = -1;
    if (0 > (scr_dev->dev_fd = open(dev_node, O_RDWR))) {
        printf("open %s error\n", dev_node);
        return ret;
    }
    do {
        // 获取设备信息
        ret = ioctl(scr_dev->dev_fd, FBIOGET_VSCREENINFO, &scr_dev->fb_var);
        if (ret) {
            perror("FBIOGET_VSCREENINFO fail");
            break;
        }

        ret = ioctl(scr_dev->dev_fd, FBIOGET_FSCREENINFO, &scr_dev->fb_fix);
        if (ret) {
            perror("FBIOGET_FSCREENINFO fail");
            break;
        }

        display_device_info(scr_dev);
        scr_dev->bits_pixel = scr_dev->fb_var.bits_per_pixel;
        scr_dev->screen_size = scr_dev->fb_fix.smem_len;
        scr_dev->line_length = scr_dev->fb_fix.line_length;
        scr_dev->width = scr_dev->fb_var.xres;
        scr_dev->height = scr_dev->fb_var.yres;

        if (scr_dev->screen_base == NULL) {
            scr_dev->screen_base = (unsigned char *)mmap(NULL, scr_dev->screen_size, PROT_READ | PROT_WRITE, MAP_SHARED, scr_dev->dev_fd, 0);
            if (MAP_FAILED == (void *)scr_dev->screen_base) {
                perror("mmap error");
                break;
            }
        }

        ret = 0;
    } while (0);

    if (ret) {
        if (scr_dev->screen_base) {
            munmap(scr_dev->screen_base, scr_dev->screen_size);
            if (MAP_FAILED == (void *)scr_dev->screen_base) {
                perror("munmap error");
            }
        }
        close(scr_dev->dev_fd);
        scr_dev->dev_fd = -1;
    }

    return ret;
}

int rgb_lcd_deinit(screen_info_t *scr_dev)
{
    if (scr_dev->screen_base) {
        munmap(scr_dev->screen_base, scr_dev->screen_size);
        if (MAP_FAILED == (void *)scr_dev->screen_base) {
            perror("munmap error");
        }
    }
    if (scr_dev->dev_fd > 0) {
        close(scr_dev->dev_fd);
        scr_dev->dev_fd = -1;
    }
    return 0;
}
