/*
 * FileName : pc.h
 * Author   : Pengcheng Liu(Lpc-Win32)
 * Date     : 2017-06-14    Created
*/

#include "pc_pool.h"

#include <iostream>
#include <functional>

#include <iostream>

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

            stPcRoutine_t *pc = PcPool::get_instance()->schedule();
            if (nullptr == pc) {
                std::cout << "not in pool" << std::endl;
                pc_create(&pc, NULL, __work_pc, ptr);
                pc_resume_free(pc);
            } else {
                // 从协程栈中取协程处理
                std::cout << "in pool" << std::endl;
                pc->pfn = __work_pc;
                pc->arg = ptr;
                pc_resume(pc);
            }
        }
};

// Add a syntactic sugar PC
#define PC __PcOp()-
