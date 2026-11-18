#pragma once
#undef BOOL
#include <windows.h>
#include <time.h>
#include <stdlib.h>

typedef HANDLE pthread_t;
typedef SIZE_T pthread_attr_t;
typedef SRWLOCK pthread_mutex_t;
typedef CONDITION_VARIABLE pthread_cond_t;

#define PTHREAD_MUTEX_INITIALIZER SRWLOCK_INIT
#define PTHREAD_COND_INITIALIZER CONDITION_VARIABLE_INIT
#define pthread_mutex_init(mutex, attr) InitializeSRWLock(mutex)
#define pthread_mutex_lock AcquireSRWLockExclusive
#define pthread_mutex_unlock ReleaseSRWLockExclusive
#define pthread_cond_init(cond, attr) InitializeConditionVariable(cond)
#define pthread_cond_broadcast WakeAllConditionVariable
#define pthread_cond_signal WakeConditionVariable
#define pthread_cond_wait(cond, mutex) SleepConditionVariableSRW(cond, mutex, INFINITE, 0)
#define pthread_self GetCurrentThreadId
#define pthread_detach CloseHandle
#define pthread_attr_init(attr) (*(attr) = 0)
#define pthread_attr_destroy(attr) ((void)0)
#define clock_gettime(clock, result) timespec_get(result, TIME_UTC)

/* The desktop scheduler requests multi-megabyte stacks for every N64 thread.
 * nxdk reserves their backing pages on a console with 64 MiB of shared RAM. */
static inline int pthread_attr_setstacksize(pthread_attr_t *attr, size_t size)
{
    *attr = size < 256 * 1024 ? size : 256 * 1024;
    return 0;
}

static inline int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
                                         const struct timespec *deadline)
{
    struct timespec now;
    timespec_get(&now, TIME_UTC);
    long long ns = ((long long)deadline->tv_sec - now.tv_sec) * 1000000000LL
                   + deadline->tv_nsec - now.tv_nsec;
    DWORD timeout = ns > 0 ? (DWORD)((ns + 999999) / 1000000) : 0;
    return !SleepConditionVariableSRW(cond, mutex, timeout, 0);
}

struct xbox_thread_args {
    void *(*entry)(void *);
    void *argument;
};

static DWORD WINAPI xbox_thread_main(LPVOID opaque)
{
    struct xbox_thread_args args = *(struct xbox_thread_args *)opaque;
    free(opaque);
    args.entry(args.argument);
    return 0;
}

static inline int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                                 void *(*entry)(void *), void *argument)
{
    struct xbox_thread_args *args = malloc(sizeof(*args));
    if (!args) return -1;
    args->entry = entry;
    args->argument = argument;
    *thread = CreateThread(NULL, attr ? *attr : 128 * 1024, xbox_thread_main, args, 0, NULL);
    if (*thread) return 0;
    free(args);
    return -1;
}
