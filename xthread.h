#ifndef X_THREAD_H
#define X_THREAD_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <avrt.h>

#define THREAD_CREATE_ERROR -1

typedef HANDLE Thread;

typedef void (*FunctionPointer)(void *arg);

typedef struct {
    FunctionPointer name;
    void *arg;
} ThreadFunction;

static DWORD WINAPI _ThreadLauncher (LPVOID param) {
    ThreadFunction routine = *(ThreadFunction *) param;
    free(param);
    routine.name(routine.arg);
    return 0;
}

static inline i32 ThreadLaunch (Thread *t, FunctionPointer f, void *arg) {
    
    ThreadFunction *routine = (ThreadFunction *) malloc(sizeof(ThreadFunction));
    if (!routine) return -1;
    routine->name = f;
    routine->arg = arg;
    
    HANDLE threadHandle = CreateThread(NULL, 0, _ThreadLauncher, routine, 0, NULL);
    
    if (!threadHandle) {
        free(routine);
        return THREAD_CREATE_ERROR;
    }

    *t = threadHandle;
    return 0;
}

static inline i32 ThreadJoin (Thread *t) {
    DWORD ret = WaitForSingleObject(t, INFINITE);
    CloseHandle(t);
    return (ret == WAIT_OBJECT_0) ? 0 : -1;
}

typedef CRITICAL_SECTION Mutex;

// takes Mutex *
#define MutexInit(mutex) InitializeCriticalSection(mutex)
#define MutexDestroy(mutex) DeleteCriticalSection(mutex)
#define MutexLock(mutex) EnterCriticalSection(mutex)
#define MutexUnlock(mutex) LeaveCriticalSection(mutex)

#endif // X_THREAD_H