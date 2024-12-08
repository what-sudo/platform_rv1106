#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#ifndef _RV1106_GRAPHICS_DRAW_H_
#define _RV1106_GRAPHICS_DRAW_H_

typedef enum {
    GD_FMT_BGRA5551,
    GD_FMT_BGR888,
    GD_FMT_RGB888,
    GD_FMT_BGRA8888,
} graphics_fmt_type_t;

typedef enum {
    GD_FONT_8x16,
    GD_FONT_16x32,
    GD_FONT_16x32B,
} graphics_font_size_t;

#define graphics_Red                  0xffff0000
#define graphics_Green                0xff00ff00
#define graphics_Blue                 0xff0000ff

#define graphics_Yellow               0xffffff00
#define graphics_Cyan                 0xff00ffff
#define graphics_Magenta              0xffff00ff

#define graphics_White                0xffffffff
#define graphics_Black                0xff000000

#define graphics_Clear                0x00000000

typedef struct
{
    int width;
    int height;
    graphics_fmt_type_t fmt;
    uint32_t line_length;    // 一行的长度（字节为单位）
    uint8_t *buf;
} graphics_image_t;

int graphics_full(graphics_image_t *img, uint32_t color);
int graphics_line(graphics_image_t *img, uint32_t x, uint32_t y, uint32_t dir, uint32_t length, uint32_t color, int flip);
int graphics_rectangle(graphics_image_t *img, uint32_t start_x, uint32_t start_y, uint32_t end_x, uint32_t end_y, uint32_t color, int flip);
int graphics_fillrectangle(graphics_image_t *img, uint32_t start_x, uint32_t start_y, uint32_t end_x, uint32_t end_y, uint32_t color, int flip);

int graphics_show_char(graphics_image_t *img, uint32_t start_x, uint32_t start_y, uint8_t *buf, graphics_font_size_t size, uint32_t color, int flip);

int graphics_show_string(graphics_image_t *img, uint32_t start_x, uint32_t start_y, char *str, graphics_font_size_t size, uint32_t color, int flip);
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
