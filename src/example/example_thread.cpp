/*
 * FileName : example_thread.cpp
 * Author   : Pengcheng Liu
 * Date     : 2017-06-13
*/

#include "libpc/pc_routine.h"
#include "libpc/pc_routine_inner.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>

int loop2(void *)
{
    printf("loop2 ok..\n");
    sleep(1);
    return 0;
}

int loop1(void *)
{
    printf("loop1 ok..\n");
    sleep(1);
    return 0;
}

static void *routine_func(void *)
{
    stPcEpoll_t *ev = pc_get_epoll_ct();    // ct = current thread
    pc_eventloop(ev, loop1, NULL);
    pc_eventloop(ev, loop2, NULL);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        printf("number of arguments is not 2\n");
        return -1;
    }

    int cnt = atoi(argv[1]);

    pthread_t tid[cnt];
    for (int i = 0; i < cnt; i++) {
        pthread_create(tid + i, NULL, routine_func, NULL);
    }
    for (;;) {
        sleep(1);
    }

    return 0;
}
