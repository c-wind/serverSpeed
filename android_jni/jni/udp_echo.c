#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "main.h"
#include "message_list.h"


int id_idx = 0;

int udp_echo_init(session_t *s, char *ip, int timeout, int pack_size, int times, int tag, int timer)
{
    memset(s, 0, sizeof(*s));

    s->id = id_idx++;

    s->fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (s->fd == -1) {
        log_error("socket error:%s\n", strerror(errno));
        return -1;
    }

    s->type = CHECK_TYPE_UDP_ECHO;
    s->ip = ip;
    s->port = DEFAULT_PORT;
    s->tag = tag;
    sprintf(s->addr_str, "%s:%d", ip, s->port);

    s->addr.sin_family = AF_INET;
    s->addr.sin_addr.s_addr = inet_addr(s->ip);
    s->addr.sin_port = htons(s->port);
    s->num = 0;
    s->timeout = timeout;
    s->max_times = timer * times;
    s->interval = 1000000 / times;
    if (pack_size > 1500)
    {
        pack_size = 1500;
    }
    s->pack_size = pack_size - 28;

    log_info("%d udp data size:%d, interval:%d, max_times:%d, timeout:%d", s->tag, s->pack_size, s->interval, s->max_times, s->timeout);

    return 0;
}


void *udp_echo_recv(void *dat)
{
    session_t *s = (session_t *)dat;
    struct timeval timeout = {s->timeout,0};
    char data[2048];
    socklen_t addr_len = sizeof(struct sockaddr_in);
    struct timeval e;
    int idx = 0, ret = 0;

    pthread_detach(pthread_self());

    while(s->recv_idx < s->max_times)
    {
        setsockopt(s->fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
        ret = recvfrom(s->fd, data, sizeof(data), 0, (struct sockaddr *)&s->addr, &addr_len);
        if (ret == -1)
        {
            if (errno == EINTR)
            {
                log_error("中断了，继续");
                continue;
            }

            if (errno != EAGAIN)
            {
                log_error("recv %s error:%d:%s", s->addr_str, errno, strerror(errno));
                return NULL;
            }
        }

        gettimeofday(&e, NULL);

        if (ret != -1)
        {
            idx = *(int *)data;
            if (idx < 0 || idx > s->max_times)
            {
                log_error("invalid idx:%d", idx);
                continue;
            }

            if (s->time_list[idx].stat != -1)
            {
                s->time_list[idx].e = e;
                s->recv_idx++;
                int d = INTERVAL(s->time_list[idx].e, s->time_list[idx].b);
                s->time_list[idx].stat = d;
                push_message(s->id, ACTION_DETAIL, s->type, s->tag, d, 0);
                log_debug("id:%d, type:%d, tag:%d, delay:%d", s->id, s->type, s->tag, d);
            }
        }

        while((s->send_idx > s->del_idx) && (s->time_list[s->del_idx].b.tv_sec < (e.tv_sec - s->timeout)))
        {
            if (s->time_list[s->del_idx].stat > 0)
            {
                s->del_idx++;
                continue;
            }

            push_message(s->id, ACTION_DETAIL, s->type, s->tag, -1, 0);
            s->time_list[s->del_idx].stat = -1;
            log_debug("idx:%d, type:%d, tag:%d, b:%d, e:%d, timeout", s->del_idx, s->type, s->tag, (int)s->time_list[s->del_idx].b.tv_sec, (int)e.tv_sec);
            s->del_idx++;
            s->recv_idx++;
        }
    }

    close(s->fd);

    int i, drop = 0, ok = 0, avg, sum = 0;

    for (i=0; i<s->max_times; i++)
    {
        if (s->time_list[i].stat < 1)
        {
            drop++;
        }
        else
        {
            ok++;
            sum += s->time_list[i].stat;
        }
    }

    if (ok == 0)
    {
        avg = -1;
        drop = 100;
        log_info("%d drop:%d, avg:%d",  s->tag, drop, avg);
    } else {
        avg = sum / ok;
        log_info("%d drop:%d, avg:%d",  s->tag, drop, avg);
        drop = (int)(drop * 100)/s->max_times;
    }

    push_message(s->id, ACTION_RESULT, s->type, s->tag, drop, avg);

    push_action(s->id, ACTION_STOP, s->type, s->tag);

    return NULL;
}



void *udp_echo_start(void *dat)
{
    session_t *s = (session_t *)dat;
    char buf[2048];
    socklen_t addr_len = sizeof(struct sockaddr_in);
    pthread_detach(pthread_self());

    pthread_create(&s->pid, NULL, udp_echo_recv, s);

    while(s->num < s->max_times)
    {
        *(int *)buf = s->num++;
        if(sendto(s->fd, &buf, s->pack_size, 0, (struct sockaddr *)&s->addr, addr_len) == -1)
        {
            log_error("udp send to:%s, fd:%d, size:%d, error:%s",
                      s->addr_str, s->fd, s->pack_size, strerror(errno));
            close(s->fd);
            return NULL;
        }

        gettimeofday(&s->time_list[s->num - 1].b, NULL);
        s->send_idx++;
        usleep(s->interval);
        log_info("send tag:%d, idx:%d\n", s->tag, s->num - 1);
    }

    return NULL;
}


