#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "postprocess.h"

static char *g_labels[OBJ_CLASS_NUM];

static char *readLine(FILE *fp, char *buffer, int *len)
{
    int ch;
    int i = 0;
    size_t buff_len = 0;

    buffer = (char *)malloc(buff_len + 1);
    if (!buffer)
        return NULL; // Out of memory

    while ((ch = fgetc(fp)) != '\n' && ch != EOF)
    {
        buff_len++;
        void *tmp = realloc(buffer, buff_len + 1);
        if (tmp == NULL)
        {
            free(buffer);
            return NULL; // Out of memory
        }
        buffer = (char *)tmp;

        buffer[i] = (char)ch;
        i++;
    }
    buffer[i] = '\0';

    *len = buff_len;

    // Detect end
    if (ch == EOF && (i == 0 || ferror(fp)))
    {
        free(buffer);
        return NULL;
    }
    return buffer;
}

static int readLines(const char *fileName, char *lines[], int max_line)
{
    FILE *file = fopen(fileName, "r");
    char *s;
    int i = 0;
    int n = 0;

    if (file == NULL)
    {
        printf("Open %s fail!\n", fileName);
        return -1;
    }

    while ((s = readLine(file, s, &n)) != NULL)
    {
        lines[i++] = s;
        if (i >= max_line)
            break;
    }
    fclose(file);
    return i;
}

int init_post_process(const char *label_name_txt_path)
{
    int ret = 0;
    ret = readLines(label_name_txt_path, g_labels, OBJ_CLASS_NUM);
    if (ret < 0)
    {
        printf("Load %s failed!\n", label_name_txt_path);
        return -1;
    }
    return 0;
}

void deinit_post_process(void)
{
    for (int i = 0; i < OBJ_CLASS_NUM; i++)
    {
        if (g_labels[i] != nullptr)
        {
            free(g_labels[i]);
            g_labels[i] = nullptr;
        }
    }
}

