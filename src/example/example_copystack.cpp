/*
 * FileName : example_copystack.cpp
 * Author   : Pengcheng Liu(Lpc-Win32)
 * Date     : 2017-06-21
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <libpc/pc_ctx.h>
#include <libpc/pc.h>
#include <libpc/pc_routine_inner.h>

void *routine_func(void *args)
{
    pc_enable_hook_sys();
    int *routine_id = (int *)args;
    while (true) {
        char sBuff[128];
        printf("from routine_id %d stack addr %p\n", *routine_id, sBuff);
        sleep(1);
    }
    return NULL;
}

int main()
{
    stShareStack_t *share_stack = pc_alloc_sharestack(1, 1024 * 128);
    stPcRoutineAttr_t attr;
    attr.stack_size = 0;
    attr.share_stack = share_stack;

    stPcRoutine_t *pc[3];
    int routine_id[3];

    for (int i = 0; i < 3; i++) {
        routine_id[i] = i;
        pc_create(&pc[i], &attr, routine_func, routine_id + i);
        pc_resume(pc[i]);
    }

    pc_eventloop(pc_get_epoll_ct(), NULL, NULL);

    return 0;
}
