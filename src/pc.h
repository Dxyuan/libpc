/*
 * FileName : pc.h
 * Author   : Pengcheng Liu(Lpc-Win32)
 * Date     : 2017-06-14    Created
*/

#pragma once

#include "pc_schedule.h"
#include "pc_logger.h"

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

        static void *__work_pc(void *arg);

        void operator-(const std::function<void()> &fn);
};

// Add a syntactic sugar PC
#define PC __PcOp()-
