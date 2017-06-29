/*
 * FileName : pc.h
 * Author   : Pengcheng Liu(Lpc-Win32)
 * Date     : 2017-06-14    Created
*/

#pragma once

#include "pc_schedule.h"

#include <functional>
#include <unistd.h>
#include <assert.h>

class __PcOp {
    public:
        class __PcData {
            public:
                __PcData(const std::function<void()> &fn) {
                    fn_ = fn;
                }

                ~__PcData() {
                    fn_ = NULL;
                }

                std::function<void()> fn_;
        };

        static void *__work_pc(void *arg) {
            __PcData *ptr = (__PcData *)arg;

            // 允许执行hook函数
            pc_enable_hook_sys();

            // 执行我们约定的方法
            ptr->fn_();
            delete ptr;

            return NULL;
        }

        inline void operator-(const std::function<void()> &fn) {
            __PcData *ptr = new __PcData(fn);

            if (PcSchedule::schedule_inited()) {
                // in pc
                // in schedule
                stPcRoutine_t *pc = PcPool::get_instance()->schedule();
                // should add capacity
                if (nullptr == pc) {
                    PcPool *pc_pool_ptr = PcPool::get_instance();
                    int begin_capacity = pc_pool_ptr->capacity();

                    int ret = pc_pool_ptr->add_capacity();
                    if (ret == -1) {
                        int after_capacity = pc_pool_ptr->capacity();

                        if (after_capacity > begin_capacity) {
                            // that means some pc malloc successful
                            stPcRoutine_t *pc = PcPool::get_instance()->schedule();
                            assert(pc != nullptr);
                            pc->pfn = __work_pc;
                            pc->arg = ptr;
                        } else {
                            // that means pc_pool is full
                            while (true) {
                                usleep(30);
                                pc = pc_pool_ptr->schedule();
                                if (nullptr != pc) {
                                    pc->pfn = __work_pc;
                                    pc->arg = ptr;
                                    break;
                                }
                            }
                        }
                    }
                } else {
                    // 正常从pc_pool中拿到pc
                    pc->pfn = __work_pc;
                    pc->arg = ptr;
                }
            } else {
                stPcRoutine_t *pc = PcPool::get_instance()->schedule();
                if (nullptr == pc) {
                    pc_create(&pc, NULL, __work_pc, ptr);
                    pc_resume_free(pc);
                } else {
                    // 从协程栈中取协程处理
                    pc->pfn = __work_pc;
                    pc->arg = ptr;
                    pc_resume(pc);
                }
            }
        }
};

// Add a syntactic sugar PC
#define PC __PcOp()-
