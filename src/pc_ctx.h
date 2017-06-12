/*
 * FileName : pc_ctx.h
 * Author   : Pengcheng Liu(Lpc-Win32)
 * Date     : 2017-05-15
*/

#pragma once

#include <stdlib.h>

// 确保g++编译器采用c++11标准
#if __cplusplus < 201103L
    #error "complile libpc should use C++11 standard"
#endif

typedef void *(*pcctx_pfn_t)(void *s, void *s2);

struct pcctx_param_t
{
    const void *s1;
    const void *s2;
};

// 此处对cpu架构进行限定，不支持32位位宽以下的cpu架构
// i386架构的通用寄存器数量为8
// 而64位cpu架构的通用寄存器的数量为16
static_assert(sizeof(void *) >= 4, "not allowed whitch cpu less than 32bit");

/* ---32bit cpu-----
 *
 * regs[0] = ret
 * regs[1] = ebx
 * regs[2] = ecx
 * regs[3] = edx
 * regs[4] = edi
 * regs[5] = esi
 * regs[6] = ebp
 * regs[7] = eax = esp
*/

/* ---64bit cpu-----
 * low
 *      regs[0]  = r15
 *      regs[1]  = r14
 *      regs[2]  = r13
 *      regs[3]  = r12
 *      regs[4]  = r9
 *      regs[5]  = r8
 *      regs[6]  = rbp
 *      regs[7]  = rdi
 *      regs[8]  = rsi
 *      regs[9]  = ret      // ret func addr
 *      regs[10] = rdx
 *      regs[11] = rcx
 *      regs[12] = rbx
 * high
 *      regs[13] = rsp
*/

struct pcctx_t
{
#ifdef __i386__
    void *regs[8];
#elif __x86_64__
    void *regs[14];
#else
    #error "libpc support i386 and x86_64 cpu architecture"
#endif
    size_t ss_size;
    char * ss_sp;
};

void pcctx_init(pcctx_t *ctx);

void pcctx_make(pcctx_t *ctx, pcctx_pfn_t pfn,
               const void *s1, const void *s2);
