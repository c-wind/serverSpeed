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

static message_list_t this;

void message_list_init()
{
    memset(&this, 0, sizeof(this));
    this.total = M_64KB;
    pthread_mutex_init(&this.mtx, NULL);
}

void push_action(int id, int what, int type, int tag)
{
    push_message(id, what, type, tag, 0, 0);
}

void push_detail(int id, int type, int tag, int val, int arg1)
{
    push_message(id, ACTION_DETAIL, type, tag, val, arg1);
}

void push_result(int id, int what, int type, int tag, int value, int arg1, int arg2, int arg3, int arg4)
{
    pthread_mutex_lock(&this.mtx);
    if (this.size == this.total)
    {
        log_fatal("msg list full.");
        pthread_mutex_unlock(&this.mtx);
        return;
    }
    this.wpos = (this.wpos + 1) % this.total;
    this.list[this.wpos].id = id;
    this.list[this.wpos].what = what;
    this.list[this.wpos].tm = time(NULL);
    this.list[this.wpos].type = type;
    this.list[this.wpos].tag = tag;
    this.list[this.wpos].value = value;
    this.list[this.wpos].arg1 = arg1;
    this.list[this.wpos].arg2 = arg2;
    this.list[this.wpos].arg3 = arg3;
    this.list[this.wpos].arg4 = arg4;
    this.size++;
    pthread_mutex_unlock(&this.mtx);
}


void push_message(int id, int what, int type, int tag, int value, int arg1)
{
    pthread_mutex_lock(&this.mtx);
    if (this.size == this.total)
    {
        log_fatal("msg list full, size:%d, total:%d", this.size, this.total);
        pthread_mutex_unlock(&this.mtx);
        return;
    }
    this.wpos = (this.wpos + 1) % this.total;
    this.list[this.wpos].id = id;
    this.list[this.wpos].what = what;
    this.list[this.wpos].tm = time(NULL);
    this.list[this.wpos].type = type;
    this.list[this.wpos].tag = tag;
    this.list[this.wpos].value = value;
    this.list[this.wpos].arg1 = arg1;
    this.size++;
    pthread_mutex_unlock(&this.mtx);
}

int pop_message(jni_message_t *j)
{
    pthread_mutex_lock(&this.mtx);
    if (this.size == 0)
    {
        pthread_mutex_unlock(&this.mtx);
        return -1;
    }

    this.rpos = (this.rpos + 1) % this.total;
    memcpy(j, &this.list[this.rpos], sizeof(*j));
    this.size--;
    pthread_mutex_unlock(&this.mtx);
    return 0;
}

