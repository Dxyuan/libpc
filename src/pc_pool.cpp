/*
 * FileName : pc_pool.cpp
 * Author   : Pengcheng Liu(Lpc-Win32)
 * Date     : 2017-06-25    Created
*/

#include "pc_pool.h"

PcPool *PcPool::pc_pool_instance_ = NULL;

PcPool::PcPool() {}

PcPool *PcPool::get_instance()
{
    if (!pc_pool_instance_) {
        pc_pool_instance_ = new PcPool();
    }
    return pc_pool_instance_;
}

int PcPool::init(int capacity)
{
    if (is_inited_) {
        pc_log_err("PcPool has been inited");
        return 0;
    }

    if (capacity < MIN_POOL_CAPACITY) {
        pc_log_err("PcPool capacity can't less than %d", MIN_POOL_CAPACITY);
        return -1;
    }
    if (capacity > MAX_POOL_CAPACITY) {
        pc_log_err("PcPool capacity can't bigger than %d", MAX_POOL_CAPACITY);
        return -1;
    }
    
    // init pc pool
    for (int index = 0; index < capacity; index++) {
        stPcRoutine_t *pc_ptr;
        pc_create(&pc_ptr, nullptr, nullptr, nullptr);
        if (nullptr == pc_ptr) {
            pc_log_err("pc_create error");
            return -1;
        }
        pc_pool_.push_back(pc_ptr);
    }

    pool_capacity_ = capacity;
    is_inited_ = true;

    return 0;
}

stPcRoutine_t *PcPool::schedule()
{
    if (!is_inited_) {
        pc_log_err("PcPool has not inited");
        return nullptr;
    }
    // get pc from pc_pool
    for (int index = 0; index < pool_capacity_; index++) {
        int real_index = cursor_ + index >= pool_capacity_ ?
                         cursor_ + index - pool_capacity_ : cursor_ + index;
        stPcRoutine_t *curr_pc_ptr = pc_pool_[real_index];
        if (curr_pc_ptr->cEnd) {
            // update cursor_ and return pc_ptr
            cursor_ = real_index;
            curr_pc_ptr->cEnd = 0;
            return curr_pc_ptr;
        }
        if (curr_pc_ptr->pfn == nullptr) {
            cursor_ = real_index;
            return curr_pc_ptr;
        }
    }
    return nullptr;
}

void PcPool::add_used()
{
    if (!is_inited_) {
        pc_log_err("PcPool has not inited");
        return;
    }
    ++pool_used_;
}

void PcPool::del_used()
{
    if (!is_inited_) {
        pc_log_err("PcPool has not inited");
        return;
    }
    --pool_used_;
}

int PcPool::get_uesd()
{
    if (!is_inited_) {
        pc_log_err("PcPool has not inited");
        return -1;
    }
    return pool_used_;
}
