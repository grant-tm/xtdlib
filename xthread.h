#ifndef X_THREAD_H
#define X_THREAD_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <avrt.h>

#include "xmath.h"

// -- BASIC THREAD ------------------------------------------------------------

#define THREAD_CREATE_ERROR -1
#define THREAD_CREATE_OK 0

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

static inline i32 ThreadLaunch(Thread *t, FunctionPointer f, void *arg) {
    ThreadFunction *routine = (ThreadFunction *) malloc(sizeof(ThreadFunction));
    if (!routine) {
        MessageBoxW(NULL, L"Failed to allocate memory for thread routine\n", L"Error", MB_OK);
        return THREAD_CREATE_ERROR;
    };
    routine->name = f;
    routine->arg = arg;
    
    HANDLE threadHandle = CreateThread(NULL, 0, _ThreadLauncher, routine, 0, NULL);
    if (!threadHandle) {
        MessageBoxW(NULL, L"Failed to create thread!", L"Error", MB_OK);
        free(routine);
        return THREAD_CREATE_ERROR;
    }

    *t = threadHandle;
    return THREAD_CREATE_OK;
}

static inline int ThreadJoin(Thread *t) {
    if (!t || !*t) return -1;
    DWORD ret = WaitForSingleObject(*t, INFINITE);
    BOOL okClose = CloseHandle(*t);
    *t = NULL;
    return (ret == WAIT_OBJECT_0 && okClose) ? 0 : -1;
}

// -- MUTEX -------------------------------------------------------------------

typedef CRITICAL_SECTION Mutex;
// takes Mutex *
#define MutexInit(mutex) InitializeCriticalSection(mutex)
#define MutexDestroy(mutex) DeleteCriticalSection(mutex)
#define MutexLock(mutex) EnterCriticalSection(mutex)
#define MutexUnlock(mutex) LeaveCriticalSection(mutex)

// -- ATOMICS -----------------------------------------------------------------

#define AtomicBarrierFull() MemoryBarrier()
#define AtomicBarrierAcquire() _ReadBarrier()
#define AtomicBarrierRelease() _WriteBarrier()

#define AtomicLoad(type, ptr) (*(volatile (type))(ptr))
#define AtomicStore(type, ptr, val) (*(volatile type)(ptr) = (val))

#define AtomicCompEx32(ptr, expected, desired) \
    InterlockedCompareExchange((volatile i32*)(ptr), (i32)desired, (i32)expected)
#define AtomicCompEx64(ptr, expected, desired) \
    InterlockedCompareExchange64((volatile i64*)(ptr), (i64)desired, (i64)expected)
#define AtomicCompExPtr(ptr, expected, desired) \
    InterlockedCompareExchangePointer((void * volatile *)(ptr), (void *)desired, (void *)expected)

#define AtomicEx32(ptr, val) InterlockedExchange((volatile i32*)(ptr), (i32)val)
#define AtomicEx64(ptr, val) InterlockedExchange64((volatile i64*)(ptr), (i64)val)
#define AtomicExPtr(ptr, val) InterlockedExchangePointer((void * volatile *)(ptr), (void *) val)

#define AtomicFetchAdd32(ptr, val) InterlockedExchangeAdd((volatile i32*)(ptr), (i32)val)
#define AtomicFetchAdd64(ptr, val) InterlockedExchangeAdd64((volatile i64*)(ptr), (i64)val)
#define AtomicFetchSub32(ptr, val) InterlockekdExchangeSub((volatile i32*)(ptr), -(i32)val)

#define AtomicInc32(ptr, val) InterlockedIncrement((volatile i32*)(ptr))
#define AtomicInc64(ptr, val) InterlockedIncrement64((volatile i64*)(ptr))

#define AtomicDec32(ptr, val) InterlockedDecrement((volatile i32*)(ptr))
#define AtomicDec64(ptr, val) InterlockedDecrement64((volatile i64*)(ptr))

#define AtomicFetchOr32(ptr, mask) InterlockedOr((volatile i32*)(ptr), (i32)mask)
#define AtomicFetchOr64(ptr, mask) InterlockedOr64((volatile i64*)(ptr), (i64)mask)

#define AtomicFetchAnd32(ptr, mask) InterlockedAnd((volatile i32*)(ptr), (i32)mask)
#define AtomicFetchAnd64(ptr, mask) InterlockedAnd64((volatile i64*)(ptr), (i64)mask)

#define AtomicFetchXor32(ptr, mask) InterlockedXor((volatile i32*)(ptr), (i32)mask)
#define AtomicFetchXor64(ptr, mask) InterlockedXor64((volatile i64*)(ptr), (i64)mask)

// -- Shared Queue ------------------------------------------------------------

typedef struct {
    u8 *buffer;
    u64 capacity;
    volatile u64 head;
    u64 padding;
    volatile u64 tail;
} SharedQueue;

// -- Single Producer Single Consumer -----------------------------------------

static inline bool SPSCQueueInit(SharedQueue *q, u64 capacity_bytes) {
    if (!IsPowerOfTwo(capacity_bytes)) { return false; }

    q->buffer = (u8 *) malloc(capacity_bytes);
    if (!q->buffer) return false;
    
    q->capacity = capacity_bytes;
    q->head = 0;
    q->tail = 0;
    
    return true;
}

static inline void QueueDestroy(SharedQueue *q) {
    free(q->buffer);
    q->buffer = NULL;
    q->capacity = 0;
    q->head = q->tail = 0;
}

bool SPSCQueuePush (SharedQueue *q, const void *data, u64 size) {
    u64 head = q->head;
    u64 tail = q->tail;
    u64 capacity = q->capacity;

    if (tail - head >= capacity) {
        return false;
    }

    u64 position = tail & (capacity - 1);
    u64 first_chunk = Min(capacity - position, size);

    memcpy(q->buffer + position, data, first_chunk);
    memcpy(q->buffer, (u8 *)data + first_chunk, size - first_chunk);

    AtomicBarrierRelease();
    q->tail = tail + size;
    return true;
}

bool SPSCQueuePop (SharedQueue *q, void *dest, u64 size) {
    u64 head = q->head;
    u64 tail = q->tail;
    u64 capacity = q->capacity;

    if (tail - head < size) {
        return false;
    }

    AtomicBarrierAcquire();
    u64 position = head & (capacity - 1);
    u64 first_chunk = Min(capacity - position, size);

    memcpy(dest, q->buffer + position, first_chunk);
    memcpy((u8 *) dest + first_chunk, q->buffer, size - first_chunk);

    q->head = head + size;
    return true;
}

// -- Multiple Producer Single Consumer (TODO) --------------------------------
bool MPSCQueuePush (SharedQueue *q, const void *data, u64 size);
bool MPSCQueuePop (SharedQueue *q, void *data, u64 size);

// -- Single Producer Multiple Consumer (TODO) --------------------------------
bool SPMCQueuePush (SharedQueue *q, const void *data, u64 size);
bool SPMCQueuePop (SharedQueue *q, void *data, u64 size);

// -- Multiple Producer Multiple Consumer (TODO) ------------------------------
bool MPMCQueuePush (SharedQueue *q, const void *data, u64 size);
bool MPMCQueuePop (SharedQueue *q, void *data, u64 size);

#endif // X_THREAD_H