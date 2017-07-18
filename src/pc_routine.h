#pragma once

#include "pc_logger.h"
#include <stdint.h>
#include <sys/poll.h>
#include <pthread.h>

// 1.struct

struct stPcRoutine_t;
struct stShareStack_t;

// __attribute__((packed))表示不采用系统自动对齐方式
struct stPcRoutineAttr_t
{
    int stack_size;
    stShareStack_t *share_stack;
    stPcRoutineAttr_t()
    {
        // stack_size of routine is 128kb
        stack_size = 128 * 1024;
        share_stack = NULL;
    }
} __attribute__ ((packed));

struct stPcEpoll_t;
typedef int (*pfn_pc_eventloop_t)(void *);
typedef void *(*pfn_pc_routine_t)(void *);

// 2. pc_routine

int  pc_create(stPcRoutine_t **pc_ptr, const stPcRoutineAttr_t *attr,
               pfn_pc_routine_t routine, void *arg);
void pc_resume(stPcRoutine_t *pc);
void pc_resume_free(stPcRoutine_t *pc);
void pc_yield(stPcRoutine_t *pc);
void pc_yield_ct(); // ct = current thread;
void pc_release(stPcRoutine_t *pc);

stPcRoutine_t *pc_self();

int  pc_poll(stPcEpoll_t *ctx, struct pollfd fds[], nfds_t nfds, int timeout_ms);
void pc_eventloop(stPcEpoll_t *ctx, pfn_pc_eventloop_t pfn, void *arg);

// 3.specific
int   pc_setspecific(pthread_key_t key, const void *value);
void *pc_getspecific(pthread_key_t key);

// 4.event
stPcEpoll_t *pc_get_epoll_ct(); // ct - current thread

// 5.hook syscall (poll/read/write/recv/send/recvfrom/sendto)

void pc_enable_hook_sys();
void pc_disable_hook_sys();
bool pc_is_enable_sys_hook();

// 6.sync
struct stPcCond_t;

stPcCond_t *pc_cond_alloc();
int pc_cond_free(stPcCond_t *cc);

int pc_cond_timewait(stPcCond_t *, int timeout_ms);

// 7.share stack
stShareStack_t *pc_alloc_sharestack(int iCount, int iStackSize);

// 8.init envlist for hook get/set env
void pc_set_env_list(const char *name[], size_t cnt);
