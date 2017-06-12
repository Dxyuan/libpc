/*
 * FileName : pc_routine_specific.h
 * Author   : Pengcheng Liu(Lpc-Win32)
 * Date     : 2017-05-18
*/

#pragma once
#include <pthread.h>
#include <stdlib.h>

extern int   pc_setspecific(pthread_key_t key, const void *value);
extern void *pc_getspecific(pthread_key_t key);

#define PC_ROUTINE_SPECIFIC(name, obj) \
\
static pthread_once_t _routine_once_##name = PTHREAD_ONCE_INIT; \
static pthread_key_t _routine_key_##name; \
static int _routine_init_##name = 0; \
static void _routine_make_key_##name() \
{\
    (void) pthread_key_create(&_routine_key_##name, NULL); \
}\
template <class T> \
class clsRoutineData_routine_##name \
{\
public: \
    inline T *operator->() \
    {\
        if (!_routine_init_##name) {\
            pthread_once(&_routine_once_##name, _routine_make_key_##name); \
            _routine_init_##name = 1; \
        }\
        T *p = (T *)pc_getspecific(_routine_key_##name); \
        if (!p) {\
            p = (T *)calloc(1, sizeof(T)); \
            int ret = pc_setspecific(_routine_key_##name, p); \
            if (ret) {\
                if (p) {\
                    free(p); \
                    p = NULL; \
                }\
            }\
        } \
        return p; \
    }\
}; \
\
static clsRoutineData_routine_##name<name> obj;
