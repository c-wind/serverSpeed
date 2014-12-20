#ifndef __MAIN_H__
#define __MAIN_H__

#include <sys/types.h>	       /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#ifdef BUILD_JNI
#include <android/log.h>
#include <jni.h>
#endif

// ------------ 日志级别 ------------
#define MRT_DEBUG 1
#define MRT_INFO 2
#define MRT_WARNING 3
#define MRT_ERROR 4
#define MRT_FATAL 5

// ---------- 字节换算 --------------
#define M_1MB   1048576
#define M_16MB  16777216
#define M_64KB  65536
#define M_128KB 131072
#define M_32KB  32768
#define M_16KB  16384
#define M_8KB   8192
#define M_4KB   4096
#define M_1KB   1024

#define LOG_TAG "SPEED"

#define DEFAULT_PORT 9003

extern int __g_log_level;

#define INTERVAL(e, b) (((e.tv_sec - b.tv_sec) * 1000000 + (e.tv_usec - b.tv_usec)) / 1000)
#define TIMER_START() \
    struct timeval __b, __e;\
    gettimeofday(&__b, NULL)

#define TIMER_STOP() gettimeofday(&__e, NULL)
#define TIMER_USED() INTERVAL(__e, __b)

#define TIME2BUF(b) \
    do { \
        time_t vt = time(NULL); \
        struct tm *vtm = localtime(&vt); \
        strftime(b, sizeof(b), "%T", vtm); \
    }while(0)



#ifdef BUILD_JNI
#define log_debug(fmt, ...) \
    do { \
        if(MRT_DEBUG >= __g_log_level) {\
            __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, "%s "fmt, __FUNCTION__, ##__VA_ARGS__); \
 } \
    }while(0)
#define log_info(fmt, ...) \
    do { \
        if(MRT_INFO >= __g_log_level) {\
            __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "%s "fmt, __FUNCTION__, ##__VA_ARGS__); \
        } \
    }while(0)

#define log_warning(fmt, ...) \
    do { \
        if(MRT_WARNING >= __g_log_level) {\
            __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "%s "fmt, __FUNCTION__, ##__VA_ARGS__); \
        } \
    }while(0)

#define log_error(fmt, ...) \
    do { \
        if(MRT_ERROR >= __g_log_level) {\
            __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "%s "fmt, __FUNCTION__, ##__VA_ARGS__);\
        } \
    }while(0)

#define log_fatal(fmt, ...) \
    do { \
        __android_log_print(ANDROID_LOG_FATAL, LOG_TAG, "%s "fmt, __FUNCTION__, ##__VA_ARGS__); \
    }while(0)

#else  //BUILD_JNI

#define log_debug(fmt, ...) \
    do { \
        if(MRT_DEBUG >= __g_log_level) {\
            char vtb[32]; \
            TIME2BUF(vtb); \
            printf("%s %s "fmt"\n", vtb, __FUNCTION__, ##__VA_ARGS__); \
        } \
    }while(0)

#define log_info(fmt, ...) \
    do { \
        if(MRT_INFO >= __g_log_level) {\
            char vtb[32]; \
            TIME2BUF(vtb); \
            printf("\033[32m%s %s "fmt"\n\033[0m", vtb, __FUNCTION__, ##__VA_ARGS__); \
        } \
    }while(0)

#define log_warning(fmt, ...) \
    do { \
        if(MRT_WARNING >= __g_log_level) {\
            char vtb[32]; \
            TIME2BUF(vtb); \
            printf("%s %s "fmt"\n", vtb, __FUNCTION__, ##__VA_ARGS__); \
        } \
    }while(0)

#define log_error(fmt, ...) \
    do { \
        if(MRT_ERROR >= __g_log_level) {\
            char vtb[32]; \
            TIME2BUF(vtb); \
            printf("\033[31m%s %s "fmt"\n\033[0m", vtb, __FUNCTION__, ##__VA_ARGS__);\
        } \
    }while(0)

#define log_fatal(fmt, ...) printf("%s "fmt"\n", __FUNCTION__, ##__VA_ARGS__)

#endif

#define CHECK_TYPE_UDP_ECHO 1
#define CHECK_TYPE_TCP_CONN 2
#define CHECK_TYPE_TCP_ECHO 3

#define CHECK_STAT_START 0
#define CHECK_STAT_STOP 1

#define CHECK_DEST_NORMAL 0
#define CHECK_DEST_QOS 1

typedef struct {
    int    stat;
    int    fd;
    struct timeval b;
    struct timeval e;
}used_time_t;


typedef struct {
    int     id;
    int     tag;
    int     type;
    int     fd;
    char    *ip;
    int     port;
    int     timeout;
    int     num;
    int     interval;
    int     max_times;
    int     recv_idx;
    int     pack_size;

    char    addr_str[64];
    struct sockaddr_in addr;

    int     send_idx;
    int     del_idx;
    used_time_t time_list[60000];
    pthread_t pid;

}session_t;

typedef struct {
    int     id;
    int     type;
    int     tag;
    int     stat;


    char    prefix[33];

    char    ip[33];
    int     port;
    int     timeout;
    int     times;

    int (*init)(session_t *, char *, int, int);
    int (*check)(session_t *);

    session_t session;

    pthread_t ptid;

}pthread_data_t;


int udp_echo_init(session_t *s, char *ip, int timeout, int pack_size, int times, int tag, int timer);
void *udp_echo_start(void *dat);

int tcp_conn_init(session_t *s, char *ip, int timeout, int pack_size, int times, int tag, int timer);
void *tcp_conn_start(void *dat);

int tcp_echo_init(session_t *s, char *ip, int timeout, int pack_size, int times, int tag, int timer);
void *tcp_echo_start(void *dat);

#define JNI_FUNC(f) Java_jni_VPNJni_##f


#endif
