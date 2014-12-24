#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/stat.h>

#include "main.h"
#include "message_list.h"
#include "report.h"


FILE *fp = NULL;
FILE *result_fp = NULL;
char file_name[MAX_FILE_PATH];
char sd_path[MAX_FILE_PATH];

int file_path_init(char *sd_path)
{
    char tmp_path[MAX_LINE];
    sprintf(tmp_path, "%s/server_speed", sd_path);
    if (mkdir(tmp_path, 0777) == -1)
    {
        log_error("mkdir path:%s error:%s", tmp_path, strerror(errno));
        return -1;
    }

    return 0;
}

int report_init(char *path)
{
    char file[MAX_FILE_PATH];

    file_path_init(path);

    sprintf(sd_path, "%s", path);

    sprintf(file, "%s/server_speed/result", path);

    result_fp = fopen(file, "a");
    if (!result_fp)
    {
        log_error("open %s error:%s", file, strerror(errno));
        return -1;
    }

    return 0;
}


int report_open(int bin_type, char *addr1, char *addr2)
{
    if (fp)
    {
        fclose(fp);
    }

    int tm = (int)time(NULL);
    sprintf(file_name, "%s/server_speed/%d.tmp", sd_path, tm);

    fp = fopen(file_name, "w");
    if (!fp)
    {
        log_error("open log file:%s error:%s", file_name, strerror(errno));
        return -1;
    }

    fprintf(fp, "%d %s %s %d\n", bin_type, addr1 && *addr1 ?addr1:"", addr2 && *addr2?addr2:"", tm);

    return 0;
}



void report_detail(int tm, int id, int type, int tag, int delay, int size)
{
    fprintf(fp, "D %d %d %d %d %d %d\n", tm, id, type, tag, delay, size);
}


void report_result(int tm, int id, int type, int tag, int delay, int send, int recv, int drop)
{
    fprintf(result_fp, "%d %d %d %d %d %d %d %d\n", tm, id, type, tag, delay, send, recv, drop);
    fflush(result_fp);
    fprintf(fp, "R %d %d %d %d %d %d %d %d\n", tm, id, type, tag, delay, send, recv, drop);
}

int report_close()
{
    char new_file[MAX_LINE];
    snprintf(new_file, strlen(file_name) - 3, "%s", file_name);

    fclose(fp);
    fp = NULL;

    if (rename(file_name, new_file) == -1)
    {
        log_error("link from:%s to:%s error:%s", file_name, new_file, strerror(errno));
        return -1;
    }

    log_info("new report file succ, path:%s", new_file);

    return 0;
}





