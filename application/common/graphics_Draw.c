#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "graphics_Draw.h"

extern const unsigned char ascii_8x16[][16];
extern const unsigned char ascii_16x32[][64];
extern const unsigned char ascii_16x32B[][64];

int graphics_full(graphics_image_t *img, graphics_color_t color)
{
    uint32_t i = 0;
    uint32_t len = img->height * img->width;

    switch (img->fmt) {
        case GD_FMT_BGRA5551: {
            color.a = color.a ? 1 : 0;
            for (i = 0; i < len; i++) {
                *(uint16_t*)&img->buf[i * 2] = (color.a << 15) | ((color.r & 0xf8) << 7) | ((color.g & 0xf8) << 2) | ((color.b & 0xf8) >> 3);
            }
        } break;
        default:
            printf("[%s %d] error: unsupport this fmt : %d\n", __func__, __LINE__, img->fmt);
        break;
    }

    return 0;
}

/********************************************************************
 * graphics_line
 * 功能描述： 画线（水平或垂直线）
 * 输入参数： x, y, dir, length, color
 * 返 回 值： 无
 ********************************************************************/
int graphics_line(graphics_image_t *img, uint32_t x, uint32_t y, uint32_t dir, uint32_t length, graphics_color_t color)
{
    int ret = -1;
    unsigned int end;
    unsigned long temp;

    if (img->buf == NULL) {
        printf("[%s %d] error: img is null\n", __func__, __LINE__);
        return ret;
    }

    /* 对传入参数的校验 */
    if (x >= img->width)
        x = img->width - 1;
    if (y >= img->height)
        y = img->height - 1;

    switch (img->fmt) {
        case GD_FMT_BGRA5551: {
            color.a = color.a ? 1 : 0;
            /* 填充颜色 */
            if (dir) {  //水平线
                end = x + length - 1;
                if (end >= img->width)
                    end = img->width - 1;
                temp = y * img->line_length;
                for ( ; x <= end; x++) {
                    *(uint16_t*)&img->buf[temp + x * 2] = (color.a << 15) | ((color.r & 0xf8) << 7) | ((color.g & 0xf8) << 2) | ((color.b & 0xf8) >> 3);
                }
            }
            else {  //垂直线
                end = y + length - 1;
                if (end >= img->height)
                    end = img->height - 1;
                for ( ; y <= end; y++) {
                    temp = y * img->line_length;
                    *(uint16_t*)&img->buf[temp + x * 2] = (color.a << 15) | ((color.r & 0xf8) << 7) | ((color.g & 0xf8) << 2) | ((color.b & 0xf8) >> 3);
                }
            }
        } break;
        default:
            printf("[%s %d] error: unsupport this fmt : %d\n", __func__, __LINE__, img->fmt);
        break;
    }

    return 0;
}

int graphics_rectangle(graphics_image_t *img, uint32_t start_x, uint32_t start_y, uint32_t end_x, uint32_t end_y, graphics_color_t color)
{
    int x_len = end_x - start_x;
    int y_len = end_y - start_y;

    graphics_line(img, start_x, start_y, 1, x_len, color);//上边
    graphics_line(img, start_x, end_y, 1, x_len, color); //下边
    graphics_line(img, start_x, start_y + 1, 0, y_len - 2, color);//左边
    graphics_line(img, end_x, start_y + 1, 0, y_len - 2, color);//右边
    return 0;
}

/********************************************************************
 * 函数名称： graphics_fillrectangle
 * 功能描述： 将一个矩形区域填充为参数color所指定的颜色
 * 输入参数： start_x, end_x, start_y, end_y, color
 * 返 回 值： 无
 ********************************************************************/
int graphics_fillrectangle(graphics_image_t *img, uint32_t start_x, uint32_t start_y, uint32_t end_x, uint32_t end_y, graphics_color_t color)
{
    uint32_t x;
    uint32_t temp;
    int ret = -1;

    if (img->buf == NULL) {
        printf("[%s %d] error: img is null\n", __func__, __LINE__);
        return ret;
    }

    /* 对传入参数的校验 */
    if (end_x >= img->width)
        end_x = img->width - 1;
    if (end_y >= img->height)
        end_y = img->height - 1;

    /* 填充颜色 */
    temp = start_y * img->line_length; //定位到起点行首
    switch (img->fmt) {
        case GD_FMT_BGRA5551: {
            color.a = color.a ? 1 : 0;
            for ( ; start_y <= end_y; start_y++, temp += img->line_length) {
                for (x = start_x; x <= end_x; x++) {
                    *(uint16_t*)&img->buf[temp + x * 2] = (color.a << 15) | ((color.r & 0xf8) << 7) | ((color.g & 0xf8) << 2) | ((color.b & 0xf8) >> 3);
                }
            }
        } break;
        default:
            printf("[%s %d] error: unsupport this fmt : %d\n", __func__, __LINE__, img->fmt);
        break;
    }
    return 0;
}

int graphics_show_char(graphics_image_t *img, uint32_t start_x, uint32_t start_y, uint8_t *buf, graphics_font_size_t size, graphics_color_t color)
{
    int ret = -1;
    int i,j;
    int w, h, end_x, end_y;
    uint32_t temp;
    uint8_t *data;

    if (img->buf == NULL) {
        printf("[%s %d] error: img is null\n", __func__, __LINE__);
        return ret;
    }

    switch (size) {
        case GD_FONT_8x16: {
            w = 8; h = 16;
        } break;
        case GD_FONT_16x32B:
        case GD_FONT_16x32: {
            w = 16; h = 32;
        } break;
        default:
            printf("[%s %d] error: unsupport size\n", __func__, __LINE__);
            return ret;
        break;
    }

    end_x = start_x + w;
    end_y = start_y + h;

    /* 对传入参数的校验 */
    if (end_x >= img->width || end_y >= img->height) {
        printf("[%s %d] warning: coordinate out of range\n", __func__, __LINE__);
        return ret;
    }

    temp = start_y * img->line_length; //定位到起点行首

    switch (img->fmt) {
        case GD_FMT_BGRA5551: {
            color.a = color.a ? 1 : 0;
            for ( ; start_y <= end_y; start_y++, temp += img->line_length) {
                data = buf; buf += w / 8; j = 0;
                for (i = start_x; i <= end_x; i++) {
                    *(uint16_t*)&img->buf[temp + i * 2] =
                      *(data + j / 8) & (0x80 >> j % 8) ? ((color.a << 15) | ((color.r & 0xf8) << 7) | ((color.g & 0xf8) << 2) | ((color.b & 0xf8) >> 3)) : 0;
                      j++;
                }
            }
        } break;
        default:
            printf("[%s %d] error: unsupport this fmt : %d\n", __func__, __LINE__, img->fmt);
        break;
    }

    ret = 0;
    return ret;
}

int graphics_show_string(graphics_image_t *img, uint32_t start_x, uint32_t start_y, char *str, graphics_font_size_t size, graphics_color_t color)
{
    int i = 0;
    int len = strlen(str);
    int end_x = start_x;

    switch (size) {
        case GD_FONT_8x16: {
            for (i = 0; i < len; i++) {
                if (graphics_show_char(img, end_x, start_y, (uint8_t*)ascii_8x16[str[i] - ' '], size, color))
                    return -1;
                end_x += 8;
            }
        } break;
        case GD_FONT_16x32B:
        case GD_FONT_16x32: {
            for (i = 0; i < len; i++) {
                if (graphics_show_char(img, end_x, start_y, (uint8_t*)ascii_16x32[str[i] - ' '], size, color))
                    return -1;
                end_x += 16;
            }
        } break;
        default:
            printf("[%s %d] error: unsupport size\n", __func__, __LINE__);
            return -1;
        break;
    }

    return 0;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
