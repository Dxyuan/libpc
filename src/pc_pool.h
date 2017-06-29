/*
 * FileName : pc_pool.h
 * Author   : Pengcheng Liu(Lpc-Win32)
 * Date     : 2017-06-25    Created
*/

#pragma once

#include <vector>
#include "pc_routine_inner.h"

const int MIN_POOL_CAPACITY = 1;
const int MAX_POOL_CAPACITY = 10240;

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
        int  size();
        int  capacity();

        // 仅供pc_schedule使用
        std::vector<stPcRoutine_t *> will_resumes();

        int add_capacity(int add_num = 64);

        static PcPool *get_instance();

    private:
        PcPool();

        static PcPool *pc_pool_instance_;

        int pool_capacity_;     // pc_pool capacity
        int pool_size_;
        int pool_used_;
        int cursor_;

        std::vector<stPcRoutine_t *> pc_pool_;
        bool is_inited_;
};
