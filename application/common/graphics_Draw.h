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
} graphics_fmt_type_t;

typedef enum {
    GD_FONT_8x16,
    GD_FONT_16x32,
    GD_FONT_16x32B,
} graphics_font_size_t;

typedef struct {
    uint32_t a:8;
    uint32_t r:8;
    uint32_t g:8;
    uint32_t b:8;
} graphics_color_t;

#define graphics_Red                  ((graphics_color_t){255, 255, 0, 0})
#define graphics_Green                ((graphics_color_t){255, 0, 255, 0})
#define graphics_Blue                 ((graphics_color_t){255, 0, 0, 255})
#define graphics_Yellow               ((graphics_color_t){255, 255, 255, 0})
#define graphics_Cyan                 ((graphics_color_t){255, 0, 255, 255})
#define graphics_Magenta              ((graphics_color_t){255, 255, 0, 255})

#define graphics_White                ((graphics_color_t){255, 255, 255, 255})
#define graphics_Black                ((graphics_color_t){255, 0, 0, 0})

#define graphics_Clear                ((graphics_color_t){0, 0, 0, 0})

typedef struct
{
    int width;
    int height;
    graphics_fmt_type_t fmt;
    uint32_t line_length;    // 一行的长度（字节为单位）
    uint8_t *buf;
} graphics_image_t;

int graphics_full(graphics_image_t *img, graphics_color_t color);
int graphics_line(graphics_image_t *img, uint32_t x, uint32_t y, uint32_t dir, uint32_t length, graphics_color_t color);
int graphics_rectangle(graphics_image_t *img, uint32_t start_x, uint32_t start_y, uint32_t end_x, uint32_t end_y, graphics_color_t color);
int graphics_fillrectangle(graphics_image_t *img, uint32_t start_x, uint32_t start_y, uint32_t end_x, uint32_t end_y, graphics_color_t color);

int graphics_show_char(graphics_image_t *img, uint32_t start_x, uint32_t start_y, uint8_t *buf, graphics_font_size_t size, graphics_color_t color);

int graphics_show_string(graphics_image_t *img, uint32_t start_x, uint32_t start_y, char *str, graphics_font_size_t size, graphics_color_t color);
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
