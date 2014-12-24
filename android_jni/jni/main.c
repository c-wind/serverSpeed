#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

#include "main.h"
#include "message_list.h"
#include "report.h"

#define CHECK_WAIT_TIMEOUT 1

void calljava_checkDetail(int id, int tm, int type, int tag, int result, int size);
void calljava_checkStop();
void calljava_checkResult(int tm, int id, int type, int tag, int avg, int send, int recv, int drop);

int __g_log_level = MRT_DEBUG;

int stat;
session_t s1;
session_t s2;
pthread_t pid1;
pthread_t pid2;
int default_size[128];


void default_size_random()
{
    int i =0;
    struct timeval tv;

    for (i=0; i<128; i++)
    {
        gettimeofday(&tv, NULL);
        default_size[i] = (tv.tv_usec  * i % 1400) + 64;
        log_info("size[%d]:%d", i, default_size[i]);
    }
}



int pthread_start(char *addr1, char *addr2, int bin_type, int pack_len, int times, int timeout, int timer)
{
    int (*init)(session_t *, char *, int, int, int, int, int) = NULL;
    void *(*start)(void *) = NULL;
    int type = bin_type & 3;

    default_size_random();

    switch (type)
    {
    case CHECK_TYPE_UDP_ECHO:
        init = udp_echo_init;
        start = udp_echo_start;
        break;

    case CHECK_TYPE_TCP_CONN:
        init = tcp_conn_init;
        start = tcp_conn_start;
        break;

    case CHECK_TYPE_TCP_ECHO:
        init = tcp_echo_init;
        start = tcp_echo_start;
        break;

    default:
        log_error("invalid type:%d", type);
        return -1;
    }

    if (report_open(bin_type, addr1, addr2) == -1)
    {
        log_error("report file open error");
        return -1;
    }


    if (addr1  && *addr1)
    {
        init(&s1, addr1, timeout, times, 1, timer, bin_type);
        memcpy(s1.pack_size, default_size, sizeof(default_size));
        if (pthread_create(&pid1, NULL, start, &s1))
        {
            log_error("pthread_create error:%s", strerror(errno));
            return -1;
        }
        stat |= 1;
    }

    if (addr2  && *addr2)
    {
        init(&s2, addr2, timeout, times, 2, timer, bin_type);
        memcpy(s2.pack_size, default_size, sizeof(default_size));
        if (pthread_create(&pid2, NULL, start, &s2))
        {
            log_error("pthread_create error:%s", strerror(errno));
            return -1;
        }
        stat |= 2;
    }


    return 0;
}


#ifdef BUILD_JNI

JNIEnv *__env;
jclass __class;

char *jbyteArray2char(jbyteArray ba)
{
    int len =  (*__env)->GetArrayLength(__env, ba);
    if(len < 1)
    {
        log_error("invalid byteArray len:%d", len);
        return NULL;
    }

    char *src = (char *)(*__env)->GetByteArrayElements(__env, ba, JNI_FALSE);
    if(!src)
    {
        log_error("GetByteArrayElements error, byteArray len:%d", len);
        return NULL;
    }

    char *buf = calloc(sizeof(char) * len + 1, 1);
    if(!buf)
    {
        log_error("alloc str error, len:%d", len);
        (*__env)->ReleaseByteArrayElements(__env, ba, (jbyte *)src, 0);
        return NULL;
    }

    memcpy(buf, src, len);
    buf[len] = 0;

    (*__env)->ReleaseByteArrayElements(__env, ba, (jbyte *)src, 0);

    return buf;
}


void JNI_FUNC(DumpMessage)(JNIEnv *e, jclass jc)
{
    __env = e;
    __class = jc;
    jni_message_t jm;

    while(!pop_message(&jm))
    {
        switch (jm.what)
        {
        case ACTION_DETAIL:
            report_detail(jm.tm, jm.id, jm.type, jm.tag, jm.value, jm.arg1);
            calljava_checkDetail(jm.id, jm.tm, jm.type, jm.tag, jm.value, jm.arg1);
            break;
        case ACTION_RESULT:
            report_result(jm.tm, jm.id, jm.type, jm.tag, jm.value, jm.arg1, jm.arg2, jm.arg3);
            calljava_checkResult(jm.tm, jm.id, jm.type, jm.tag, jm.value, jm.arg1, jm.arg2, jm.arg3);
            break;
        case ACTION_STOP:
            stat &= ~jm.tag;
            log_info("stat:%d, tag:%d, exit", stat, jm.tag);
            if (stat == 0)
            {
                calljava_checkStop();
                report_close();
            }
            break;
        }
    }
}

int JNI_FUNC(checkInit)(JNIEnv *e, jclass jc, jbyteArray path_array)
{
    __env = e;
    __class = jc;

    message_list_init();

    char *path = jbyteArray2char(path_array);
    if (!path || !*path)
    {
        log_error("path is null");
        return -1;
    }

    if (report_init(path) == -1)
    {
        log_error("report init error");
        return -1;
    }


    return 0;
}

int JNI_FUNC(checkStop)(JNIEnv *e, jclass jc)
{


    return 0;
}

int JNI_FUNC(checkStart)(JNIEnv *e, jclass jc, jbyteArray addr_array1, jbyteArray addr_array2, int type, int pack_len, int times, int timeout, int max_time)
{
    __env = e;
    __class = jc;
    char *addr1 = jbyteArray2char(addr_array1);
    char *addr2 = jbyteArray2char(addr_array2);

    if (pthread_start(addr1, addr2, type, pack_len, times, timeout, max_time) == -1)
    {
        log_error("pthread_start error");
        calljava_checkStop();
        return -1;
    }
    //TODO：要删除
    log_info("addr1:%s, addr2:%s, type:%d, packLen:%d, times:%d, timeout:%d, timer:%d", addr1, addr2, type, pack_len, times, timeout, max_time);

    return 0;
}

#endif

void calljava_checkStop()
{
#ifdef BUILD_JNI
    jmethodID method;

    method = (*__env)->GetStaticMethodID(__env, __class, "checkStop", "()V");
    if (method == NULL)
    {
        log_error("checkStop is NULL!");
        return;
    }

    (*__env)->CallStaticIntMethod(__env, __class, method);
    if((*__env)->ExceptionCheck(__env))
    {
        (*__env)->ExceptionClear(__env);//清除异常
        log_error("java checkStop Exception");
    }
#endif
}

void calljava_checkResult(int tm, int id, int type, int tag, int avg, int send, int recv, int drop)
{
#ifdef BUILD_JNI
    jmethodID method;

    method = (*__env)->GetStaticMethodID(__env, __class, "checkResult", "(IIIIIII)V");
    if (method == NULL)
    {
        log_error("checkResult is NULL!");
        return;
    }

    (*__env)->CallStaticIntMethod(__env, __class, method, tm, type, tag, avg, send, recv, drop);
    if((*__env)->ExceptionCheck(__env))
    {
        (*__env)->ExceptionClear(__env);//清除异常
        log_error("java checkResult Exception");
    }
#endif
}



void calljava_checkDetail(int id, int tm, int type, int tag, int result, int size)
{
#ifdef BUILD_JNI
    jmethodID method;

    method = (*__env)->GetStaticMethodID(__env, __class, "checkDetail", "(IIIIII)V");
    if (method == NULL)
    {
        log_error("checkDetail is NULL!");
        return;
    }

    (*__env)->CallStaticIntMethod(__env, __class, method, id, tm, type, tag, result, size);
    if((*__env)->ExceptionCheck(__env))
    {
        (*__env)->ExceptionClear(__env);//清除异常
        log_error("java checkDetail Exception");
    }
#endif
}


#ifndef BUILD_JNI
void dump_message()
{
    jni_message_t jm;

    while(!pop_message(&jm))
    {
        switch (jm.what)
        {
        case ACTION_DETAIL:
            printf("detail id:%d type:%d tag:%d delay:%d\n", jm.id, jm.type, jm.tag, jm.value);
            break;
        case ACTION_RESULT:
            printf("result %d, %d, %d, drop:%d, delay:%d\n", jm.tm, jm.type, jm.tag, jm.value, jm.arg1);
            break;
        case ACTION_STOP:
            printf("stop\n");
            break;
        }
    }
}


int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        printf("%s addrA addrB pack_len times timeout\n", argv[0]);
        return -1;
    }
    message_list_init();

    char *addrA = argv[1];
    char *addrB = argv[2];
    int pack_len = atoi(argv[3]);
    int times = atoi(argv[4]);
    int timeout = atoi(argv[5]);

    if (pthread_start(addrA, addrB, CHECK_TYPE_UDP_ECHO, pack_len, times, timeout, 10) == -1)
    {
        printf("pthread_start error\n");
        return -1;
    }

    printf("wait\n");
    while(1)
    {
        usleep(10);
        dump_message();
    }

    return 0;
}

#endif


