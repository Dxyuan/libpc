/*
 * FileName : example_specific.cpp
 * Author   : Pengcheng Liu
 * Date     : 2017-06-13
*/

#include <libpc/pc_routine_specific.h>
#include <libpc/pc_routine.h>
#include <unistd.h>
#include <stdio.h>
#include <vector>
#include <iostream>

using namespace std;

struct stRoutineArgs_t
{
    stPcRoutine_t *pc;
    int routine_id;
};

struct stRoutineSpecificData_t
{
    int idx;
};

PC_ROUTINE_SPECIFIC(stRoutineSpecificData_t, __routine);

void *RoutineFunc(void *args)
{
    pc_enable_hook_sys();
    stRoutineArgs_t *routine_args = (stRoutineArgs_t *)args;
    __routine->idx = routine_args->routine_id;

    while (true) {
        printf("%s:%d routine specific data idx %d\n", __func__, __LINE__, __routine->idx);
        poll(NULL, 0, 1000);
    }
    return NULL;
}

int main(int argc, char **argv)
{
    stRoutineArgs_t args[10];
    for (int i = 0; i < 10; i++) {
        args[i].routine_id = i;
        pc_create(&args[i].pc, NULL, RoutineFunc, (void *)&args[i]);
        pc_resume(args[i].pc);
    }
    pc_eventloop(pc_get_epoll_ct(), NULL, NULL);
    return 0;
}
