#include "pc_routine.h"
#include "pc_routine_inner.h"
#include "pc_epool.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <map>

#include <poll.h>
#include <sys/time.h>
#include <errno.h>

#include <assert.h>

#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/syscall.h>
#include <unistd.h>

#define CALL_STACK_CNT 128

extern "C"
{
    extern void pcctx_swap(pcctx_t *, pcctx_t *) asm("pcctx_swap");
}

stPcRoutine_t *GetCurrPc(stPcRoutineEnv_t *env);
struct stPcEpoll_t;

struct stPcRoutineEnv_t
{
    stPcRoutine_t *pCallStack[CALL_STACK_CNT];
    int iCallStackSize;     // 当前协程战协程数量
    stPcEpoll_t *pEpoll;

    stPcRoutine_t *pending_pc;
    stPcRoutine_t *occupy_pc;
};

void pc_log_err( const char *fmt, ...)
{
    // TODO
}

#ifdef __LIBPC_RDTSCP__
static unsigned long long counter(void)
{
    register uint32_t lo, hi;
    register unsigned long long o;
    __asm__ __volatile__ (
            "rdtscp"
            : "=a"(lo), "=d"(hi)
    );
    o = hi;
    o <<= 32;
    return (o | lo);
}

static unsigned long long getCpuKhz()
{
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (!fp) {
        return 1;
    }
    char buf[4095] = {};
    fread(buf, 1, sizeof(buf), fp);
    fclose(fp);

    char *lp = strstr(buf, "cpu MHz");
    if (!lp) {
        return 1;
    }
    lp += strlen("cpu MHz");
    while (*lp == ' ' || *lp == '\t' || *lp == ':') {
        ++lp;
    }

    double mhz = atof(lp);
    unsigned long long u = (unsigned long long)(mhz * 1000);
    return u;
}
#endif  // __LIBPC_RDTSCP__

static unsigned long long GetTickMS()
{
#ifdef __LIBPC__RDTSCP__
    static uint32_t khz = getCpuKhz();
    return counter() / khz;
#else
    struct timeval now = {};
    gettimeofday(&now, NULL);
    unsigned long long u = now.tv_sec;
    u *= 1000;
    u += now.tv_usec / 1000;
    return u;
#endif
}

static pid_t GetPid()
{
    // 声明时不会改变static修饰的局部变量的值
    // 此处使用static关键字是为了提高GetPid的执行效率
    static __thread pid_t pid = 0;
    static __thread pid_t tid = 0;
    if (!pid || !tid || pid != getpid()) {
        pid = getpid();
        tid = syscall(__NR_gettid);
    }
    return tid;
}

template <class T, class TLink>
void RemoveFromLink(T *ap)
{
    TLink *lst = ap->pLink;
    if (!lst) {
        return;
    }
    assert(lst->head && lst->tail);

    if (ap == lst->head) {
        lst->head = ap->pNext;
        if (lst->head) {
            lst->head->pPrev = NULL;
        }
    } else {
        if (ap->pPrev) {
            ap->pPrev->pNext = ap->pNext;
        }
    }

    if (ap == lst->tail) {
        lst->tail = ap->pPrev;
        if (lst->tail) {
            lst->tail->pNext = NULL;
        }
    } else {
        ap->pNext->pPrev = ap->pPrev;
    }
    ap->pPrev = ap->pNext = NULL;
    ap->pLink = NULL;
}

template <class TNode, class TLink>
void inline AddTail(TLink *apLink, TNode *ap)
{
    if (ap->pLink) {
        return;
    }
    if (apLink->tail) {
        apLink->tail->pNext = (TNode *)ap;
        ap->pNext = NULL;
        ap->pPrev = apLink->tail;
        apLink->tail = ap;
    } else {
        apLink->head = apLink->tail = ap;
        ap->pNext = ap->pPrev = NULL;
    }
    ap->pLink = apLink;
}


template <class TNode, class TLink>
void inline PopHead(TLink *apLink)
{
    if (!apLink->head) {
        return ;
    }
    TNode *lp = apLink->head;
    if (apLink->head == apLink->tail) {
        apLink->head = apLink->tail = NULL;
    } else {
        apLink->head = apLink->head->pNext;
    }

    lp->pPrev = lp->pNext = NULL;
    lp->pLink = NULL;

    if (apLink->head) {
        apLink->head->pPrev = NULL;
    }
}

template <class TNode, class TLink>
void inline Join(TLink *apLink, TLink *apOther)
{
    if (!apOther->head) {
        return ;
    }
    TNode *lp = apOther->head;
    while (lp) {
        lp->pLink = apLink;
        lp = lp->pNext;
    }
    lp = apOther->head;
    if (apLink->tail) {
        apLink->tail->pNext = (TNode*)lp;
        lp->pPrev = apLink->tail;
        apLink->tail = apOther->tail;
    } else {
        apLink->head = apOther->head;
        apLink->tail = apOther->tail;
    }

    apOther->head = apOther->tail = NULL;
}

stStackMem_t *pc_alloc_stackmem(unsigned int stack_size)
{
    stStackMem_t *stack_mem = (stStackMem_t *)malloc(sizeof(stStackMem_t));
    stack_mem->occupy_pc = NULL;
    stack_mem->stack_size = stack_size;
    stack_mem->stack_buffer = (char *)malloc(stack_size);
    stack_mem->stack_bp = stack_mem->stack_buffer + stack_size;
    return stack_mem;
}

stShareStack_t *pc_alloc_sharestack(int count, int stack_size)
{
    stShareStack_t *share_stack = (stShareStack_t *)malloc(sizeof(stShareStack_t));
    share_stack->alloc_idx = 0;
    share_stack->stack_size = stack_size;

    share_stack->count = count;
    stStackMem_t **stack_array = (stStackMem_t **)calloc(count, sizeof(stStackMem_t *));
    for (int i = 0; i < count; i++) {
        stack_array[i] = pc_alloc_stackmem(stack_size);
    }
    share_stack->stack_array = stack_array;
    return share_stack;
}

static stStackMem_t *pc_get_stackmem(stShareStack_t *share_stack)
{
    if (!share_stack) {
        return NULL;
    }
    int idx = share_stack->alloc_idx % share_stack->count;
    share_stack->alloc_idx++;

    return share_stack->stack_array[idx];
}

struct stTimeoutItemLink_t;
struct stTimeoutItem_t;
struct stPcEpoll_t
{
    int iEpollFd;
    static const int _EPOLL_SIZE = 1024 * 10;
    struct stTimeout_t *pTimeout;
    struct stTimeoutItemLink_t *pstTimeoutList;
    struct stTimeoutItemLink_t *pstActiveList;
    pc_epoll_res *result;
};

typedef void (*OnPreparePfn_t)(stTimeoutItem_t *, struct epoll_event &ev, stTimeoutItemLink_t *active);
typedef void (*OnProcessPfn_t)(stTimeoutItem_t *);
struct stTimeoutItem_t
{
    enum {
        eMaxTimeout = 40 * 1000    // 40s
    };
    stTimeoutItem_t *pPrev;
	stTimeoutItem_t *pNext;
	stTimeoutItemLink_t *pLink;

	unsigned long long ullExpireTime;

	OnPreparePfn_t pfnPrepare;
	OnProcessPfn_t pfnProcess;

    void *pArg; //routine
    bool bTimeout;
};

struct stTimeoutItemLink_t
{
    stTimeoutItem_t *head;
    stTimeoutItem_t *tail;
};

struct stTimeout_t
{
    stTimeoutItemLink_t *pItems;
    int iItemSize;

    unsigned long long ullStart;
    long long llStartIdx;
};

stTimeout_t *AllocTimeout(int iSize)
{
    stTimeout_t *lp = (stTimeout_t *)calloc(1, sizeof(stTimeout_t));

    lp->iItemSize= iSize;
    lp->pItems = (stTimeoutItemLink_t *)calloc(1, sizeof(stTimeoutItemLink_t) * lp->iItemSize);

    lp->ullStart = GetTickMS();
    lp->llStartIdx = 0;

    return lp;
};

void FreeTimeout(stTimeout_t *apTimeout)
{
    free(apTimeout->pItems);
    free(apTimeout);
}

int AddTimeout(stTimeout_t *apTimeout, stTimeoutItem_t *apItem, unsigned long long allNow)
{
    if (apTimeout->ullStart == 0) {
        apTimeout->ullStart = allNow;
        apTimeout->llStartIdx = 0;
    }
    if (allNow < apTimeout->ullStart) {
        pc_log_err("CO_ERR: AddTimeout line %d allNow %llu apTimeout->ullStart %llu",
                   __LINE__, allNow, apTimeout->ullStart);
        return __LINE__;
    }
	if (apItem->ullExpireTime < allNow) {
        pc_log_err("CO_ERR: AddTimeout line %d apItem->ullExpireTime %llu allNow %llu apTimeout->ullStart %llu",
                   __LINE__, apItem->ullExpireTime, allNow, apTimeout->ullStart);
        return __LINE__;
    }
    int diff = apItem->ullExpireTime - apTimeout->ullStart;

    if (diff >= apTimeout->iItemSize) {
        pc_log_err("CO_ERR: AddTimeout line %d diff %d",
                   __LINE__, diff);
        return __LINE__;
    }
	AddTail(apTimeout->pItems + (apTimeout->llStartIdx + diff) % apTimeout->iItemSize, apItem);

	return 0;
}

inline void TakeAllTimeout(stTimeout_t *apTimeout, unsigned long long allNow, stTimeoutItemLink_t *apResult)
{
    if (apTimeout->ullStart == 0) {
        apTimeout->ullStart = allNow;
        apTimeout->llStartIdx = 0;
    }

    if (allNow < apTimeout->ullStart) {
        return;
    }
    int cnt = allNow - apTimeout->ullStart + 1;
    if (cnt > apTimeout->iItemSize) {
        cnt = apTimeout->iItemSize;
    }
    if (cnt < 0) {
        return;
    }
    for (int i = 0; i < cnt; i++) {
        int idx = (apTimeout->llStartIdx + i) % apTimeout->iItemSize;
        Join<stTimeoutItem_t, stTimeoutItemLink_t>(apResult, apTimeout->pItems + idx);
    }
    apTimeout->ullStart = allNow;
    apTimeout->llStartIdx += cnt - 1;
}

void pc_yield_env(stPcRoutineEnv_t *env);
stPcRoutineEnv_t *pc_get_curr_thread_env();
void pc_init_curr_thread_env();
void pc_swap(stPcRoutine_t *curr, stPcRoutine_t *pending_pc);

static int PcRoutineFunc(stPcRoutine_t *pc, void *)
{
    if (pc->pfn) {
        pc->pfn(pc->arg);
    }
    pc->cEnd = 1;

    stPcRoutineEnv_t *env = pc->env;
    pc_yield_env(env);

    return 0;
}

struct stPcRoutine_t *pc_create_env(stPcRoutineEnv_t *env, const stPcRoutineAttr_t *attr,
                                    pfn_pc_routine_t pfn, void *arg)
{
    // 设置协程栈的属性
    stPcRoutineAttr_t at;

    // attr复用
    if (attr) {
        memcpy(&at, attr, sizeof(at));
    }
    if (at.stack_size <= 0) {
        // 程序中默认的协程栈大小为128k
        at.stack_size = 128 * 1024;
    } else if (at.stack_size > 1024 * 1024 * 8) {
        // 程序中最大的协程栈大小为8M
        at.stack_size = 1024 * 1024 * 8;
    }
    // 确保协程栈的大小为4k的倍数
    if (at.stack_size & 0xFFF) {
        at.stack_size &= ~0xFFF;
        at.stack_size += 0x1000;
    }
    // 创建一个协程对象
    stPcRoutine_t *lp = (stPcRoutine_t *)malloc(sizeof(stPcRoutine_t));
    memset(lp, 0, (long)(sizeof(stPcRoutine_t)));

    lp->env = env;
    lp->pfn = pfn;
    lp->arg = arg;

    stStackMem_t *stack_mem = NULL;
    if (at.share_stack) {
        stack_mem = pc_get_stackmem(at.share_stack);
    } else {
        stack_mem = pc_alloc_stackmem(at.stack_size);
    }
    lp->stack_mem = stack_mem;

    lp->ctx.ss_sp = stack_mem->stack_buffer;
    lp->ctx.ss_size = at.stack_size;

    lp->cStart = 0;
    lp->cEnd = 0;
    lp->cIsMain = 0;
    lp->cEnableSysHook = 0;
    lp->cIsShareStack = at.share_stack != NULL;

    lp->save_size = 0;
    lp->save_buffer = NULL;

    return lp;
}

int pc_create(stPcRoutine_t **pppc, const stPcRoutineAttr_t *attr, pfn_pc_routine_t pfn, void *arg)
{
    if (!pc_get_curr_thread_env()) {
        pc_init_curr_thread_env();
    }
    stPcRoutine_t *pc = pc_create_env(pc_get_curr_thread_env(), attr, pfn, arg);
    *pppc = pc;
    return 0;
}

void pc_free(stPcRoutine_t *pc)
{
    free(pc);
}

void pc_release(stPcRoutine_t *pc)
{
    if (pc->cEnd) {
        free(pc);
    }
}

void pc_resume(stPcRoutine_t *pc)
{
    stPcRoutineEnv_t *env = pc->env;
    stPcRoutine_t *lpCurrRoutine = env->pCallStack[env->iCallStackSize - 1];
    if (!pc->cStart) {
        pcctx_make(&pc->ctx, (pcctx_pfn_t)PcRoutineFunc, pc, 0);
        pc->cStart = 1;
    }
    env->pCallStack[env->iCallStackSize] = pc;
    env->iCallStackSize++;
    // 恢复当前协程运行
    pc_swap(lpCurrRoutine, pc);
}

void pc_yield_env(stPcRoutineEnv_t *env)
{
    stPcRoutine_t *last = env->pCallStack[env->iCallStackSize - 2];
    stPcRoutine_t *curr = env->pCallStack[env->iCallStackSize - 1];
    env->iCallStackSize--;

    pc_swap(curr, last);
}

void pc_yield_ct()
{
    pc_yield_env(pc_get_curr_thread_env());
}

void pc_yield(stPcRoutine_t *pc)
{
    pc_yield_env(pc->env);
}

// TODO
void save_stack_buffer(stPcRoutine_t *occupy_pc)
{
    stStackMem_t *stack_mem = occupy_pc->stack_mem;
    // 结构体定义
    int len = stack_mem->stack_bp - occupy_pc->stack_sp;

    if (occupy_pc->save_buffer) {
        free(occupy_pc->save_buffer);
        occupy_pc->save_buffer = NULL;
    }
    occupy_pc->save_buffer = (char *)malloc(len);
    occupy_pc->save_size = len;

    memcpy(occupy_pc->save_buffer, occupy_pc->stack_sp, len);
}

void pc_swap(stPcRoutine_t *curr, stPcRoutine_t *pending_pc)
{
    stPcRoutineEnv_t *env = pc_get_curr_thread_env();

    // get curr stack sp, sp -> &current_sp_obj
    char current_sp_obj;
    curr->stack_sp = &current_sp_obj;

    // 仅在内存共享的情况下才存在协程的调度，否则没有
    if (!pending_pc->cIsShareStack) {
        env->pending_pc = NULL;
        env->occupy_pc  = NULL;
    } else {
        env->pending_pc = pending_pc;
        stPcRoutine_t *occupy_pc = pending_pc->stack_mem->occupy_pc;
        pending_pc->stack_mem->occupy_pc = pending_pc;

        env->occupy_pc = occupy_pc;
        if (occupy_pc && occupy_pc != pending_pc) {
            // 即将swap掉的occupy协程保存stack状态
            save_stack_buffer(occupy_pc);
        }
    }
    pcctx_swap(&(curr->ctx), &(pending_pc->ctx));

    // stack buffer may be overwrite, so get again
    stPcRoutineEnv_t *curr_env       = pc_get_curr_thread_env();
    stPcRoutine_t *update_occupy_pc  = curr_env->occupy_pc;
    stPcRoutine_t *update_pending_pc = curr_env->pending_pc;

    if (update_occupy_pc && update_pending_pc && update_occupy_pc != update_pending_pc) {
        if (update_pending_pc->save_buffer && update_pending_pc->save_size > 0) {
            memcpy(update_pending_pc->stack_sp, update_pending_pc->save_buffer, update_pending_pc->save_size);
        }
    }
}

struct stPollItem_t;
struct stPoll_t : public stTimeoutItem_t
{
    struct pollfd *fds;
    nfds_t nfds;
    stPollItem_t *pPollItems;
    int iAllEventDetach;
    int iEpollFd;
    int iRaiseCnt;
};

struct stPollItem_t : public stTimeoutItem_t
{
    struct pollfd *pSelf;
    stPoll_t *pPoll;
    struct epoll_event stEvent;
};

static uint32_t PollEvent2Epoll(short events)
{
    uint32_t e = 0;
    if (events & POLLIN)     e |= EPOLLIN;
    if (events & POLLOUT)    e |= EPOLLOUT;
    if (events & POLLHUP)    e |= EPOLLHUP;
    if (events & POLLERR)    e |= EPOLLERR;
    if (events & POLLRDNORM) e |= EPOLLRDNORM;
    if (events & POLLWRNORM) e |= EPOLLWRNORM;
    return e;
}

static short EpollEvent2Poll(uint32_t events)
{
    short e = 0;
    if (events & EPOLLIN)     e |= POLLIN;
    if (events & EPOLLOUT)    e |= POLLOUT;
    if (events & EPOLLHUP)    e |= POLLHUP;
    if (events & EPOLLERR)    e |= POLLERR;
    if (events & EPOLLRDNORM) e |= POLLRDNORM;
    if (events & EPOLLWRNORM) e |= POLLWRNORM;
    return e;
}

static stPcRoutineEnv_t *g_arrPcEnvPerThread[204800] = { 0 };

void pc_init_curr_thread_env()
{
    pid_t pid = GetPid();
    g_arrPcEnvPerThread[pid] = (stPcRoutineEnv_t *)calloc(1, sizeof(stPcRoutineEnv_t));
    stPcRoutineEnv_t *env = g_arrPcEnvPerThread[pid];

    env->iCallStackSize = 0;
    struct stPcRoutine_t *self = pc_create_env(env, NULL, NULL, NULL);
    self->cIsMain = 1;

    env->pending_pc = NULL;
    env->occupy_pc = NULL;

    pcctx_init(&self->ctx);

    env->pCallStack[env->iCallStackSize] = self;
    env->iCallStackSize++;

    stPcEpoll_t *ev = AllocEpoll();
    SetEpoll(env, ev);
}

stPcRoutineEnv_t *pc_get_curr_thread_env()
{
    return g_arrPcEnvPerThread[GetPid()];
}

void OnPollProcessEvent(stTimeoutItem_t *ap)
{
    stPcRoutine_t *pc = (stPcRoutine_t *)ap->pArg;
    pc_resume(pc);
}

void OnPollPreparePfn(stTimeoutItem_t *ap, struct epoll_event &e, stTimeoutItemLink_t *active)
{
    stPollItem_t *lp = (stPollItem_t *)ap;
    lp->pSelf->revents = EpollEvent2Poll(e.events);

    stPoll_t *pPoll = lp->pPoll;
    pPoll->iRaiseCnt++;

    if (!pPoll->iAllEventDetach) {
        pPoll->iAllEventDetach = 1;
        RemoveFromLink<stTimeoutItem_t, stTimeoutItemLink_t>(pPoll);
        AddTail(active, pPoll);
    }
}

void pc_eventloop(stPcEpoll_t *ctx, pfn_pc_eventloop_t pfn, void *arg)
{
    if (!ctx->result) {
        ctx->result = pc_epoll_res_alloc(stPcEpoll_t::_EPOLL_SIZE);
    }
    pc_epoll_res *result = ctx->result;

    for (;;) {
        int ret = pc_epoll_wait(ctx->iEpollFd, result, stPcEpoll_t::_EPOLL_SIZE, 1);
        stTimeoutItemLink_t *active = (ctx->pstActiveList);
        stTimeoutItemLink_t *timeout = (ctx->pstTimeoutList);

        memset(timeout, 0, sizeof(stTimeoutItemLink_t));

        for (int i = 0; i < ret; i++) {
            stTimeoutItem_t *item = (stTimeoutItem_t *)result->events[i].data.ptr;
            if (item->pfnPrepare) {
                item->pfnPrepare(item, result->events[i], active);
            } else {
                AddTail(active, item);
            }
        }

        unsigned long long now = GetTickMS();
        TakeAllTimeout(ctx->pTimeout, now, timeout);
        
        stTimeoutItem_t *lp = timeout->head;
        while (lp) {
            lp->bTimeout = true;
            lp = lp->pNext;
        }

        Join<stTimeoutItem_t, stTimeoutItemLink_t>(active, timeout);

        lp = active->head;
        while (lp) {
            PopHead<stTimeoutItem_t, stTimeoutItemLink_t>(active);
            if (lp->pfnProcess) {
                lp->pfnProcess(lp);
            }
            lp = active->head;
        }
        if (pfn) {
            if (-1 == pfn(arg)) {
                break;
            }
        }
    }
}

void OnPcroutineEvent(stTimeoutItem_t *ap)
{
    stPcRoutine_t *pc = (stPcRoutine_t *)ap->pArg;
    pc_resume(pc);
}

stPcEpoll_t *AllocEpoll()
{
    stPcEpoll_t *ctx = (stPcEpoll_t *)calloc(1, sizeof(stPcEpoll_t));
    ctx->iEpollFd = pc_epoll_create(stPcEpoll_t::_EPOLL_SIZE);
    ctx->pTimeout = AllocTimeout(60 * 1000);

    ctx->pstActiveList = (stTimeoutItemLink_t *)calloc(1, sizeof(stTimeoutItemLink_t));
    ctx->pstTimeoutList = (stTimeoutItemLink_t *)calloc(1, sizeof(stTimeoutItemLink_t));

    return ctx;
}

void FreeEpoll(stPcEpoll_t *ctx)
{
    if (ctx) {
        free(ctx->pstActiveList);
        free(ctx->pstTimeoutList);
        FreeTimeout(ctx->pTimeout);
        pc_epoll_res_free(ctx->result);
    }
    free(ctx);
}

stPcRoutine_t *GetCurrPc(stPcRoutineEnv_t *env)
{
    return env->pCallStack[env->iCallStackSize - 1];
}

stPcRoutine_t *GetCurrThreadPc()
{
    stPcRoutineEnv_t *env = pc_get_curr_thread_env();
    if (!env) {
        return 0;
    }
    return GetCurrPc(env);
}

typedef int (*poll_pfn_t)(struct pollfd fds[], nfds_t nfds, int timeout);

int pc_poll_inner(stPcEpoll_t *ctx, struct pollfd fds[], nfds_t nfds, int timeout, poll_pfn_t pollfunc)
{
    if (timeout > stTimeoutItem_t::eMaxTimeout) {
        timeout = stTimeoutItem_t::eMaxTimeout;
    }
    int epfd = ctx->iEpollFd;
    stPcRoutine_t *self = pc_self();

    // 1. struct change
    stPoll_t &arg = *((stPoll_t *)malloc(sizeof(stPoll_t)));
    memset(&arg, 0, sizeof(arg));

    arg.iEpollFd = epfd;
    arg.fds = (pollfd *)calloc(nfds, sizeof(pollfd));
    arg.nfds = nfds;

    stPollItem_t arr[2];
    if (nfds < sizeof(arr) / sizeof(arr[0]) && !self->cIsShareStack) {
        arg.pPollItems = arr;
    } else {
        arg.pPollItems = (stPollItem_t *)malloc(nfds * sizeof(stPollItem_t));
    }
    memset(arg.pPollItems, 0, nfds * sizeof(stPollItem_t));

    arg.pfnProcess = OnPollProcessEvent;
    arg.pArg = GetCurrPc(pc_get_curr_thread_env());

    // 2. add epoll
    for (nfds_t i = 0; i < nfds; i++) {
        arg.pPollItems[i].pSelf = arg.fds + i;
        arg.pPollItems[i].pPoll = &arg;

        arg.pPollItems[i].pfnPrepare = OnPollPreparePfn;
        struct epoll_event &ev = arg.pPollItems[i].stEvent;

        if (fds[i].fd > -1) {
            ev.data.ptr = arg.pPollItems + i;
            ev.events = PollEvent2Epoll(fds[i].events);

            int ret = pc_epoll_ctl(epfd, EPOLL_CTL_ADD, fds[i].fd, &ev);
            if (ret < 0 && errno == EPERM && nfds == 1 && pollfunc != NULL) {
                if (arg.pPollItems != arr) {
                    free(arg.pPollItems);
                    arg.pPollItems = NULL;
                }
                free(arg.fds);
                free(&arg);
                return pollfunc(fds, nfds, timeout);
            }
        }
    }

    // 3. add timeout
    unsigned long long now = GetTickMS();
    arg.ullExpireTime = now + timeout;
    int ret = AddTimeout(ctx->pTimeout, &arg, now);
    if (ret != 0) {
        pc_log_err("PC_ERR: AddTimeout ret %d now %lld timeout %d arg.ullExpireTime %lld",
                ret, now, timeout, arg.ullExpireTime);
        errno = EINVAL;

        if (arg.pPollItems != arr) {
            free(arg.pPollItems);
            arg.pPollItems = NULL;
        }
        free(arg.fds);
        free(&arg);

        return -__LINE__;
    }

    pc_yield_env(pc_get_curr_thread_env());

    RemoveFromLink<stTimeoutItem_t, stTimeoutItemLink_t>(&arg);
    for (nfds_t i = 0; i < nfds; i++) {
        int fd = fds[i].fd;
        if (fd > -1) {
            pc_epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &arg.pPollItems[i].stEvent);
        }
        fds[i].revents = arg.fds[i].revents;
    }

    int iRaiseCnt = arg.iRaiseCnt;
    if (arg.pPollItems != arr) {
        free(arg.pPollItems);
        arg.pPollItems = NULL;
    }

    free(arg.fds);
    free(&arg);

    return iRaiseCnt;
}

int pc_poll(stPcEpoll_t *ctx, struct pollfd fds[], nfds_t nfds, int timeout_ms)
{
    return pc_poll_inner(ctx, fds, nfds, timeout_ms, NULL);
}

void SetEpoll(stPcRoutineEnv_t *env, stPcEpoll_t *ev)
{
    env->pEpoll = ev;
}

stPcEpoll_t *pc_get_epoll_ct()
{
    if (!pc_get_curr_thread_env()) {
        pc_init_curr_thread_env();
    }
    return pc_get_curr_thread_env()->pEpoll;
}

struct stHookPThreadSpec_t
{
    stPcRoutine_t *pc;
    void *value;

    enum {
        size = 1024
    };
};

void *pc_getspecific(pthread_key_t key)
{
    stPcRoutine_t *pc = GetCurrThreadPc();
    if (!pc || pc->cIsMain) {
        return pthread_getspecific(key);
    }
    return pc->aSpec[key].value;
}

int pc_setspecific(pthread_key_t key, const void *value)
{
    stPcRoutine_t *pc = GetCurrThreadPc();
    if (!pc || pc->cIsMain) {
        return pthread_setspecific(key, value);
    }
    pc->aSpec[key].value = (void *)value;
    return 0;
}

void pc_disable_hook_sys()
{
    stPcRoutine_t *pc = GetCurrThreadPc();
    if (pc) {
        pc->cEnableSysHook = 0;
    }
}

bool pc_is_enable_sys_hook()
{
    stPcRoutine_t *pc = GetCurrThreadPc();
    return (pc && pc->cEnableSysHook);
}

stPcRoutine_t *pc_self()
{
    return GetCurrThreadPc();
}

// pc cond
struct stPcCond_t;
struct stPcCondItem_t
{
    stPcCondItem_t *pPrev;
    stPcCondItem_t *pNext;
    stPcCond_t *pLink;

    stTimeoutItem_t timeout;
};

struct stPcCond_t
{
    stPcCondItem_t *head;
    stPcCondItem_t *tail;
};

static void OnSignalProcessEvent(stTimeoutItem_t *ap)
{
    stPcRoutine_t *pc = (stPcRoutine_t *)ap->pArg;
    pc_resume(pc);
}

stPcCondItem_t *pc_cond_pop(stPcCond_t *link);
int pc_cond_signal(stPcCond_t *so)
{
    stPcCondItem_t *sp = pc_cond_pop(so);
    if (!sp) {
        return 0;
    }
    RemoveFromLink<stTimeoutItem_t, stTimeoutItemLink_t>(&sp->timeout);
    AddTail(pc_get_curr_thread_env()->pEpoll->pstActiveList, &sp->timeout);
    return 0;
}

int pc_cond_broadcast(stPcCond_t *si)
{
    for (;;) {
        stPcCondItem_t *sp = pc_cond_pop(si);
        if (!sp) {
            return 0;
        }
        RemoveFromLink<stTimeoutItem_t, stTimeoutItemLink_t>(&sp->timeout);
        AddTail(pc_get_curr_thread_env()->pEpoll->pstActiveList, &sp->timeout);
    }
}

int pc_cond_timedwait(stPcCond_t *link, int ms)
{
    stPcCondItem_t *psi = (stPcCondItem_t *)calloc(1, sizeof(stPcCondItem_t));
    psi->timeout.pArg = GetCurrThreadPc();
    psi->timeout.pfnProcess = OnSignalProcessEvent;

    if (ms > 0) {
        unsigned long long now = GetTickMS();
        psi->timeout.ullExpireTime = now + ms;

        int ret = AddTimeout(pc_get_curr_thread_env()->pEpoll->pTimeout, &psi->timeout, now);
        if (ret != 0) {
            free(psi);
            return ret;
        }
    }
    AddTail(link, psi);
    pc_yield_ct();

    RemoveFromLink<stPcCondItem_t, stPcCond_t>(psi);
    free(psi);

    return 0;
}

stPcCond_t *pc_cond_alloc()
{
    return (stPcCond_t *)calloc(1, sizeof(stPcCond_t));
}

int pc_cond_free(stPcCond_t *pc_cond)
{
    free(pc_cond);
    return 0;
}

stPcCondItem_t *pc_cond_pop(stPcCond_t *link)
{
    stPcCondItem_t *p = link->head;
    if (p) {
        PopHead<stPcCondItem_t, stPcCond_t>(link);
    }
    return p;
}
