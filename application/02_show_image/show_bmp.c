#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "data_type.h"
#include "show_bmp.h"

/********************************************************************
 * 函数名称： show_bmp_image
 * 功能描述： 在LCD上显示指定的BMP图片
 * 输入参数： 文件路径
 * 返 回 值： 成功返回0, 失败返回-1
 ********************************************************************/
int show_bmp_image(const char *path, screen_info_t scr_info)
{
    int ret = -1;
    bmp_file_header file_h;
    bmp_info_header info_h;
    unsigned char *line_buf = NULL;    //行缓冲区
    unsigned char *line_buf_tmp = NULL;    //行缓冲区
    unsigned long line_bytes;   //BMP图像一行的字节的大小
    unsigned char *screen_base;   //映射后的显存基地址
    unsigned char *screen_base_tmp;   //映射后的显存基地址
    unsigned char screen_bpp_B;   //映射后的显存基地址
    unsigned char img_bpp_B;   //映射后的显存基地址

    unsigned int min_h, min_w;
    int fd = -1;
    int i,j;

    /* 打开文件 */
    if (0 > (fd = open(path, O_RDONLY))) {
        perror("open error");
        return -1;
    }

    do {
        /* 读取BMP文件头 */
        if (sizeof(bmp_file_header) !=
            read(fd, &file_h, sizeof(bmp_file_header))) {
            perror("read error");
            break;
        }

        if (0 != memcmp(file_h.type, "BM", 2)) {
            fprintf(stderr, "it's not a BMP file\n");
            break;
        }

        /* 读取位图信息头 */
        if (sizeof(bmp_info_header) !=
            read(fd, &info_h, sizeof(bmp_info_header))) {
            perror("read error");
            break;
        }

        /* 打印信息 */
        printf("\n%s\n文件大小: %d\n"
            "位图数据的偏移量: %d\n"
            "位图信息头大小: %d\n"
            "图像分辨率: %d*%d\n"
            "像素深度: %d\n", path, file_h.size, file_h.offset,
            info_h.size, info_h.width, info_h.height,
            info_h.bpp);

        screen_base = scr_info.screen_base;
        screen_bpp_B = scr_info.bits_pixel / 8;
        img_bpp_B = info_h.bpp / 8;

        /* 将文件读写位置移动到图像数据开始处 */
        if (-1 == lseek(fd, file_h.offset, SEEK_SET)) {
            perror("lseek error");
            break;
        }

        /* 申请一个buf、暂存bmp图像的一行数据 */
        line_bytes = info_h.width * img_bpp_B;

        if (scr_info.width > info_h.width)
            min_w = info_h.width;
        else
            min_w = scr_info.width;

        if (line_bytes % 4) {
            line_bytes = ((line_bytes / 4) + 1) * 4;
        }
        line_buf = malloc(line_bytes + 4);
        if (NULL == line_buf) {
            fprintf(stderr, "malloc error\n");
            break;
        }

        printf("%s\n", 0 < info_h.height ? "Inverted bitmap" : "Forward bitmap");
        printf("screen_bpp_B: %d\n", screen_bpp_B);
        printf("img_bpp_B: %d\n", img_bpp_B);
        printf("min width: %d\n", min_w);

        /**** 读取图像数据显示到LCD ****/
        /*******************************************
         * 为了软件处理上方便，这个示例代码便不去做兼容性设计了
         * 如果你想做兼容, 可能需要判断传入的BMP图像是565还是888
         * 如何判断呢？文档里边说的很清楚了
         * 我们默认传入的bmp图像是RGB565格式
         *******************************************/
        if (0 < info_h.height) {//倒向位图
            if (info_h.height > scr_info.height) {
                min_h = scr_info.height;
                lseek(fd, (info_h.height - scr_info.height) * line_bytes, SEEK_CUR);
            }
            else {
                min_h = info_h.height;
            }
            printf("min height: %d\n", min_h);

            screen_base += scr_info.line_length * (min_h - 1);    //定位屏幕上显示的图片左下角位置

            for (j = min_h; j > 0; screen_base -= scr_info.line_length, j--) {
                read(fd, line_buf, line_bytes); //读取出图像数据

                line_buf_tmp = line_buf;
                screen_base_tmp = screen_base;
                for (i = 0; i < min_w; i++) {
                    memcpy(screen_base_tmp, line_buf_tmp, img_bpp_B); //刷入LCD显存
                    screen_base_tmp += screen_bpp_B;
                    line_buf_tmp += img_bpp_B;
                }
            }
        }
        else {  //正向位图
            int temp = 0 - info_h.height;   //负数转成正数
            if (temp > scr_info.height)
                min_h = scr_info.height;
            else
                min_h = temp;
            printf("min height: %d\n", min_h);

            for (j = 0; j < min_h; j++, screen_base += scr_info.line_length) {
                read(fd, line_buf, line_bytes);

                line_buf_tmp = line_buf;
                screen_base_tmp = screen_base;
                for (i = 0; i < min_w; i++) {
                    memcpy(screen_base_tmp, line_buf_tmp, img_bpp_B); //刷入LCD显存
                    screen_base_tmp += screen_bpp_B;
                    line_buf_tmp += img_bpp_B;
                }
            }
        }

        ret = 0;
    } while (0);

    /* 关闭文件、函数返回 */
    close(fd);
    free(line_buf);
    return ret;
}
