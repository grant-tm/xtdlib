#ifndef XSTORAGE_H
#define XSTORAGE_H

#include "xstyle.h"
#include "xmemory.h"

// == Stack ===================================================================

/* -- USAGE ------------
struct Node {
    // Node Data
    Node *next;
};

// Stack
Node *first;
----------------------*/

#define StackPushN(first, node, next) \
    ((node->next) = (first), (first) = (node)) 

#define StackPopN(first, next) \
    ((first) = (first)->next)

#define StackPush(first, node) StackPushN(first, node, next)

#define StackPop(first) StackPopN(first, next)

// == Queue ===================================================================

/* -- USAGE ------------
struct Node {
    // Node Data
    Node *next;
};

// Queue
Node *first;
Node *last;
----------------------*/

#define QueuePushN(first, last, node, next) ((first) == NULL ?                \
    ((first) = (last) = (node), (node)->next = NULL) :                        \
    ((last)->next = (node), (last) = (node), (node)->next = NULL))

#define QueuePopN(first, last, next) (((first) == NULL) ? NULL :             \
    ((first) == (last)) ? ((last) = NULL, (first) = NULL) :                  \
    ((first) = (first)->next))

#define QueuePush(first, last, node) QueuePushN(first, last, node, next)

#define QueuePop (first, last) QueuePopN(first, last, next)

// == Doubly Linked List ======================================================

/* -- USAGE ------------
struct Node {
    // Node Data
    Node *next;
    Node *prev;
};

// Doubly Linked List
Node *first;
Node *last;
----------------------*/

#define DLLPushBackNP(first, last, node, next, prev) ((first) == NULL) ?        \
    ((first) = node, (last) = node, (node)->next = NULL, (node)->prev = NULL) : \
    ((node)->prev = last, (node)->next = NULL,                                  \
    (last)->next = (node), (last) = (node))
#define DLLPushBack(first, last, node) \
    DLLPushBackNP(first, last, node, next, prev)


#define DLLPushFrontNP(first, last, node, next, prev) ((first) == NULL) ?        \
    ((first) = node, (last) = node, (node)->next = NULL, (node)->prev = NULL) :  \
    ((node)->next = first, (node)->prev = NULL,                                  \
    (first)->prev = (node), (first) = (node))
#define DLLPushFront(first, last, node) \
    DLLPushFrontNP(first, last, node, next, prev)


// if refNode is NULL, insert at end of linked list
#define DLLInsertAfterNP(first, last, refNode, node, next, prev) \
    ((refNode) == NULL || (refNode == last)) ?                   \
    DLLPushBackNP(first, last, node, next, prev) :               \
    ((node)->next = (refNode)->next, (node)->prev = (refNode),   \
    (refNode)->next->prev = (node), (refNode)->next = (node))
#define DLLInsertAfter(first, last, refNode, node) \
    DLLInsertAfterNP(first, last, refNode, node, next, prev)


// if refNode is NULL, insert at beginning of linked list
#define DLLInsertBeforeNP(first, last, refNode, node, next, prev) \
    ((refNode) == NULL || (refNode == first)) ?                   \
    DLLPushFrontNP(first, last, node, next, prev) :               \
    ((node)->prev = (refNode)->prev, (node)->next = (refNode),    \
    (refNode)->prev->next = (node), (refNode)->prev = (node))
#define DLLInsertBefore(first, last, refNode, node) \
    DLLInsertBeforeNP(first, last, refNode, node, next, prev)


#define DLLRemoveNP(first, last, node, next, prev)                \
    do {                                                          \
        if ((node) == (first)) (first) = (node)->next;            \
        if ((node) == (last))  (last) = (node)->prev;             \
        if ((node)->prev)      (node)->prev->next = (node)->next; \
        if ((node)->next)      (node)->next->prev = (node)->prev; \
        (node)->next = (node)->prev = NULL;                       \
    } while (0)
#define DLLRemove(first, last, node) \
    DLLRemoveNP(first, last, node, next, prev)

// == Array ===================================================================

#define _ArrayHeader_ struct { u64 count; u64 capacity; }
typedef struct ArrayHeader { u64 count; u64 capacity; } ArrayHeader;

#define ArrayHeaderCast(a) ((ArrayHeader *)&(a))
#define ArrayItemSize(a)   (sizeof(*(a).v))

void *ArrayGrow (Arena *arena, ArrayHeader *header, void *array, u64 itemSize, u64 count, b32 clearToZero) {
    u64 prevCount = header->count;
    u64 prevCapacity = header->capacity;
    u64 targetCount = prevCount + count;

    if (targetCount <= prevCapacity) {
        return array;
    }

    u64 newCapacity = prevCapacity ? prevCapacity * 2 : 16;
    while (newCapacity < targetCount) {
        newCapacity *= 2;
    }

    u64 totalOldBytes = prevCapacity * itemSize;
    u64 totalNewBytes = newCapacity * itemSize;

    void *newArray = _ArenaPush(arena, totalNewBytes, alignof(u8), false);
    if (!newArray) return NULL;

    if (array) {
        memcpy(newArray, array, totalOldBytes);
    }

    if (clearToZero) {
        u64 newPartOffset = prevCapacity * itemSize;
        ZeroMemory((u8 *)newArray + newPartOffset, totalNewBytes - newPartOffset);
    }

    header->capacity = newCapacity;
    return newArray;
}


void ArrayShift(ArrayHeader *header, void *array, u64 itemSize, u64 fromIndex);

#define ArrayPush(arena, a, value) (*((void **)&(a).v) =                       \
    ArrayGrow((arena), ArrayHeaderCast(a), (a).v, ArrayItemSize(a), 1, false), \
    (a).v[(a).count++] = (value))   

#define ArrayAdd(arena, a, n) (*((void **)&(a).v) =                              \
    ArrayGrow((arena), ArrayHeaderCast(a), (a).v, ArrayItemSize(a), (n), false), \
    (a).count += (n), &(a).v[(a).count - (n)])

#define ArrayAddClear(arena, a, n) (*((void **)&(a).v) =                        \
    ArrayGrow((arena), ArrayHeaderCast(a), (a).v, ArrayItemSize(a), (n), true), \
    (a).count += (n), &(a).v[(a).count - (n)])

#define ArrayReserve(arena, a, n) (((n) > (a).capacity) ? (*((void **)&(a).v) =                   \
    ArrayGrow((arena), ArrayHeaderCast(a), (a).v, ArrayItemSize(a), (n) - (a).capacity, false)) : \
    (a).v)

#define ArrayInsert(arena, a, i, value) (*((void **)&(a).v) =                  \
    ArrayGrow((arena), ArrayHeaderCast(a), (a).v, ArrayItemSize(a), 1, false), \
    ArrayShift(ArrayHeaderCast(a), (a).v, ArrayItemSize(a), (i)),              \
    (a).v[(i)] = (value), (a).count++)

#define ArrayRemove(a, i, n) (ArrayShift(ArrayHeaderCast(a), (a).v, ArrayItemSize(a), (i) + (n)), \
    (a).count -= (n))

#define ArrayClear(a) a.count = 0

// == Map =====================================================================

/* -- USAGE ------------

struct KVStrEmployee { String key; Employee *value; };
struct MapStrEmployee { _MapHeader_; KVStrEmployee *v; };

MapStrEmployee map = {0};

Employee *bob = ...;
MapsPut(arena, map, bob->name, bob);

Employee *bob = MapsGet(map, Employee *, S("Bob"));

MapDelete(scratch.arena, map, S("Bob"));
----------------------*/



/* -- USAGE ------------

Index index = MapsIndex(map, S("Bob"));
if (index.exists) {
    i32 at = index.value;
    Employee *bob = map.v[at].value;
}

for (i32 i = 0; i < map.count; ++i) {
    Employee *empoyee = map.v[i].value;
}

ArenaRelease(&arena);
----------------------*/

/*
struct MapHeader
{
    u64 count;
    u64 capacity;
    u64 insertIndex;
    u64 usedCountThreshold;
    u64 deletedCount;
    u64 deletedCountThreshold
    MapHashEntry *table;
};
struct MapHashEntry { u64 hash; u64 index; };

#define MapReserve(arena, map, size)
#define MapDefault(arena, map, value)
#define MapDefaultS(arena, map, string)
#define MapClear(map)
#define MapClone(arena, map, result)

#define MapPut(arena, map, k, value)
#define MapIndex(map, k)
#define MapGet(map, type, k)
#define MapTryGet(map, k, outVal)
#define MapGetS(map, type, k)
#define MapGetPtr(map, k)
#define MapGetPtrNull(map, k)
#define MapDelete(scratch, map, k)

#define MapsPut(arena, map, k, value)
#define MapsPutS(arena, m, s)
#define MapsIndex(map, k)
#define MapsGet(map, type, k)
#define MapsTryGet(map, k, outVal)
#define MapsGetS(map, type, k)
#define MapsGetPtr(map, k)
#define MapsGetPtrNull(map, k)
#define MapsDelete(scratch, map, k)
*/

#endif // XSTORAGE_H