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

int udp_echo_init(session_t *s, char *ip, int timeout, int times, int tag, int timer, int type, int pack_len)
{
    memset(s, 0, sizeof(*s));

    s->id = id_idx++;

    s->fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (s->fd == -1) {
        log_error("socket error:%s\n", strerror(errno));
        return -1;
    }

    s->pack_len = pack_len;
    s->type = type;
    s->ip = ip;
    s->port = DEFAULT_PORT;
    s->tag = tag;
    sprintf(s->addr_str, "%s:%d", ip, s->port);

    s->addr.sin_family = AF_INET;
    s->addr.sin_addr.s_addr = inet_addr(s->ip);
    s->addr.sin_port = htons(s->port);
    s->timeout = timeout;
    s->max_times = timer * times;
    s->interval = 1000000 / times;
    s->end_time = time(NULL) + timer;

    log_info("%d udp interval:%d, max_times:%d, timeout:%d", s->tag, s->interval, s->max_times, s->timeout);

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

    while(s->recv_idx < s->max_times && time(NULL) < s->end_time + 6)
    {
        if (s->fd == -1)
        {
            log_error("%d socket fd:%d", s->tag, s->fd);
            usleep(s->interval);
            continue;
        }

        setsockopt(s->fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
        ret = recvfrom(s->fd, data, sizeof(data), 0, (struct sockaddr *)&s->addr, &addr_len);
        if (ret == -1)
        {
            if (errno == EINTR || errno == EAGAIN)
            {
                log_error("中断了，继续");
                continue;
            }

            if(s->fd == -1)
            {
                log_error("wait socket, curr fd:%d", s->fd);
                continue;
            }

        }


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
                gettimeofday(&e, NULL);
                s->time_list[idx].e = e;
                s->recv_idx++;
                int d = INTERVAL(s->time_list[idx].e, s->time_list[idx].b);
                if (d < 0)
                {
                    char line[1024];
                    sprintf(line, "idx:%d, b.sec:%d usec:%d, e.sec:%d usec:%d\n",
                            idx, (int)s->time_list[idx].b.tv_sec, (int)s->time_list[idx].b.tv_usec, (int)s->time_list[idx].e.tv_sec, (int)s->time_list[idx].e.tv_usec);

                    log_write(line);
                }
                s->time_list[idx].stat = d;
                push_detail(s->id, s->type, s->tag, d, ret + 20);
                log_debug("id:%d, type:%d, tag:%d, delay:%d", s->id, s->type, s->tag, d);
            }
        }

        gettimeofday(&e, NULL);
        while((s->send_idx > s->del_idx) && (s->time_list[s->del_idx].b.tv_sec < (e.tv_sec - s->timeout)))
        {
            if (s->time_list[s->del_idx].stat > 0)
            {
                s->del_idx++;
                continue;
            }

            push_detail(s->id, s->type, s->tag, -1, -1);
            s->time_list[s->del_idx].stat = -1;
            log_debug("idx:%d, type:%d, tag:%d, b:%d, e:%d, timeout", s->del_idx, s->type, s->tag, (int)s->time_list[s->del_idx].b.tv_sec, (int)e.tv_sec);
            s->del_idx++;
            s->recv_idx++;
        }
    }

    close(s->fd);

    int i, recv_num = 0, drop = 0, avg, sum = 0;
    for (i=0; i<s->max_times; i++)
    {
        if (s->time_list[i].stat > 0)
        {
            recv_num++;
            sum += s->time_list[i].stat;
        }
    }

    if (recv_num == 0)
    {
        avg = -1;
        drop = 100;
        log_info("%d drop:%d, avg:%d",  s->tag, drop, avg);
    } else {
        avg = sum / recv_num;
        drop = (int)((s->max_times - recv_num) * 100) / s->max_times;
        log_info("%d send:%d, recv:%d, drop:%d%%, avg:%d",  s->tag, s->max_times, recv_num, drop, avg);
    }

    push_result(s->id, ACTION_RESULT, s->type, s->tag, avg, s->max_times, recv_num, drop, s->pack_len);

    push_action(s->id, ACTION_STOP, s->type, s->tag);

    close(s->fd);

    return NULL;
}



void *udp_echo_start(void *dat)
{
    session_t *s = (session_t *)dat;
    char buf[2048];
    int i = 0;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    pthread_detach(pthread_self());

    pthread_create(&s->pid, NULL, udp_echo_recv, s);


    for(i = 0; i < s->max_times && time(NULL) < s->end_time; i++)
    {
        *(int *)buf = i;
        int size_idx = i % 128;
        int pack_size =  s->pack_size[size_idx] - 20;
        gettimeofday(&s->time_list[i].b, NULL);

        if (s->fd == -1)
        {
            int nfd = socket(AF_INET, SOCK_DGRAM, 0);
            if (nfd == -1) {
                log_error("socket error:%s\n", strerror(errno));
                push_detail(s->id, s->type, s->tag, -1, pack_size + 20);
                usleep(s->interval);
                continue;
            }
            s->fd = nfd;
            log_info("new fd:%d", s->fd);
        }

        if(sendto(s->fd, &buf, pack_size, 0, (struct sockaddr *)&s->addr, addr_len) == -1)
        {
            if (errno == EINTR)
            {
                log_error("中断了，继续");
                continue;
            }

            log_error("%d idx:%d fd:%d sendto error:%s", s->tag, i, s->fd, strerror(errno));
            close(s->fd);
            s->fd = -1;
            usleep(s->interval);
            push_detail(s->id, s->type, s->tag, -1, pack_size + 20);
            continue;
        }
        log_info("send line:%d", __LINE__);

        s->send_idx++;
        usleep(s->interval);
        log_info("send tag:%d, idx:%d, size:%d\n", s->tag, i, s->pack_size[size_idx]);
    }

    log_info("send over i:%d, max:%d", i, s->max_times);
    s->max_times = i;

    return NULL;
}


