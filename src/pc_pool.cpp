/*
 * FileName : pc_pool.cpp
 * Author   : Pengcheng Liu(Lpc-Win32)
 * Date     : 2017-06-25    Created
*/

#include "pc_pool.h"
#include "pc_schedule.h"

PcPool *PcPool::pc_pool_instance_ = nullptr;

PcPool::PcPool() {
    for (auto ptr : pc_pool_) {
        free(ptr);
    }
    PcPool::pc_pool_instance_ = nullptr;
}

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
        pc_log_error("PcPool is been inited");
        return 0;
    }

    if (capacity < MIN_POOL_CAPACITY) {
        pc_log_error("PcPool capacity can't less than %d", MIN_POOL_CAPACITY);
        return -1;
    }
    if (capacity > MAX_POOL_CAPACITY) {
        pc_log_error("PcPool capacity can't bigger than %d", MAX_POOL_CAPACITY);
        return -1;
    }
    
    // init pc pool
    for (int index = 0; index < capacity; index++) {
        stPcRoutine_t *pc_ptr;
        pc_create(&pc_ptr, nullptr, nullptr, nullptr);
        if (nullptr == pc_ptr) {
            pc_log_error("pc_create error");
            pool_capacity_ = pool_size_;
            return -1;
        }
        pc_pool_.push_back(pc_ptr);
        ++pool_size_;
    }

    pool_capacity_ = capacity;
    is_inited_ = true;

    return 0;
}

stPcRoutine_t *PcPool::schedule()
{
    if (!is_inited_) {
        pc_log_error("PcPool is not inited");
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
        pc_log_error("PcPool is not inited");
        return;
    }
    ++pool_used_;
}

void PcPool::del_used()
{
    if (!is_inited_) {
        pc_log_error("PcPool is not inited");
        return;
    }
    --pool_used_;
}

int PcPool::get_uesd()
{
    if (!is_inited_) {
        pc_log_error("PcPool is not inited");
        return -1;
    }
    return pool_used_;
}

std::vector<stPcRoutine_t *> PcPool::will_resumes()
{
    std::vector<stPcRoutine_t *> resume_pcs;

    if (!is_inited_) {
        pc_log_error("PcPool is not inited");
        return resume_pcs;
    }
    if (!PcSchedule::schedule_inited()) {
        pc_log_error("PcPool is not inited");
        return resume_pcs;
    }

    for (auto item : pc_pool_) {
        if (item->cStart == 0 && item->cEnd == 0) {
            if (item->pfn != NULL) {
                resume_pcs.push_back(item);
            }
        }
    }

    return resume_pcs;
}

int PcPool::size()
{
    return pool_size_;
}

int PcPool::capacity()
{
    return pool_capacity_;
}

/*
 * return:
 *      0   ok
 *      1   not important, ignore
 *     -1   a. pc pool is full, start pc should wait a moment.
 *          b. malloc error, start pc should wait a moment.
*/
int PcPool::add_capacity(int add_num)
{
    if (!PcSchedule::schedule_inited()) {
        pc_log_error("add_capacity used in pc_schedule");
        return 1;
    }
    if (add_num < 16) {
        pc_log_error("add_num is %d, must greater than 16", add_num);
        return 1;
    }
    if (pool_capacity_ == MAX_POOL_CAPACITY) {
        pc_log_error("pc pool is full, can't add capacity");
        return -1;
    }
    if (pool_capacity_ + add_num > MAX_POOL_CAPACITY) {
        pc_log_error("PcPool capacity can't bigger than %d", MAX_POOL_CAPACITY);
        add_num = MAX_POOL_CAPACITY - pool_capacity_;
    }
    
    for (int index = 0; index < add_num; index++) {
        stPcRoutine_t *pc_ptr;
        pc_create(&pc_ptr, nullptr, nullptr, nullptr);
        if (nullptr == pc_ptr) {
            pc_log_error("pc_create error");
            pool_capacity_ = pool_size_;
            return -1;
        }
        pc_pool_.push_back(pc_ptr);
        ++pool_size_;
    }
    pool_capacity_ += add_num;

    return 0;
}
