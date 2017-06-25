/*
 * FileName : pc_pool.h
 * Author   : Pengcheng Liu(Lpc-Win32)
 * Date     : 2017-06-25    Created
*/

#pragma once

#include <vector>
#include "pc_routine_inner.h"

#define MIN_POOL_CAPACITY 1
#define MAX_POOL_CAPACITY 1024

class PcPool
{
    public:
        PcPool(const PcPool &other) = delete;
        PcPool *operator =(const PcPool &other) = delete;

        int init(int pool_capacity = 256);

        stPcRoutine_t *schedule();

        void add_used();
        void del_used();
        int  get_uesd();

        static PcPool *get_instance();

    private:
        PcPool();

        static PcPool *pc_pool_instance_;

        int pool_capacity_;     // pc_pool capacity
        int pool_used_;
        int cursor_;

        std::vector<stPcRoutine_t *> pc_pool_;
        bool is_inited_;
};
