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

int tcp_conn_init(session_t *s, char *ip, int timeout, int pack_size, int times, int tag, int timer)
{
    memset(s, 0, sizeof(*s));

    s->type = CHECK_TYPE_TCP_CONN;
    s->ip = ip;
    s->tag = tag;
    s->port = DEFAULT_PORT;
    sprintf(s->addr_str, "%s:%d", ip, s->port);
    s->addr.sin_addr.s_addr = inet_addr(s->ip);
    s->addr.sin_port = htons(s->port);
    s->timeout = timeout;
    s->addr.sin_family = AF_INET;
    s->max_times = timer * times;
    s->interval = 1000 / times;

    return 0;
}

int tcp_conn_check(session_t *s)
{
    int i,j,k, nfd = -1, max_fd = -1;
    struct timeval b, e;
    int epoll_fd = epoll_create(MAX_EVENTS);
    struct epoll_event ev;
    struct epoll_event events[MAX_EVENTS];

    for (i=0; i<s->max_times; i++)
    {
        nfd = socket(AF_INET, SOCK_STREAM, 0);
        if (nfd == -1)
        {
            log_error("socket error:%s", strerror(errno));
            return -1;
        }

        if (fcntl(nfd, F_SETFL, O_NONBLOCK) == -1) {
            close(nfd);
            log_error("set nonblock error:%s", strerror(errno));
            return -1;
        }

        if (connect(nfd, (const struct sockaddr *)&(s->addr), sizeof(s->addr)) == -1)
        {
            if(errno != EINPROGRESS)
            {
                printf("connect %s error:%s\n", s->addr_str, strerror(errno));
                close(nfd);
                return -1;
            }
        }

        s->time_list[i].fd = nfd;

        gettimeofday(&s->time_list[i].b, NULL);


        if (nfd > max_fd)
        {
            max_fd = nfd;
        }


        ev.events = EPOLLIN|EPOLLOUT;
        ev.data.fd = i;

        if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, nfd, &ev)==-1)
        {
            log_error("calloc struct epoll_event error:%s", strerror(errno));
            close(nfd);
            return -1;
        }


        gettimeofday(&b, NULL);
        e = b;
        b.tv_usec += s->interval * 1000;


        while(e.tv_usec < b.tv_usec)
        {
            int wait_time = (b.tv_usec - e.tv_usec) / 1000;

            if (wait_time == 0)
                break;

            int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, wait_time);
            if (nfds == 0)
            {
                break;
            }

            for (j=0; j<nfds; j++)
            {
                int idx = events[j].data.fd;
                int err, len = sizeof(err);
                getsockopt(s->time_list[idx].fd, SOL_SOCKET, SO_ERROR, &err, (socklen_t *)&len);
                if (err == 0)
                {
                    gettimeofday(&s->time_list[idx].e, NULL);
                    s->time_list[idx].stat = 1;
                    close(s->time_list[idx].fd);

                }
                else
                {
                    log_error("socket fd:%d error:%s", s->time_list[idx].fd, strerror(err));
                    close(s->time_list[idx].fd);
                }
            }
        }


        gettimeofday(&e, NULL);
        for (k = s->recv_idx; k<i; k++)
        {
            if (s->time_list[k].stat == 1)
            {
                int d = INTERVAL(s->time_list[k].e, s->time_list[k].b);
                push_message(s->id, ACTION_DETAIL, s->type, s->tag, d, 0);
                continue;
            }

            if (s->time_list[i].b.tv_sec < (e.tv_sec - s->timeout))
            {
                push_message(s->id, ACTION_DETAIL, s->type, s->tag, -1, 0);
                continue;
            }
            break;
        }
    }

    int drop = 0, sum = 0, ok = 0, avg = 0;

    for (k = 0; k<s->max_times; k++)
    {
        if (s->time_list[k].stat == 0)
        {
            drop++;

        }
        else
        {
            sum += INTERVAL(s->time_list[k].e, s->time_list[k].b);
            ok++;
        }
    }

    if (ok == 0)
    {
        avg = -1;
    }
    else
    {
        avg = sum / ok;
    }

    log_info("drop:%d, avg:%d", drop, avg);
    push_message(s->id, ACTION_RESULT, s->type, s->tag, drop, avg);

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


