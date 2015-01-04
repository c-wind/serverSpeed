#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>

#include "main.h"
#include "message_list.h"

#define MAX_EVENTS 1024

int tmp = 0;

int tcp_conn_init(session_t *s, char *ip, int timeout, int times, int tag, int timer, int type, int pack_len)
{
    memset(s, 0, sizeof(*s));

    s->type = type;
    s->ip = ip;
    s->pack_len = pack_len;
    s->tag = tag;
    s->port = DEFAULT_PORT;
    sprintf(s->addr_str, "%s:%d", ip, s->port);
    s->addr.sin_addr.s_addr = inet_addr(s->ip);
    s->addr.sin_port = htons(s->port);
    s->timeout = timeout;
    s->end_time = time(NULL) + timer;
    s->addr.sin_family = AF_INET;
    s->max_times = timer * times;
    s->interval = 1000 / times;

    return 0;
}



int tcp_conn_connect(session_t *s, int epfd, int idx)
{
    struct epoll_event ev;
    int nfd = socket(AF_INET, SOCK_STREAM, 0);
    if (nfd == -1)
    {
        log_error("%d socket error:%s", s->tag, strerror(errno));
        push_detail(s->id, s->type, s->tag, -1, 0);
        return -1;
    }

    if (fcntl(nfd, F_SETFL, O_NONBLOCK) == -1) {
        close(nfd);
        log_error("set nonblock error:%s", strerror(errno));
        push_detail(s->id, s->type, s->tag, -1, 0);
        return -1;
    }

    if (connect(nfd, (const struct sockaddr *)&(s->addr), sizeof(s->addr)) == -1)
    {
        if(errno != EINPROGRESS)
        {
            log_error("%d connect %s error:%s\n", s->tag, s->addr_str, strerror(errno));
            close(nfd);
            push_detail(s->id, s->type, s->tag, -2, 0);
            return -1;
        }
    }

    s->time_list[idx].fd = nfd;
    log_info("idx:%d connect", idx);
    gettimeofday(&s->time_list[idx].b, NULL);

    ev.events = EPOLLIN|EPOLLOUT;
    ev.data.fd = idx;

    if(epoll_ctl(epfd, EPOLL_CTL_ADD, nfd, &ev)==-1)
    {
        log_error("%d calloc struct epoll_event error:%s", s->tag, strerror(errno));
        close(nfd);
        return -1;
    }

    return 0;
}


int tcp_conn_wait(session_t *s, int epfd, int sec, int usec)
{
    struct timeval b, e;
    struct epoll_event events[MAX_EVENTS];
    int j = 0;

    gettimeofday(&b, NULL);
    e = b;
    b.tv_sec += sec;
    b.tv_usec += usec;

    while((e.tv_sec * 100000) + e.tv_usec < (b.tv_sec * 100000 ) + b.tv_usec)
    {
        int wait_time = (b.tv_sec - e.tv_sec) * 1000 +  (b.tv_usec - e.tv_usec) / 1000;
        log_info("wait:%d", wait_time);

        if (wait_time < 1)
            break;

        int nfds = epoll_wait(epfd, events, MAX_EVENTS, wait_time);
        if (nfds == 0)
            break;

        for (j=0; j<nfds; j++)
        {
            int idx = events[j].data.fd;
            int err, len = sizeof(err);
            if(epoll_ctl(epfd, EPOLL_CTL_DEL, s->time_list[idx].fd, NULL) == -1)
            {
                log_error("%d epoll_ctl del error:%s", s->tag, strerror(errno));
                s->time_list[idx].stat = 1;
                close(s->time_list[idx].fd);
                gettimeofday(&b, NULL);
                continue;
            }

            getsockopt(s->time_list[idx].fd, SOL_SOCKET, SO_ERROR, &err, (socklen_t *)&len);
            close(s->time_list[idx].fd);
            s->time_list[idx].fd = -1;
            if (err == 0)
            {
                gettimeofday(&s->time_list[idx].e, NULL);
                s->time_list[idx].stat = 1;
                log_info("%d idx:%d ok", s->tag, idx);
            }
            else
            {
                push_detail(s->id, s->type, s->tag, -1, 0);
                log_info("%d idx:%d error:%s", s->tag, idx, strerror(errno));
            }
        }
        gettimeofday(&b, NULL);
    }


    int k = 0;
    gettimeofday(&e, NULL);
    for (k = s->recv_idx; k< s->max_times; k++)
    {
        if (!s->time_list[k].b.tv_sec)
            break;

        if (s->time_list[k].fd != -1)
        {
            close(s->time_list[k].fd);
            s->time_list[k].fd = -1;
        }

        if (s->time_list[k].stat == 1)
        {
            int d = INTERVAL(s->time_list[k].e, s->time_list[k].b);
            push_detail(s->id, s->type, s->tag, d, 0);
            log_info("%d idx:%d delay:%d", s->tag, k, d);
            s->recv_idx++;
            continue;
        }

        if (s->time_list[k].b.tv_sec < (e.tv_sec - s->timeout))
        {
            log_info("%d idx:%d timeout, b sec:%d, e sec:%d", s->tag, k, s->time_list[k].b.tv_sec, e.tv_sec);
            push_detail(s->id, s->type, s->tag, -1, 0);
            s->recv_idx++;
            continue;
        }
        break;
    }

    return 0;
}


int tcp_conn_check(session_t *s)
{
    int i,k;
    int epoll_fd = epoll_create(MAX_EVENTS);

    for (i=0; i<s->max_times; i++)
    {
        tcp_conn_connect(s, epoll_fd, i);
        tcp_conn_wait(s, epoll_fd, 0, s->interval * 1000);
        usleep(s->interval * 1000);
    }

    tcp_conn_wait(s, epoll_fd, s->timeout, 0);

    int drop = 0, sum = 0, recv_num = 0, avg = 0;

    for (k = 0; k<s->max_times; k++)
    {
        if (s->time_list[k].stat)
        {
            sum += INTERVAL(s->time_list[k].e, s->time_list[k].b);
            recv_num++;
        }else {
            log_info("idx:%d drop", k);
        }

    }

    if (recv_num == 0)
    {
        avg = -1;
    }
    else
    {
        avg = sum / recv_num;
    }

    drop = ((s->max_times - recv_num) * 100) / s->max_times;

    log_info("%d drop:%d, avg:%d", s->tag, drop, avg);

    push_result(s->id, ACTION_RESULT, s->type, s->tag, avg, s->max_times, recv_num, drop, s->pack_len);

    push_action(s->id, ACTION_STOP, s->type, s->tag);


    return 0;
}

void *tcp_conn_start(void *dat)
{
    session_t *s = (session_t *)dat;

    pthread_detach(pthread_self());

    tcp_conn_check(s);

    return NULL;
}


