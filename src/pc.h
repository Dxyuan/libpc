/*
 * FileName : pc.h
 * Author   : Pengcheng Liu(Lpc-Win32)
 * Date     : 2017-06-14    Created
*/

#include "pc_routine.h"

#include <functional>

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
            ptr->fn_();
            delete ptr;
            return NULL;
        }

        inline void operator-(const std::function<void()> &fn) {
            __PcData *ptr = new __PcData(fn);
            stPcRoutine_t *pc;
            pc_create(&pc, NULL, __work_pc, ptr);
            pc_resume(pc);
        }
};

// Add a syntactic sugar PC
#define PC __PcOp()-
