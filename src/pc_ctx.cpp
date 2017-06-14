/*
 * FileName : pc_ctx.cpp
 * Author   : Pengcheng Liu(Lpc-Win32)
 * Date     : 2017-05-28    Created
*/

#include "pc_ctx.h"
#include <string.h>

#define ESP 0
#define EIP 1
#define EAX 2
#define ECX 3

#define RSP 0
#define RIP 1
#define RBX 2
#define RDI 3
#define RSI 4
#define RBP 5
#define R12 6
#define R13 7
#define R14 8
#define R15 9
#define RDX 10
#define RCX 11
#define R8  12
#define R9  13

// 32bit
enum
{
    kEIP = 0,
    kESP = 7
};

// 64bit
enum
{
    kRDI = 7,
    kRSI = 8,
    kRETAddr = 9,
    kRSP = 13
};

extern "C"
{
    extern void pcctx_swap(pcctx_t *, pcctx_t *)
        asm ("pcctx_swap");
};

void pcctx_init(pcctx_t *ctx)
{
    bzero(ctx, sizeof(*ctx));
}

#ifdef __i386__
void pcctx_make(pcctx_t *ctx, pcctx_param_t pfn,
               const void *s1, const void *s2)
{
    // make room for pcctx_param
    char *sp = ctx->ss_sp + ctx->ss_size - sizeof(pcctx_param_t);
    sp = (char *)((unsigned long)sp & -16L);

    pcctx_param_t *param = (pcctx_param_t *)sp;
    param->s1 = s;
    param->s2 = s1;

    // 清理寄存器的值为地址0
    bzero(ctx->regs, sizeof(ctx->regs));

    ctx->regs[kESP] = (char *)(sp) - sizeof(void *);
    ctx->regs[kEIP] = (char *)pfn;
}

#elif __x86_64__
void pcctx_make(pcctx_t *ctx, pcctx_pfn_t pfn,
               const void *s1, const void *s2)
{
    char *sp = ctx->ss_sp + ctx->ss_size;
    sp = (char *)((unsigned long)sp & -16LL);

    bzero(ctx->regs, sizeof(ctx->regs));

    ctx->regs[kRSP] = sp - 8;
    ctx->regs[kRETAddr] = (char *)pfn;
    ctx->regs[kRDI] = (char *)s1;
    ctx->regs[kRSI] = (char *)s2;
}

#endif
