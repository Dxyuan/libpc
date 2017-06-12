/*
 * FileName : pc_routine_inner.h
 * Author   : Pengcheng Liu(Lpc-Win32)
 * Date     : 2017-05-18
*/

#pragma once

#include "pc_routine.h"
#include "pc_ctx.h"

struct stPcRoutineEnv_t;

struct stPcSpec_t
{
    void *value;
};

struct stStackMem_t
{
    stPcRoutine_t *occupy_pc;
    int stack_size;
    char *stack_bp;
    char *stack_buffer;
};

struct stShareStack_t
{
    unsigned int alloc_idx;
    int stack_size;
    int count;
    stStackMem_t **stack_array;
};

struct stPcRoutine_t
{
    stPcRoutineEnv_t *env;
    pfn_pc_routine_t pfn;
    void *arg;
    pcctx_t ctx;

    char cStart;
    char cEnd;
    char cIsMain;
    char cEnableSysHook;
    char cIsShareStack;

    void *pvEnv;
    stStackMem_t *stack_mem;

    char *stack_sp;
    unsigned int save_size;
    char *save_buffer;
    stPcSpec_t aSpec[1024];
};

struct stTimeout_t;
struct stTimeoutItem_t;

stTimeout_t *AllocTimeout(int iSize);
void FreeTimeout(stTimeout_t *apTimeout);
int  AddTimeout(stTimeout_t *apTimeout, stTimeoutItem_t *apItem, uint64_t allNow);

struct stPcEpoll_t;
stPcEpoll_t *AllocEpoll();
void FreeEpoll(stPcEpoll_t *ctx);

stPcRoutine_t *GetCurrThreadPc();
void           SetEpoll(stPcRoutineEnv_t *env, stPcEpoll_t *ev);

typedef void (*pfnPcRoutineFunc_t)();
