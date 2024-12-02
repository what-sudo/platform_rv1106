#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#ifndef _RV1106_GPIO_H_
#define _RV1106_GPIO_H_

#define RK_GPIO0	0
#define RK_GPIO1	1
#define RK_GPIO2	2
#define RK_GPIO3	3
#define RK_GPIO4	4
#define RK_GPIO6	6

#define RK_PA0		0
#define RK_PA1		1
#define RK_PA2		2
#define RK_PA3		3
#define RK_PA4		4
#define RK_PA5		5
#define RK_PA6		6
#define RK_PA7		7
#define RK_PB0		8
#define RK_PB1		9
#define RK_PB2		10
#define RK_PB3		11
#define RK_PB4		12
#define RK_PB5		13
#define RK_PB6		14
#define RK_PB7		15
#define RK_PC0		16
#define RK_PC1		17
#define RK_PC2		18
#define RK_PC3		19
#define RK_PC4		20
#define RK_PC5		21
#define RK_PC6		22
#define RK_PC7		23
#define RK_PD0		24
#define RK_PD1		25
#define RK_PD2		26
#define RK_PD3		27
#define RK_PD4		28
#define RK_PD5		29
#define RK_PD6		30
#define RK_PD7		31

#define GPIO(bank, pin)	((((bank) * 32) + (pin)))

// GPIO 操作函数声明
int export_gpio(int gpio_number);
int set_gpio_direction(int gpio_number, const char *direction);
int read_gpio_value(int gpio_number);

#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
