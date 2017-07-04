/*
 * FileName : pc_logger.h
 * Author   : Pengcheng Liu(Lpc-Win32)
 * Date     : Fri 30 Jun 2017 11:54:36 AM CST   Created
*/

#pragma once

#include <string>
#include <stdlib.h>

#if __cplusplus < 201103L
    #error "complile pc_logger should use C++11 standard"
#endif

enum PcLogLevels {
    LOG_LEVEL_FATAL,
    LOG_LEVEL_ERR,
    LOG_LEVEL_WARN,
    LOG_LEVEL_NOTICE,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_FULL
};

typedef struct logger_t {
    int   log_fd;
    int   log_level;
    char *log_name;
} logger_t;


class PcLog {
    public:
        ~PcLog();

        PcLog(const PcLog &othrer) = delete;

        PcLog *operator =(const PcLog &other) = delete;

        static PcLog *get_instance();

        void init(char *log_path, int level = LOG_LEVEL_WARN);

        void pc_log_reopen();
        void pc_log_level_up();
        void pc_log_level_down();
        void pc_log_level_set(int log_level);
        bool pc_log_able(int log_level);

        void log_write(const int log_level, const char *file, const int line,
                       const char *func, const char *fmt, ...);
    private:
        PcLog() noexcept;

        inline void pc_log_write(const char *msg, ...);

        logger_t *log_;
        static PcLog *pc_log_instance_;

        char *log_buffer_;       // used to store logs
};

#define pc_log_fatal(fmt, ...) \
    do{ \
        if(PcLog::get_instance()->pc_log_able(LOG_LEVEL_FATAL)){ \
            PcLog::get_instance()->log_write(LOG_LEVEL_FATAL, __FILE__, __LINE__, __FUNCTION__, \
                fmt, ## __VA_ARGS__); \
        } \
        exit(-1); \
    } while(false)

#define pc_log_error(fmt, ...) \
    do{ \
        if(PcLog::get_instance()->pc_log_able(LOG_LEVEL_ERR)){ \
            PcLog::get_instance()->log_write(LOG_LEVEL_ERR, __FILE__, __LINE__, __FUNCTION__, \
                fmt, ## __VA_ARGS__); \
        } \
    } while(false)

#define pc_log_warn(fmt, ...) \
    do{ \
        if(PcLog::get_instance()->pc_log_able(LOG_LEVEL_WARN)){ \
            PcLog::get_instance()->log_write(LOG_LEVEL_WARN, __FILE__, __LINE__, __FUNCTION__, \
                fmt, ## __VA_ARGS__); \
        } \
    } while(false)

#define pc_log_notice(fmt, ...) \
    do{ \
        if(PcLog::get_instance()->pc_log_able(LOG_LEVEL_NOTICE)){ \
            PcLog::get_instance()->log_write(LOG_LEVEL_NOTICE, __FILE__, __LINE__, __FUNCTION__, \
                fmt, ## __VA_ARGS__); \
        } \
    } while(false)

#define pc_log_info(fmt, ...) \
    do{ \
        if(PcLog::get_instance()->pc_log_able(LOG_LEVEL_INFO)){ \
            PcLog::get_instance()->log_write(LOG_LEVEL_INFO, __FILE__, __LINE__, __FUNCTION__, \
                fmt, ## __VA_ARGS__); \
        } \
    } while(false)

#define pc_log_debug(fmt, ...) \
    do{ \
        if(PcLog::get_instance()->pc_log_able(LOG_LEVEL_DEBUG)){ \
            PcLog::get_instance()->log_write(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __FUNCTION__, \
                fmt, ## __VA_ARGS__); \
        } \
    } while(false)

#define pc_log_full(fmt, ...) \
    do{ \
        if(PcLog::get_instance()->pc_log_able(LOG_LEVEL_FULL)){ \
            PcLog::get_instance()->log_write(LOG_LEVEL_FULL, __FILE__, __LINE__, __FUNCTION__, \
                fmt, ## __VA_ARGS__); \
        } \
    } while(false)
