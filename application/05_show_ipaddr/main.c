#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <time.h>
#include <assert.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/prctl.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#include <errno.h>
#include <pthread.h>
#include <sys/poll.h>

#include <linux/fb.h>
#include <sys/mman.h>

typedef struct interface_info
{
    char ifname[32];
    char ifhwmac[32];
    char ifipaddr[16];
} interface_info_t;

interface_info_t ifinfo[5];

static bool quit = false;

static void sigterm_handler(int sig) {
	fprintf(stderr, "signal %d\n", sig);
	quit = true;
}

void show_hex(uint8_t *buf, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        printf("%x ", buf[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n");
}

int get_interface_info(interface_info_t *ifinfo)
{
    int ret = -1;
    int sockfd = -1;
    struct ifconf ifc;
    struct ifreq *ifr;
    char buffer[2048];
    int num_ifs = 0;
    int i = 0;

    ifc.ifc_len = sizeof(buffer);
    ifc.ifc_buf = buffer;

    do {
        // 创建套接字
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            perror("socket");
            break;
        }

        if (ioctl(sockfd, SIOCGIFCONF, &ifc) < 0) {
            perror("ioctl");
            break;
        }

        ifr = ifc.ifc_req;

        // 遍历接口
        for (i = 0; i < ifc.ifc_len / sizeof(struct ifreq); i++) {
            // 检查接口状态
            if (ioctl(sockfd, SIOCGIFFLAGS, ifr) == 0) {
                if (ifr->ifr_flags & IFF_UP) { // 接口状态为"up"

                    // printf("name:%s\n", ifr->ifr_name);

                    // 获取并打印接口名称
                    if (strcmp(ifr->ifr_name, "lo") == 0) {
                        ifr++;
                        continue;
                    }

                    strcpy(ifinfo[num_ifs].ifname, ifr->ifr_name);
                    struct sockaddr_in *sin = (struct sockaddr_in *)&ifr->ifr_addr;
                    if (ioctl(sockfd, SIOCGIFADDR, ifr) == 0) {
                        // printf("addr:%s\n", inet_ntoa(sin->sin_addr));
                        // 获取IP地址
                        strcpy(ifinfo[num_ifs].ifipaddr, inet_ntoa(sin->sin_addr));
                    }

                    if (ioctl(sockfd, SIOCGIFHWADDR, ifr) == 0) {
                    unsigned char *mac = (unsigned char *)&ifr->ifr_hwaddr.sa_data;
                    snprintf(ifinfo[num_ifs].ifhwmac, sizeof(ifinfo[num_ifs].ifhwmac), "%02x:%02x:%02x:%02x:%02x:%02x\n",
                        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
                    }

                    num_ifs++;
                }
            }
            // 移动到下一个接口
            ifr++;
        }

        ret = 0;
    } while (0);

    if (sockfd != -1) {
        // 关闭套接字
        close(sockfd);
    }

    return ret ? ret : num_ifs;
}

// echo -e '\033c' > /dev/tty0
// echo -e "\033[1;1H1234567" > /dev/tty0

void clear_screen(void)
{
    system("echo -e '\033c' > /dev/tty0");
}

void show_string(uint8_t row, uint8_t col, char* str)
{
    char buf[256];
    snprintf(buf, sizeof(buf), "echo -e \"\033[%d;%dH%s\" > /dev/tty0", row + 1, col + 1, str);
    system(buf);
}

int main(int argc, char *argv[])
{
    int ret = -1;
    int i = 0;
    int ifnum = 0;
    char buf[256];

    signal(SIGINT, sigterm_handler);

    do {

        while (1)
        {
            // 获取接口列表
            ifnum = get_interface_info(ifinfo);
            if (ifnum < 0) break;

            if (ifnum == 0) {
                show_string(0, 0, "No Interface");
            } else {
                clear_screen();
            }

            for (i = 0; i < ifnum; i++) {
                // printf("%d name:%s\n", i, ifinfo[i].ifname);
                // printf("%d ipaddr:%s\n", i, ifinfo[i].ifipaddr);

                snprintf(buf, sizeof(buf), "name:%s", ifinfo[i].ifname);
                show_string(i * 3, 0, buf);
                snprintf(buf, sizeof(buf), "hwmac:%s", ifinfo[i].ifhwmac);
                show_string(i * 3 + 1, 0, buf);
                snprintf(buf, sizeof(buf), "ipaddr:%s", ifinfo[i].ifipaddr);
                show_string(i * 3 + 2, 0, buf);
            }
            if (ifnum > 0) break;
            sleep(4);

            if (quit) {
                break;
            }
        }

        ret = 0;
    } while (0);

    return ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
