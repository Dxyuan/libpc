/*
 * FileName : pc_logger.cpp
 * Author   : Pengcheng Liu(Lpc-Win32)
 * Date     : Sun 02 Jul 2017 04:45:11 PM CST   Created
*/

#include "pc_logger.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>

#define MAX(a, b)           (a) > (b) ? (a) : (b)
#define MIN(a, b)           (a) < (b) ? (a) : (b)

#define LOG_MAX_LEN         66635

static const char *pc_log_level_str[7] = {
    "FATAL",
    "ERROR",
    "WARN",
    "NOTICE",
    "INFO",
    "DEBUG",
    "FULL"
};

PcLog::PcLog() noexcept
{
    log_ = new logger_t();
    if (nullptr == log_) {
        throw "logger_t new error";
    }
    log_->log_level = LOG_LEVEL_FULL;

    log_buffer_ = (char *)malloc(LOG_MAX_LEN);
    if (nullptr == log_buffer_) {
        throw "log_buffer_ malloc error";
    }
}

PcLog::~PcLog()
{
    // strdup use malloc
    free(log_->log_name);
    free(log_buffer_);
    delete log_;
}

PcLog *PcLog::pc_log_instance_ = nullptr;

PcLog *PcLog::get_instance()
{
    if (pc_log_instance_ == nullptr) {
        pc_log_instance_ = new PcLog();
    }
    return pc_log_instance_;
}

void PcLog::init(char *log_path, int log_level)
{
    log_->log_level = MAX(LOG_LEVEL_ERR, MIN(LOG_LEVEL_FULL, log_level));

    if (log_path == nullptr) {
        log_->log_fd = STDOUT_FILENO;
        return;
    }
    if (log_path[0] == '\0') {
        log_->log_fd = STDOUT_FILENO;
        return;
    }

    log_->log_name = strdup(log_path);

    int fd = open(log_path, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd == -1) {
        fd = STDOUT_FILENO;
    }
    log_->log_fd = fd;
}

void PcLog::pc_log_reopen()
{
    if (log_->log_fd == STDOUT_FILENO) {
        return;
    }

    // O_APPEND is importent
    int fd = open(log_->log_name, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd == -1) {
        fd = STDOUT_FILENO;
    }
    log_->log_fd = fd;
}

void PcLog::pc_log_level_set(int log_level)
{
    log_->log_level = MAX(LOG_LEVEL_ERR, MIN(LOG_LEVEL_FULL, log_level));
}

void PcLog::pc_log_level_up()
{
    if (log_->log_level < LOG_LEVEL_FULL) {
        --(log_->log_level);
    }
}

void PcLog::pc_log_level_down()
{
    if (log_->log_level > LOG_LEVEL_ERR) {
        ++(log_->log_level);
    }
}

bool PcLog::pc_log_able(int log_level)
{
    return log_level > log_->log_level ? false : true;
}

void PcLog::log_write(const int log_level, const char *file, const int line,
                      const char *func, const char *fmt, ...)
{
    // 这种场景，几乎不会发生
    if (nullptr == log_buffer_) {
        return;
    }
    if (log_->log_fd < 0) {
        return;
    }
    va_list args;
    struct tm local_tm;
    struct timeval tv;
    
    gettimeofday(&tv, NULL);

    // 多协程中暂定使用信号，信号+锁 易存在死锁
    // TODO
    localtime_r(&tv.tv_sec, &local_tm);

    int level = MAX(LOG_LEVEL_ERR, MIN(log_level, LOG_LEVEL_FULL));

    const char *p = strrchr(file, '/');
    if (nullptr == p) {
        p = file;
    } else {
        ++p;
    }

    int len = snprintf(log_buffer_, LOG_MAX_LEN, "[%04d-%02d-%02d %02d:%02d:%02d.%06lu][%d]|"
            "%s|%s:%d:(%s):", local_tm.tm_year+1900, local_tm.tm_mon+1, local_tm.tm_mday,
            local_tm.tm_hour, local_tm.tm_min, local_tm.tm_sec, tv.tv_usec, getpid(),
            pc_log_level_str[level], p, line, func);

    if (len < LOG_MAX_LEN - 1) {
        va_start(args, fmt);
        len += vsnprintf(log_buffer_+len, LOG_MAX_LEN-len, fmt, args);
        va_end(args);
    } else {
        len = LOG_MAX_LEN - 1;
    }
    log_buffer_[len++] = '\n';
    len = write(log_->log_fd, log_buffer_, len);

    (void)len;
}
