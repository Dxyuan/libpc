/*
 * FileName : pc_schedule.h
 * Author   : Pengcheng Liu(Lpc-Win32)
 * Date     : Mon 26 Jun 2017 10:46:11 PM CST   Created
*/

#pragma once

#include "pc_pool.h"

class PcSchedule
{
    public:
        ~PcSchedule();

        PcSchedule(const PcSchedule &other) = delete;

        PcSchedule *operator =(const PcSchedule &other) = delete;

        int init(int pool_capacity = 256);

        int start();

        static PcSchedule *get_instance();

        static bool schedule_inited();

    private:
        PcSchedule();

        int schedule();

        PcPool *pc_pool_;
        bool    schedule_started_;

        static bool        schedule_inited_;
        static PcSchedule *pc_schedule_instance_;
};
