#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "main.h"
#include "message_list.h"

int tcp_echo_init(session_t *s, char *ip, int timeout, int pack_size, int times, int tag, int timer)
{
    memset(s, 0, sizeof(*s));

    s->fd = socket(AF_INET, SOCK_STREAM, 0);
    s->type = CHECK_TYPE_TCP_ECHO;
    s->ip = ip;
    s->port = DEFAULT_PORT;
    sprintf(s->addr_str, "%s:%d", ip, s->port);
    s->addr.sin_family = AF_INET;
    s->addr.sin_addr.s_addr = inet_addr(s->ip);
    s->addr.sin_port = htons(s->port);
    s->num = 0;
    s->tag = tag;
    s->timeout = timeout;

    s->max_times = timer * times;
    s->interval = 1000000 / times;

    if (pack_size > 1500)
    {
        pack_size = 1500;
    }
    s->pack_size = pack_size - 40;

    struct timeval tv = {timeout, 0};

    setsockopt(s->fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv));
    if(connect(s->fd, (const struct sockaddr *)&s->addr, sizeof(s->addr)) == -1)
    {
        log_error("connect %s error:%s", s->addr_str, strerror(errno));
        close(s->fd);
        return -1;
    }

    log_info("%d server:%s, fd:%d, max_times:%d, interval:%d usec", s->tag, s->addr_str, s->fd, s->max_times, s->interval);

    return 0;
}


int tcp_echo_send(session_t *s)
{
    int idx = s->num++, len = 0, n = 0, size = s->pack_size;
    char buf[2048];

    *(int *)buf = idx;

    gettimeofday(&s->time_list[idx].b, NULL);

    while(len < size)
    {
        n = write(s->fd, buf + len, size - len);
        if (n == -1)
        {
            log_error("write fd:%d error:%s", s->fd, strerror(errno));
            return -1;
        }

        len += n;
    }

    log_debug("%x send idx:%d ok", s->tag, idx);

    return 0;
}


int tcp_echo_recv(session_t *s)
{
    int idx, len = 0, n = 0;
    char buf[2048];


    while(len < s->pack_size)
    {
        n = read(s->fd, buf + len, s->pack_size - len);
        if (n == -1)
        {
            log_error("read fd:%d error:%s", s->fd, strerror(errno));
            return -1;
        }

        len += n;
    }

    idx = *(int *)buf;

    gettimeofday(&s->time_list[idx].e, NULL);

    s->time_list[idx].stat = 1;

    log_debug("%x recv idx:%d ok", s->tag, idx);

    return 0;
}



void *tcp_echo_recv_pthread(void *dat)
{
    session_t *s = (session_t *)dat;
    int i=0, sum = 0, avg = 0, delay;

    pthread_detach(pthread_self());

    for(i=0; i<s->max_times; i++)
    {
        if (tcp_echo_recv(s) == -1)
        {
            log_error("%d tcp_echo_recv error", s->tag);
            push_message(s->id, ACTION_RESULT, s->type, s->tag, -1, -1);
            push_action(s->id, ACTION_STOP, s->type, s->tag);
            close(s->fd);
            return NULL;
        }

        delay = INTERVAL(s->time_list[i].e, s->time_list[i].b);
        sum += delay;
        push_message(s->id, ACTION_DETAIL, s->type, s->tag, delay, 0);
        log_info("%d idx:%d delay:%d", s->tag, i, delay);
    }

    avg = sum / s->max_times;

    push_message(s->id, ACTION_RESULT, s->type, s->tag, -1, avg);

    push_action(s->id, ACTION_STOP, s->type, s->tag);

    log_info("%d avg:%d", s->tag, avg);

    return NULL;
}


void *tcp_echo_start(void *dat)
{
    session_t *s = (session_t *)dat;
    int i = 0, tv = 0;
    struct timeval b, e;

    pthread_detach(pthread_self());

    pthread_create(&s->pid, NULL, tcp_echo_recv_pthread, dat);

    for(i=0; i<s->max_times; i++)
    {
        gettimeofday(&b, NULL);

        if (tcp_echo_send(s) == -1)
        {
            log_error("%d tcp_echo_send error", s->tag);
            close(s->fd);
            push_message(s->id, ACTION_RESULT, s->type, s->tag, -1, -1);
            push_action(s->id, ACTION_STOP, s->type, s->tag);
            return NULL;
        }

        gettimeofday(&e, NULL);

        tv = INTERVAL(e, b);
        log_info("tv:%d", tv);

        if (tv < s->interval)
        {
            log_info("sleep:%d", s->interval - tv);
            usleep(s->interval - tv);
        }

    }

    return NULL;
}
