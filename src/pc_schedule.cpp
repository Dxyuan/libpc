/*
 * FileName : pc_schedule.cpp
 * Author   : Pengcheng Liu(Lpc-Win32)
 * Date     : Mon 26 Jun 2017 11:33:36 PM CST   Created
*/

#include <vector>
#include <unistd.h>

#include "pc_schedule.h"

bool PcSchedule::schedule_inited_ = false;

PcSchedule *PcSchedule::pc_schedule_instance_ = nullptr;

PcSchedule::~PcSchedule()
{
    delete pc_pool_;
    PcSchedule::pc_schedule_instance_ = nullptr;
}

// 此处用于告知pc_pool是否启用调度器
bool PcSchedule::schedule_inited()
{
    return schedule_inited_;
}

PcSchedule::PcSchedule()
{
    pc_pool_ = PcPool::get_instance();
}

PcSchedule *PcSchedule::get_instance()
{
    if (pc_schedule_instance_ == nullptr) {
        pc_schedule_instance_ = new PcSchedule();
    }
    return pc_schedule_instance_;
}

int PcSchedule::init(int pool_capacity)
{
    if (schedule_inited_) {
        pc_log_error("pc_schedule has been inited");
        return -1;
    }

    int ret = pc_pool_->init(pool_capacity);
    if (ret == -1) {
        pc_log_error("pc pool init error");
        return -1;
    }
    schedule_inited_ = true;

    // pc_log_error("TEST ok");

    return 0;
}

// TODO 此处不会在线程环境中运行的判断
int PcSchedule::start()
{
    if (!schedule_inited_) {
        pc_log_error("pc_schedule must start");
        return -1;
    }
    if (schedule_started_) {
        pc_log_error("pc_schedule has been started");
        return -1;
    }

    schedule_inited_ = true;

    // start schedule()
    while (true) {
        if (schedule()) {
            // do not have pc can resume
            usleep(30 * 1000);
        }
    }
    return 0;
}

int PcSchedule::schedule()
{
    std::vector<stPcRoutine_t *> pcs = PcPool::get_instance()->will_resumes();
    if (pcs.size() == 0) {
        // 没有可以调度的pc
        return 1;
    }
    for (auto item : pcs) {
        // 执行可调度的pc
        pc_resume(item);
    }
    return 0;
}
