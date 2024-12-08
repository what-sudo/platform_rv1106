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

int graphics_full(graphics_image_t *img, uint32_t color)
{
    uint32_t i = 0;
    uint32_t len = img->height * img->width;

    switch (img->fmt) {
        case GD_FMT_BGRA5551: {
            color = color & 0xff000000 ? color | 0x01000000 : color;
            for (i = 0; i < len; i++) {
                *(uint16_t*)&img->buf[i * 2] = (color & 0x01000000 >> 9) | ((color & 0x00f80000) >> 9) | ((color & 0x0000f800) >> 6) | (color & 0x0000f800);
            }
        } break;
        case GD_FMT_BGR888: {
            for (i = 0; i < len; i++) {
                *(uint8_t*)&img->buf[i * 3 + 0] = color & 0x000000ff;
                *(uint8_t*)&img->buf[i * 3 + 1] = (color & 0x0000ff00) >> 8;
                *(uint8_t*)&img->buf[i * 3 + 2] = (color & 0x00ff0000) >> 16;
            }
        } break;
        case GD_FMT_RGB888: {
            for (i = 0; i < len; i++) {
                *(uint8_t*)&img->buf[i * 3 + 0] = (color & 0x00ff0000) >> 16;
                *(uint8_t*)&img->buf[i * 3 + 1] = (color & 0x0000ff00) >> 8;
                *(uint8_t*)&img->buf[i * 3 + 2] = color & 0x000000ff;
            }
        } break;
        case GD_FMT_BGRA8888: {
            for (i = 0; i < len; i++) {
                *(uint32_t*)&img->buf[i * 4] = color;
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
int graphics_line(graphics_image_t *img, uint32_t x, uint32_t y, uint32_t dir, uint32_t length, uint32_t color, int flip)
{
    int ret = -1;
    unsigned int end;
    uint32_t y_index;
    uint32_t x_index;

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
            color = color & 0xff000000 ? color | 0x01000000 : color;
            /* 填充颜色 */
            if (dir) {  //水平线
                end = x + length - 1;
                if (end >= img->width)
                    end = img->width - 1;
                for ( ; x <= end; x++) {
                    if (flip == 1) {
                        x_index = (img->width - x) * 2;
                        y_index = y * img->line_length;
                    } else if (flip == 2) {
                        x_index = x * 2;
                        y_index = (img->height - y) * img->line_length;
                    } else if (flip == 3) {
                        x_index = (img->width - x) * 2;
                        y_index = (img->height - y) * img->line_length;
                    } else {
                        x_index = x * 2;
                        y_index = y * img->line_length;
                    }
                    *(uint16_t*)&img->buf[y_index + x_index] = (color & 0x01000000 >> 9) | ((color & 0x00f80000) >> 9) | ((color & 0x0000f800) >> 6) | (color & 0x0000f800);
                }
            }
            else {  //垂直线
                end = y + length - 1;
                if (end >= img->height)
                    end = img->height - 1;
                for ( ; y <= end; y++) {
                    if (flip == 1) {
                        x_index = (img->width - x) * 2;
                        y_index = y * img->line_length;
                    } else if (flip == 2) {
                        x_index = x * 2;
                        y_index = (img->height - y) * img->line_length;
                    } else if (flip == 3) {
                        x_index = (img->width - x) * 2;
                        y_index = (img->height - y) * img->line_length;
                    } else {
                        x_index = x * 2;
                        y_index = y * img->line_length;
                    }
                    *(uint16_t*)&img->buf[y_index + x_index] = (color & 0x01000000 >> 9) | ((color & 0x00f80000) >> 9) | ((color & 0x0000f800) >> 6) | (color & 0x0000f800);
                }
            }
        } break;
        case GD_FMT_BGR888: {
            /* 填充颜色 */
            if (dir) {  //水平线
                end = x + length - 1;
                if (end >= img->width)
                    end = img->width - 1;
                for ( ; x <= end; x++) {
                    if (flip == 1) {
                        x_index = (img->width - x) * 3;
                        y_index = y * img->line_length;
                    } else if (flip == 2) {
                        x_index = x * 3;
                        y_index = (img->height - y) * img->line_length;
                    } else if (flip == 3) {
                        x_index = (img->width - x) * 3;
                        y_index = (img->height - y) * img->line_length;
                    } else {
                        x_index = x * 3;
                        y_index = y * img->line_length;
                    }

                    *(uint8_t*)&img->buf[y_index + x_index + 0] = color & 0x000000ff;
                    *(uint8_t*)&img->buf[y_index + x_index + 1] = (color & 0x0000ff00) >> 8;
                    *(uint8_t*)&img->buf[y_index + x_index + 2] = (color & 0x00ff0000) >> 16;
                }
            }
            else {  //垂直线
                end = y + length - 1;
                if (end >= img->height)
                    end = img->height - 1;
                for ( ; y <= end; y++) {
                    if (flip == 1) {
                        x_index = (img->width - x) * 3;
                        y_index = y * img->line_length;
                    } else if (flip == 2) {
                        x_index = x * 3;
                        y_index = (img->height - y) * img->line_length;
                    } else if (flip == 3) {
                        x_index = (img->width - x) * 3;
                        y_index = (img->height - y) * img->line_length;
                    } else {
                        x_index = x * 3;
                        y_index = y * img->line_length;
                    }

                    *(uint8_t*)&img->buf[y_index + x_index + 0] = color & 0x000000ff;
                    *(uint8_t*)&img->buf[y_index + x_index + 1] = (color & 0x0000ff00) >> 8;
                    *(uint8_t*)&img->buf[y_index + x_index + 2] = (color & 0x00ff0000) >> 16;
                }
            }
        } break;
        case GD_FMT_RGB888: {
            /* 填充颜色 */
            if (dir) {  //水平线
                end = x + length - 1;
                if (end >= img->width)
                    end = img->width - 1;
                for ( ; x <= end; x++) {
                    if (flip == 1) {
                        x_index = (img->width - x) * 3;
                        y_index = y * img->line_length;
                    } else if (flip == 2) {
                        x_index = x * 3;
                        y_index = (img->height - y) * img->line_length;
                    } else if (flip == 3) {
                        x_index = (img->width - x) * 3;
                        y_index = (img->height - y) * img->line_length;
                    } else {
                        x_index = x * 3;
                        y_index = y * img->line_length;
                    }

                    *(uint8_t*)&img->buf[y_index + x_index + 0] = (color & 0x00ff0000) >> 16;
                    *(uint8_t*)&img->buf[y_index + x_index + 1] = (color & 0x0000ff00) >> 8;
                    *(uint8_t*)&img->buf[y_index + x_index + 2] = color & 0x000000ff;
                }
            }
            else {  //垂直线
                end = y + length - 1;
                if (end >= img->height)
                    end = img->height - 1;
                for ( ; y <= end; y++) {
                    if (flip == 1) {
                        x_index = (img->width - x) * 3;
                        y_index = y * img->line_length;
                    } else if (flip == 2) {
                        x_index = x * 3;
                        y_index = (img->height - y) * img->line_length;
                    } else if (flip == 3) {
                        x_index = (img->width - x) * 3;
                        y_index = (img->height - y) * img->line_length;
                    } else {
                        x_index = x * 3;
                        y_index = y * img->line_length;
                    }
                    *(uint8_t*)&img->buf[y_index + x_index + 0] = (color & 0x00ff0000) >> 16;
                    *(uint8_t*)&img->buf[y_index + x_index + 1] = (color & 0x0000ff00) >> 8;
                    *(uint8_t*)&img->buf[y_index + x_index + 2] = color & 0x000000ff;
                }
            }
        } break;
        default:
            printf("[%s %d] error: unsupport this fmt : %d\n", __func__, __LINE__, img->fmt);
        break;
    }

    return 0;
}

int graphics_rectangle(graphics_image_t *img, uint32_t start_x, uint32_t start_y, uint32_t end_x, uint32_t end_y, uint32_t color, int flip)
{
    int x_len = end_x - start_x;
    int y_len = end_y - start_y;

    graphics_line(img, start_x, start_y, 1, x_len, color, flip); //上边
    graphics_line(img, start_x, end_y, 1, x_len, color, flip); //下边
    graphics_line(img, start_x, start_y + 1, 0, y_len - 2, color, flip); //左边
    graphics_line(img, end_x, start_y + 1, 0, y_len - 2, color, flip); //右边
    return 0;
}

/********************************************************************
 * 函数名称： graphics_fillrectangle
 * 功能描述： 将一个矩形区域填充为参数color所指定的颜色
 * 输入参数： start_x, end_x, start_y, end_y, color
 * 返 回 值： 无
 ********************************************************************/
int graphics_fillrectangle(graphics_image_t *img, uint32_t start_x, uint32_t start_y, uint32_t end_x, uint32_t end_y, uint32_t color, int flip)
{
    uint32_t x;
    uint32_t y_index;
    uint32_t x_index;
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
    switch (img->fmt) {
        case GD_FMT_BGRA5551: {
            color = color & 0xff000000 ? color | 0x01000000 : color;
            for ( ; start_y <= end_y; start_y++) {
                for (x = start_x; x <= end_x; x++) {
                    if (flip == 0) {
                        x_index = x * 2;
                        y_index = start_y * img->line_length; //定位到起点行首
                    } else if (flip == 1) {
                        x_index = (img->width - start_x - x) * 2;
                        y_index = start_y * img->line_length; //定位到起点行首
                    } else if (flip == 2) {
                        x_index = x * 2;
                        y_index = (img->height - start_y) * img->line_length; //定位到终点行首
                    } else {
                        x_index = (img->width - start_x - x) * 2;
                        y_index = (img->height - start_y) * img->line_length; //定位到终点行首
                    }
                    *(uint16_t*)&img->buf[y_index + x_index] = (color & 0x01000000 >> 9) | ((color & 0x00f80000) >> 9) | ((color & 0x0000f800) >> 6) | (color & 0x0000f800);
                }
            }
        } break;
        case GD_FMT_BGR888: {
            for ( ; start_y <= end_y; start_y++) {
                for (x = start_x; x <= end_x; x++) {
                    if (flip == 0) {
                        x_index = x * 3;
                        y_index = start_y * img->line_length; //定位到起点行首
                    } else if (flip == 1) {
                        x_index = (img->width - start_x - x) * 3;
                        y_index = start_y * img->line_length; //定位到起点行首
                    } else if (flip == 2) {
                        x_index = x * 3;
                        y_index = (img->height - start_y) * img->line_length; //定位到终点行首
                    } else {
                        x_index = (img->width - start_x - x) * 3;
                        y_index = (img->height - start_y) * img->line_length; //定位到终点行首
                    }
                    *(uint8_t*)&img->buf[y_index + x_index + 0] = color & 0x000000ff;
                    *(uint8_t*)&img->buf[y_index + x_index + 1] = (color & 0x0000ff00) >> 8;
                    *(uint8_t*)&img->buf[y_index + x_index + 2] = (color & 0x00ff0000) >> 16;

                }
            }
        } break;
        case GD_FMT_RGB888: {
            for ( ; start_y <= end_y; start_y++) {
                for (x = start_x; x <= end_x; x++) {
                    if (flip == 0) {
                        x_index = x * 3;
                        y_index = start_y * img->line_length; //定位到起点行首
                    } else if (flip == 1) {
                        x_index = (img->width - start_x - x) * 3;
                        y_index = start_y * img->line_length; //定位到起点行首
                    } else if (flip == 2) {
                        x_index = x * 3;
                        y_index = (img->height - start_y) * img->line_length; //定位到终点行首
                    } else {
                        x_index = (img->width - start_x - x) * 3;
                        y_index = (img->height - start_y) * img->line_length; //定位到终点行首
                    }
                    *(uint8_t*)&img->buf[y_index + x_index + 0] = (color & 0x00ff0000) >> 16;
                    *(uint8_t*)&img->buf[y_index + x_index + 1] = (color & 0x0000ff00) >> 8;
                    *(uint8_t*)&img->buf[y_index + x_index + 2] = color & 0x000000ff;
                }
            }
        } break;
        case GD_FMT_BGRA8888: {
            for ( ; start_y <= end_y; start_y++) {
                for (x = start_x; x <= end_x; x++) {
                    if (flip == 0) {
                        x_index = x * 4;
                        y_index = start_y * img->line_length; //定位到起点行首
                    } else if (flip == 1) {
                        x_index = (img->width - start_x - x) * 4;
                        y_index = start_y * img->line_length; //定位到起点行首
                    } else if (flip == 2) {
                        x_index = x * 4;
                        y_index = (img->height - start_y) * img->line_length; //定位到终点行首
                    } else {
                        x_index = (img->width - start_x - x) * 4;
                        y_index = (img->height - start_y) * img->line_length; //定位到终点行首
                    }

                    *(uint32_t*)&img->buf[y_index + x_index] = color;
                }
            }
        } break;
        default:
            printf("[%s %d] error: unsupport this fmt : %d\n", __func__, __LINE__, img->fmt);
        break;
    }
    return 0;
}

int graphics_show_char(graphics_image_t *img, uint32_t start_x, uint32_t start_y, uint8_t *buf, graphics_font_size_t size, uint32_t color, int flip)
{
    int ret = -1;
    int i,j;
    int w, h, end_x, end_y;
    uint32_t y_index = 0;
    uint32_t x_index = 0;
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

    switch (img->fmt) {
        case GD_FMT_BGRA5551: {
            color = color & 0xff000000 ? color | 0x01000000 : color;
            for ( ; start_y <= end_y; start_y++) {
                data = buf; buf += w / 8; j = 0;
                for (i = start_x; i <= end_x; i++) {
                    if (*(data + j / 8) & (0x80 >> j % 8)) {
                        if (flip == 0) {
                            x_index = i * 2;
                            y_index = start_y * img->line_length; //定位到起点行首
                        } else if (flip == 1) {
                            x_index = (img->width - i) * 2;
                            y_index = start_y * img->line_length; //定位到起点行首
                        } else if (flip == 2) {
                            x_index = i * 2;
                            y_index = (img->height - start_y) * img->line_length; //定位到起点行首
                        } else {
                            x_index = (img->width - i) * 2;
                            y_index = (img->height - start_y) * img->line_length; //定位到起点行首
                        }

                        *(uint16_t*)&img->buf[y_index + x_index] = (color & 0x01000000 >> 9) | ((color & 0x00f80000) >> 9) | ((color & 0x0000f800) >> 6) | (color & 0x0000f800);
                    }
                    j++;
                }
            }
        } break;
        case GD_FMT_BGR888: {
            for ( ; start_y <= end_y; start_y++) {
                data = buf; buf += w / 8; j = 0;
                for (i = start_x; i <= end_x; i++) {
                    if (*(data + j / 8) & (0x80 >> j % 8)) {
                        if (flip == 0) {
                            x_index = i * 3;
                            y_index = start_y * img->line_length; //定位到起点行首
                        } else if (flip == 1) {
                            x_index = (img->width - i) * 3;
                            y_index = start_y * img->line_length; //定位到起点行首
                        } else if (flip == 2) {
                            x_index = i * 3;
                            y_index = (img->height - start_y) * img->line_length; //定位到起点行首
                        } else {
                            x_index = (img->width - i) * 3;
                            y_index = (img->height - start_y) * img->line_length; //定位到起点行首
                        }

                        *(uint8_t*)&img->buf[y_index + x_index + 0] = color & 0x000000ff;
                        *(uint8_t*)&img->buf[y_index + x_index + 1] = (color & 0x0000ff00) >> 8;
                        *(uint8_t*)&img->buf[y_index + x_index + 2] = (color & 0x00ff0000) >> 16;
                    }
                    j++;
                }
            }
        } break;
        case GD_FMT_RGB888: {
            for ( ; start_y <= end_y; start_y++) {
                data = buf; buf += w / 8; j = 0;
                for (i = start_x; i <= end_x; i++) {
                    if (*(data + j / 8) & (0x80 >> j % 8)) {
                        if (flip == 0) {
                            x_index = i * 3;
                            y_index = start_y * img->line_length; //定位到起点行首
                        } else if (flip == 1) {
                            x_index = (img->width - i) * 3;
                            y_index = start_y * img->line_length; //定位到起点行首
                        } else if (flip == 2) {
                            x_index = i * 3;
                            y_index = (img->height - start_y) * img->line_length; //定位到起点行首
                        } else {
                            x_index = (img->width - i) * 3;
                            y_index = (img->height - start_y) * img->line_length; //定位到起点行首
                        }

                        *(uint8_t*)&img->buf[y_index + x_index + 0] = (color & 0x00ff0000) >> 16;
                        *(uint8_t*)&img->buf[y_index + x_index + 1] = (color & 0x0000ff00) >> 8;
                        *(uint8_t*)&img->buf[y_index + x_index + 2] = color & 0x000000ff;
                    }
                    j++;
                }
            }
        } break;
        case GD_FMT_BGRA8888: {
            for ( ; start_y <= end_y; start_y++) {
                data = buf; buf += w / 8; j = 0;
                for (i = start_x; i <= end_x; i++) {
                    if (*(data + j / 8) & (0x80 >> j % 8)) {
                        if (flip == 0) {
                            x_index = i * 4;
                            y_index = start_y * img->line_length; //定位到起点行首
                        } else if (flip == 1) {
                            x_index = (img->width - i) * 4;
                            y_index = start_y * img->line_length; //定位到起点行首
                        } else if (flip == 2) {
                            x_index = i * 4;
                            y_index = (img->height - start_y) * img->line_length; //定位到起点行首
                        } else {
                            x_index = (img->width - i) * 4;
                            y_index = (img->height - start_y) * img->line_length; //定位到起点行首
                        }

                        *(uint32_t*)&img->buf[y_index + x_index] = color;
                    }
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

int graphics_show_string(graphics_image_t *img, uint32_t start_x, uint32_t start_y, char *str, graphics_font_size_t size, uint32_t color, int flip)
{
    int i = 0;
    int len = strlen(str);
    int end_x = start_x;

    switch (size) {
        case GD_FONT_8x16: {
            for (i = 0; i < len; i++) {
                if (end_x + 8 > img->width) {
                    start_y += 16;
                    end_x = start_x;
                }
                if (graphics_show_char(img, end_x, start_y, (uint8_t*)ascii_8x16[str[i] - ' '], size, color, flip))
                    return -1;
                end_x += 8;
            }
        } break;
        case GD_FONT_16x32B:
        case GD_FONT_16x32: {
            for (i = 0; i < len; i++) {
                if (end_x + 16 > img->width) {
                    start_y += 32;
                    end_x = start_x;
                }
                if (graphics_show_char(img, end_x, start_y, (uint8_t*)ascii_16x32[str[i] - ' '], size, color, flip))
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
