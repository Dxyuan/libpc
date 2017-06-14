/*
 * FileName : pc_epoll.cpp
 * Author   : Pengcheng Liu(Lpc-Win32)
 * Date     : 2017-05-16    Created
*/

#include "pc_epoll.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

int pc_epoll_wait(int epfd, struct pc_epoll_res *events, int maxevents, int timeout)
{
    return epoll_wait(epfd, events->events, maxevents, timeout);
}

int pc_epoll_ctl(int epfd, int op, int fd, struct epoll_event *ev)
{
    return epoll_ctl(epfd, op, fd, ev);
}

int pc_epoll_create(int size)
{
    return epoll_create(size);
}

struct pc_epoll_res *pc_epoll_res_alloc(int n)
{
    struct pc_epoll_res *ptr = (struct pc_epoll_res *)malloc(sizeof(struct pc_epoll_res));
    ptr->size = n;
    ptr->events = (struct epoll_event *)calloc(1, n * sizeof(struct epoll_event));
    return ptr;
}

void pc_epoll_res_free(struct pc_epoll_res *ptr)
{
    if (!ptr) {
        return;
    }
    if (ptr->events) {
        free(ptr->events);
    }
    free(ptr);
}
