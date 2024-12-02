#ifndef _RGB_LCD_H_
#define _RGB_LCD_H_

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

typedef enum {
    LCD_FMT_YUV420SP, /* YYYY... UV...            */
    LCD_FMT_YUV422_YUYV,    /* YUYVYUYV...              */
} PIXEL_FORMAT_t;

int rgb_lcd_init(const char *dev_node, screen_info_t *scr_dev);
int rgb_lcd_deinit(screen_info_t *scr_dev);
int rgb_lcd_clear_screen(screen_info_t *scr_dev);
int rgb_lcd_show_point(screen_info_t *scr_dev, uint32_t x, uint32_t y, uint32_t color);
int rgb_lcd_show_line(screen_info_t *scr_dev, uint32_t x, uint32_t y, uint32_t dir, uint32_t length, uint32_t color);
int rgb_lcd_show_fillrectangle(screen_info_t *scr_dev, uint32_t start_x, uint32_t start_y, uint32_t end_x, uint32_t end_y, uint32_t color);

int rgb_lcd_show_yuv_gray(screen_info_t *scr_dev, uint32_t x, uint32_t y, int width, int height, uint8_t *buf, PIXEL_FORMAT_t format);
int rgb_lcd_show_rgb565(screen_info_t *scr_dev, uint32_t x, uint32_t y, int width, int height, uint8_t *buf);
int rgb_lcd_show_rgb888(screen_info_t *scr_dev, uint32_t x, uint32_t y, int width, int height, uint8_t *buf, int bigEndian, int flip);
int rgb_lcd_show_rgba8888(screen_info_t *scr_dev, uint32_t x, uint32_t y, int width, int height, uint8_t *buf);
int rgb_lcd_show_bgra8888(screen_info_t *scr_dev, uint32_t x, uint32_t y, int width, int height, uint8_t *buf);


#endif
