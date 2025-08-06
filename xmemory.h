#ifndef XMEMORY_H
#define XMEMORY_H

#include <windows.h>
#include <string.h>
#include <stddef.h>

#include "xstyle.h"
#include "xbinary.h"

#define CommitChunkSize ((u64)64 * (u64)1024)

#ifndef DefaultAlignment
#define DefaultAlignment 8
#endif

#ifndef alignof
    #if defined(_MSC_VER)
        #define alignof(type) __alignof(type)
    #elif defined(__GNUC__) || defined(__clang__)
        #define alignof(type) __alignof__(type)
    #else
        #define alignof(type) offsetof(struct { char c; type member; }, member)
    #endif
#endif

void foo (void) {}

static inline u64 AlignForward (u64 ptr, u64 alignment) {
    u64 modulo = ptr % alignment;
    if (modulo != 0) {
        ptr += (alignment - modulo);
    }
    return ptr;
}

void *ReserveMemory(u64 size) {
    return VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_NOACCESS);
}

void ReleaseMemory(void *ptr) {
    VirtualFree(ptr, 0, MEM_RELEASE);
}

typedef struct Arena {
    u64 reserved;
    u64 committed;
    u64 used;
    void *base;
} Arena;

b32 ArenaCommitMemory (Arena *arena, u64 size) {

    u64 commitSize = AlignForward(size, (u64) CommitChunkSize);
    void *commitPtr = (u8 *)arena->base + arena->committed;
    void *result = VirtualAlloc(commitPtr, commitSize, MEM_COMMIT, PAGE_READWRITE);
    
    if (result)
    {
        arena->committed += commitSize;
        return true;
    }

    return false;
}

void *_ArenaPush (Arena *arena, u64 size, u64 alignment, b32 clearToZero) {
    
    u64 currentPtr = (u64) arena->base + arena->used;
    u64 alignedPtr = AlignForward(currentPtr, alignment);
    u64 padding = alignedPtr - currentPtr;
    u64 totalSize = padding + size;

    if (arena->used + totalSize > arena->committed) {
        b32 newCommitted = ArenaCommitMemory(arena, totalSize);
        if (!newCommitted)
            return NULL;
    }

    void *result = (void *)alignedPtr;
    if (clearToZero) {
        ZeroMemory(result, size);
    }

    arena->used = (alignedPtr + size) - (u64)arena->base;

    return result;
}

#define ArenaPush(arena, size) _ArenaPush(arena, size, DefaultAlignment, true)

#define ArenaPushStruct(arena, type) (type *)_ArenaPush(arena, sizeof(type), alignof(type), true)

#define ArenaPushArray(arena, count, type) (type *)_ArenaPush(arena, (count) * sizeof(type), alignof(type), true)

#define ArenaClear(arena) (arena)->used = 0

#define ArenaRelease(arena)                              \
    do {                                                 \
        ReleaseMemory((arena)->base); \
        (arena)->base = NULL;                            \
        (arena)->used = 0;                               \
        (arena)->committed = 0;                          \
        (arena)->reserved = 0;                           \
    } while(0)

#endif // XMEMORY_H