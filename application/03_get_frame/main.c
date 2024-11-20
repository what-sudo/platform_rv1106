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

#include <errno.h>
#include <pthread.h>
#include <sys/poll.h>

#include "sample_comm.h"

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

/******************************************************************************
 * function : vi thread
 ******************************************************************************/
static void *vi_get_stream(void *pArgs) {
	SAMPLE_VI_CTX_S *ctx = (SAMPLE_VI_CTX_S *)(pArgs);
	RK_S32 s32Ret = RK_FAILURE;
	char name[256] = {0};
	FILE *fp = RK_NULL;
	void *pData = RK_NULL;
	RK_S32 loopCount = 0;
	RK_S32 loss_number = 0;

	if (ctx->dstFilePath) {
		snprintf(name, sizeof(name), "/%s/frame_%d.bin", ctx->dstFilePath, ctx->s32DevId);
		fp = fopen(name, "wb");
		if (fp == RK_NULL) {
			printf("chn %d can't open %s file !\n", ctx->s32DevId, ctx->dstFilePath);
			quit = true;
			return RK_NULL;
		}
	}

	while (!quit) {
		s32Ret = SAMPLE_COMM_VI_GetChnFrame(ctx, &pData);
		if (s32Ret == RK_SUCCESS) {
			if (ctx->stViFrame.stVFrame.u64PrivateData <= 0) {
				// SAMPLE_COMM_VI_ReleaseChnFrame(ctx);
				// continue;
			}

			printf(
			    "SAMPLE_COMM_VI_GetChnFrame DevId %d ok:data %p size:%llu loop:%d seq:%d "
			    "pts:%lld ms\n",
			    ctx->s32DevId, pData, ctx->stViFrame.stVFrame.u64PrivateData, loopCount,
			    ctx->stViFrame.stVFrame.u32TimeRef,
			    ctx->stViFrame.stVFrame.u64PTS / 1000);

			loopCount++;

			if (loss_number <= 10)
				loss_number++;
			else {
				if (fp) {
					fwrite(pData, 1, ctx->stViFrame.stVFrame.u64PrivateData, fp);
					fflush(fp);
				}

				// exit when complete
				if (ctx->s32loopCount > 0) {
					if ((loopCount - loss_number) >= ctx->s32loopCount) {
						SAMPLE_COMM_VI_ReleaseChnFrame(ctx);
						quit = true;
						break;
					}
				}
			}

			SAMPLE_COMM_VI_ReleaseChnFrame(ctx);
		}
		usleep(1000);
	}

	if (fp)
		fclose(fp);

	return RK_NULL;
}


int main(int argc, char *argv[])
{
	RK_S32 s32Ret = RK_FAILURE;
	pthread_t vi_thread_id;

	char *iq_file_dir = "/etc/iqfiles";

	RK_S32 s32CamId = 0;
	RK_S32 s32ChnId = 0;

	int video_width = 800;
	int video_height = 480;

	PIXEL_FORMAT_E PixelFormat = RK_FMT_YUV420SP;
	COMPRESS_MODE_E CompressMode = COMPRESS_MODE_NONE;

	RK_S32 s32loopCnt = 5;
	RK_CHAR *pDeviceName = NULL;
	RK_CHAR *pOutPath = "/userdata";

	SAMPLE_VI_CTX_S ctx_vi;

#ifdef RKAIQ
	rk_aiq_working_mode_t hdr_mode = RK_AIQ_WORKING_MODE_NORMAL;
	RK_BOOL bMultictx = RK_FALSE;
#endif

	memset(&ctx_vi, 0, sizeof(SAMPLE_VI_CTX_S));

	signal(SIGINT, sigterm_handler);

	printf("#CameraIdx: %d\n", s32CamId);
	printf("#pDeviceName: %s\n", pDeviceName);
	printf("#Output Path: %s\n", pOutPath);
	printf("#IQ Path: %s\n", iq_file_dir);
	if (iq_file_dir) {
#ifdef RKAIQ
		printf("#Rkaiq XML DirPath: %s\n", iq_file_dir);
		printf("#bMultictx: %d\n\n", bMultictx);

		SAMPLE_COMM_ISP_Init(s32CamId, hdr_mode, bMultictx, iq_file_dir);
		SAMPLE_COMM_ISP_Run(s32CamId);
#endif
	}

    do {

        if (RK_MPI_SYS_Init() != RK_SUCCESS) {
            break;
        }

        // Init VI
        ctx_vi.u32Width = video_width;
        ctx_vi.u32Height = video_height;
        ctx_vi.s32DevId = s32CamId;
        ctx_vi.u32PipeId = ctx_vi.s32DevId;
        ctx_vi.s32ChnId = s32ChnId;
        ctx_vi.stChnAttr.stIspOpt.u32BufCount = 2;
        ctx_vi.stChnAttr.stIspOpt.enMemoryType = VI_V4L2_MEMORY_TYPE_DMABUF;
        ctx_vi.stChnAttr.u32Depth = 1;
        ctx_vi.stChnAttr.enPixelFormat = PixelFormat;
        ctx_vi.stChnAttr.enCompressMode = CompressMode;
        ctx_vi.stChnAttr.stFrameRate.s32SrcFrameRate = -1;
        ctx_vi.stChnAttr.stFrameRate.s32DstFrameRate = -1;
        ctx_vi.dstFilePath = pOutPath;
        ctx_vi.s32loopCount = s32loopCnt;
        if (pDeviceName) {
            strcpy(ctx_vi.stChnAttr.stIspOpt.aEntityName, pDeviceName);
        }

        SAMPLE_COMM_VI_CreateChn(&ctx_vi);

        pthread_create(&vi_thread_id, 0, vi_get_stream, (void *)(&ctx_vi));

        printf("%s initial finish\n", __func__);

        while (!quit) {
            sleep(1);
        }

        printf("%s exit!\n", __func__);

        pthread_join(vi_thread_id, NULL);

        // Destroy VI
        SAMPLE_COMM_VI_DestroyChn(&ctx_vi);

        s32Ret = 0;
    } while (0);

    if (s32Ret) {
        printf("warning: s32Ret:%d\n", s32Ret);
    }

	RK_MPI_SYS_Exit();

	if (iq_file_dir) {
#ifdef RKAIQ
		SAMPLE_COMM_ISP_Stop(s32CamId);
#endif
	}

    return s32Ret;
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
