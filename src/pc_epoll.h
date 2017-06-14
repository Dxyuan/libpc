/*
 * FileName : pc_epoll.h
 * Author   : Pengcheng Liu(Lpc-Win32)
 * Date     : 2017-05-16    Created
*/

#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <sys/epoll.h>

struct pc_epoll_res
{
    int size;
    struct epoll_event *events;
};

int pc_epoll_wait(int epfd, struct pc_epoll_res *events,
                  int maxevents, int timeout);

int pc_epoll_ctl(int epfd, int op, int fd, struct epoll_event *);

int pc_epoll_create(int size);

struct pc_epoll_res *pc_epoll_res_alloc(int n);

void pc_epoll_res_free(struct pc_epoll_res *);
