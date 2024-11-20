#ifndef __SHOW_BMP_H__
#define __SHOW_BMP_H__

/**** BMP文件头数据结构 ****/
typedef struct {
    unsigned char type[2];      //文件类型
    unsigned int size;          //文件大小
    unsigned short reserved1;   //保留字段1
    unsigned short reserved2;       //保留字段2
    unsigned int offset;        //到位图数据的偏移量
} __attribute__ ((packed)) bmp_file_header;

/**** 位图信息头数据结构 ****/
typedef struct {
    unsigned int size;          //位图信息头大小
    int width;                  //图像宽度
    int height;                 //图像高度
    unsigned short planes;      //位面数
    unsigned short bpp;         //像素深度
    unsigned int compression;   //压缩方式
    unsigned int image_size;    //图像大小
    int x_pels_per_meter;       //像素/米
    int y_pels_per_meter;       //像素/米
    unsigned int clr_used;
    unsigned int clr_omportant;
} __attribute__ ((packed)) bmp_info_header;

/**** 静态全局变量 ****/


/********************************************************************
 * 函数名称： show_bmp_image
 * 功能描述： 在LCD上显示指定的BMP图片
 * 输入参数： 文件路径
 * 返 回 值： 成功返回0, 失败返回-1
 ********************************************************************/
int show_bmp_image(const char *path, screen_info_t scr_info);

#endif
