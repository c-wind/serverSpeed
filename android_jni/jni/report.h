#ifndef __REPORT_H__
#define __REPORT_H__


#define MAX_FILE_PATH 1024
#define MAX_LINE 1024

int report_init(char *path);

int report_open(int type, char *addr1, char *addr2);

void report_detail(int tm, int id, int type, int tag, int delay, int size);

void report_result(int tm, int id, int type, int tag, int delay, int send, int recv, int drop, int pack_len);

int report_close();


#endif
