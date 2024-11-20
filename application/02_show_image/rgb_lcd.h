#ifndef _RGB_LCD_H_
#define _RGB_LCD_H_

int rgb_lcd_init(const char *dev_node, screen_info_t *scr_dev);
int rgb_lcd_deinit(screen_info_t *scr_dev);
int rgb_lcd_clear_screen(screen_info_t *scr_dev);
int rgb_lcd_show_point(screen_info_t *scr_dev, uint32_t x, uint32_t y, uint32_t color);
int rgb_lcd_show_line(screen_info_t *scr_dev, uint32_t x, uint32_t y, uint32_t dir, uint32_t length, uint32_t color);
int rgb_lcd_show_fillrectangle(screen_info_t *scr_dev, uint32_t start_x, uint32_t start_y, uint32_t end_x, uint32_t end_y, uint32_t color);
int show_image2lcd_gray(screen_info_t *scr_dev, uint32_t x, uint32_t y, int width, int height, uint8_t *buf);
int show_image2lcd_rgb565(screen_info_t *scr_dev, uint32_t x, uint32_t y, int width, int height, uint8_t *buf);
int show_image2lcd_rgb888(screen_info_t *scr_dev, uint32_t x, uint32_t y, int width, int height, uint8_t *buf);

#endif
