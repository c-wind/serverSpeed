#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "main.h"
#include "message_list.h"

int tcp_echo_init(session_t *s, char *ip, int timeout, int times, int tag, int timer, int type, int pack_len)
{
    memset(s, 0, sizeof(*s));

    s->fd = socket(AF_INET, SOCK_STREAM, 0);
    s->type = type;
    s->ip = ip;
    s->pack_len = pack_len;
    s->port = DEFAULT_PORT;
    sprintf(s->addr_str, "%s:%d", ip, s->port);
    s->addr.sin_family = AF_INET;
    s->addr.sin_addr.s_addr = inet_addr(s->ip);
    s->addr.sin_port = htons(s->port);
    s->tag = tag;
    s->timeout = timeout;
    s->end_time = time(NULL) + timer;

    s->max_times = timer * times;
    s->interval = 1000000 / times;

    struct timeval tv = {timeout, 0};

    setsockopt(s->fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv));
    if(connect(s->fd, (const struct sockaddr *)&s->addr, sizeof(s->addr)) == -1)
    {
        log_error("connect %s error:%s", s->addr_str, strerror(errno));
        close(s->fd);
        s->fd = -1;
        return -1;
    }

    log_info("%d server:%s, fd:%d, max_times:%d, interval:%d usec", s->tag, s->addr_str, s->fd, s->max_times, s->interval);

    return 0;
}

int tcp_reconnect(const struct sockaddr *addr, char *addr_str)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct timeval tv = {3, 0};

    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv));
    if(connect(fd, addr, sizeof(*addr)) == -1)
    {
        log_error("connect %s error:%s", addr_str, strerror(errno));
        close(fd);
        return -1;
    }

    return fd;
}


int tcp_echo_send(session_t *s, int i)
{
    int len = 0, n = 0;
    char buf[2048] = {0};
    int size_idx = i % 128;

    int size = s->pack_size[size_idx] - 28;

    *(int *)buf = i;
    *(int *)(buf + 4) = size;

    gettimeofday(&s->time_list[i].b, NULL);

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

    log_debug("%x send idx:%d size:%d len:%d, buf:%d %dok", s->tag, i, size, len, *(int *)buf , *(int *)(buf + 4) );

    return 0;
}



int tcp_echo_recv(session_t *s, int *size, int *idx)
{
    int len = 0, n = 0, fd = s->fd, head = 8;
    char buf[2048];

    if (fd == -1)
    {
        log_error("%d lose connect", s->tag);
        return -1;
    }


    while(len < head)
    {
        struct timeval tv = {s->timeout, 0};
        setsockopt(s->fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv));
        n = read(fd, buf + len, head - len);
        if (n == -1)
        {
            if (errno == 2)
                continue;
            close(fd);
            log_error("read fd:%d error:%s", fd, strerror(errno));
            return -1;
        }
        len += n;
    }
    *idx = *(int *)buf;
    *size = *(int *)(buf + 4) - 8;
    if(*size > sizeof(buf))
    {
        close(fd);
        log_error("%d recv size:%d error", s->tag, *size);
        return -1;
    }


    len = 0;

    while(len < *size)
    {
        n = read(fd, buf + len, *size - len);
        if (n == -1)
        {
            log_error("read fd:%d error:%s", fd, strerror(errno));
            return -1;
        }
        len += n;
    }

    log_error("*idx:%d, size:%d, len:%d", *idx, *size, len);

    if (*idx < 0 || *idx > s->max_times)
    {
        return 0;
    }

    gettimeofday(&s->time_list[*idx].e, NULL);

    s->time_list[*idx].stat = 1;

    log_debug("%x recv idx:%d ok", s->tag, *idx);

    return 0;
}


int session_over(session_t *s)
{
    if (s->over)
        return 1;

    return 0;
}

void *tcp_echo_recv_pthread(void *dat)
{
    session_t *s = (session_t *)dat;
    int i=0, sum = 0, avg = 0, delay, drop;

    pthread_detach(pthread_self());

    for(i=0; i<s->max_times && time(NULL) < s->end_time + 6; i++)
    {
        int idx, size;

        if (tcp_echo_recv(s, &size, &idx) == -1)
        {
            log_error("%d tcp_echo_recv error", s->tag);
            push_detail(s->id, s->type, s->tag, -1, 0);

            while(s->fd == -1 && !session_over(s))
            {
                sleep(1);
            }
            continue;
        }


        delay = INTERVAL(s->time_list[idx].e, s->time_list[idx].b);
        log_error("%d delay:%d", s->tag, delay);
        if (delay < 1)
        {
            log_error("%d b sec:%d, usec:%d, e sec:%d, usec:%d",
                      s->tag, (int)s->time_list[idx].b.tv_sec, (int)s->time_list[idx].b.tv_usec, (int)s->time_list[idx].e.tv_sec, (int)s->time_list[idx].e.tv_usec);
            push_detail(s->id, s->type, s->tag, -1, size + 28);
            continue;
        }
        sum += delay;
        s->recv_idx++;
        push_detail(s->id, s->type, s->tag, delay, size + 28);
        log_info("%d idx:%d delay:%d", s->tag, idx, delay);
    }

    log_info("%d over, i:%d, max:%d, time:%d, end:%d", s->tag, i, s->max_times, (int)time(NULL), (int)s->end_time);

    if (!s->recv_idx)
    {
        avg = -1;
        drop = 100;
    }
    else
    {
        avg = sum / s->recv_idx;
        drop = (s->max_times - s->recv_idx) * 100 / s->max_times;
    }

    push_result(s->id, ACTION_RESULT, s->type, s->tag, avg, s->max_times, s->recv_idx, drop, s->pack_len);

    push_action(s->id, ACTION_STOP, s->type, s->tag);

    close(s->fd);

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

    for(i=0; i<s->max_times && time(NULL) < s->end_time; i++)
    {
        gettimeofday(&b, NULL);

        if (tcp_echo_send(s, i) == -1)
        {
            log_error("%d tcp_echo_send error", s->tag);
            close(s->fd);
            s->fd = -1;

            int nfd = tcp_reconnect((const struct sockaddr *)&s->addr, s->addr_str) ;
            if (nfd == -1)
            {
                log_error("%d reconnect error, send idx:%d", s->tag, i);
                sleep(1);
                continue;
            }
            s->fd = nfd;
            log_info("%d reconnect succ, fd:%d", s->tag, s->fd);
            continue;
        }

        gettimeofday(&e, NULL);

        tv = INTERVAL(e, b);

        if (tv < s->interval)
        {
            log_info("%d sleep:%d", s->tag, s->interval - tv);
            usleep(s->interval - tv);
        }
    }

    log_error("i:%d, max:%d, time:%d, end:%d", i, s->max_times, time(NULL), s->end_time);

    s->max_times = i;
    s->over = 1;
    log_info("%d send over size:%d", s->tag, s->max_times);

    return NULL;
}

