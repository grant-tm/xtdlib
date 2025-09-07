#define XTD_TEST_LOGGING_VERBOSITY TEST_LOGGING_VERBOSITY_HIGH
#define XTDLIB_IMPLEMENTATION
#include "xtdlib.h"

//=============================================================================
// Memory Module
//=============================================================================

// -- Arena Component ---------------------------------------------------------

TEST(ArenaPush, "Memory", "Arena") {
	Arena arena = {0};
    u64 push_size = 128;
    void *p = arena_push(&arena, push_size);

    if (p == NULL) {
        test_case_record_error(&ArenaPushTest, TEST_RESULT_NULL_VALUE,
            "arena_push returned NULL",
            "\t-- Requested size: %llu", push_size);
    }
    if (arena.used != push_size) {
        test_case_record_error(&ArenaPushTest, TEST_RESULT_INCORRECT_VALUE,
            "arena.used did not match push_size",
            "\t-- Expected: %llu\n\t-- Actual: %llu", push_size, arena.used);
    }
    if (arena.reserved != ARENA_DEFAULT_RESERVE_SIZE) {
        test_case_record_error(&ArenaPushTest, TEST_RESULT_INVALID_STATE,
            "arena.reserved mismatch",
            "\t-- Expected: %llu\n\t-- Actual: %llu",
            (u64)ARENA_DEFAULT_RESERVE_SIZE, arena.reserved);
    }
    return 0;
}

TEST(ArenaPushStruct, "Memory", "Arena") {
	Arena arena = {0};
    typedef struct { int id; float value; } TestStruct;

    u64 arena_used_prev = arena.used;
    TestStruct *s = arena_push_struct(&arena, TestStruct);

    if (s == NULL) {
        test_case_record_error(&ArenaPushStructTest, TEST_RESULT_NULL_VALUE,
            "arena_push_struct returned NULL", "");
    }
    if (arena.used != arena_used_prev + sizeof(*s)) {
        test_case_record_error(&ArenaPushStructTest, TEST_RESULT_INCORRECT_VALUE,
            "arena.used incorrect after struct push",
            "\t-- Expected: %llu\n\t-- Actual: %llu",
            arena_used_prev + sizeof(*s), arena.used);
    }

    s->id = 42; s->value = 3.14f;
    if (s->id != 42 || s->value != 3.14f) {
        test_case_record_error(&ArenaPushStructTest, TEST_RESULT_INCORRECT_VALUE,
            "Struct members incorrect after push",
            "\t-- Expected: id=42, value=3.14\n\t-- Actual: id=%d, value=%f",
            s->id, s->value);
    }
    return 0;
}

TEST(ArenaPushArray, "Memory", "Arena") {
	Arena arena = {0};
    int *arr = arena_push_array(&arena, 10, int);

    if (arr == NULL) {
        test_case_record_error(&ArenaPushArrayTest, TEST_RESULT_NULL_VALUE,
            "arena_push_array returned NULL", "");
    }
    for (int i = 0; i < 10; i++) {
        arr[i] = i * i;
        if (arr[i] != i * i) {
            test_case_record_error(&ArenaPushArrayTest, TEST_RESULT_INCORRECT_VALUE,
                "Array value mismatch",
                "\t-- Index %d: Expected %d, Actual %d\n",
                i, i*i, arr[i]);
        }
    }
    return 0;
}

TEST(ArenaAlignment, "Memory", "Arena") {
	Arena arena = {0};
    void *p = _arena_push(&arena, 16, 64, true);

    if (((uintptr_t)p % 64) != 0) {
        test_case_record_error(&ArenaAlignmentTest, TEST_RESULT_INCORRECT_VALUE,
            "Alignment not satisfied",
            "\t-- Expected alignment: %d\n\t-- Actual address: %p",
            64, p);
    }
    return 0;
}

TEST(ArenaCommitMemory, "Memory", "Arena") {
	Arena arena = {0};
    u64 prev_committed = arena.committed;
    void *p = arena_push(&arena, ARENA_COMMIT_CHUNK_SIZE * 2);

    if (p == NULL) {
        test_case_record_error(&ArenaCommitMemoryTest, TEST_RESULT_NULL_VALUE,
            "Large push returned NULL", "");
    }
    if (arena.committed <= prev_committed) {
        test_case_record_error(&ArenaCommitMemoryTest, TEST_RESULT_INVALID_STATE,
            "arena.committed did not increase",
            "\t-- Previous: %llu\n\t-- Current: %llu",
            prev_committed, arena.committed);
    }
    return 0;
}

TEST(ArenaZeroClear, "Memory", "Arena") {
	Arena arena = {0};
    u32 *vals = arena_push_array(&arena, 4, u32);
    for (int i = 0; i < 4; i++) {
        if (vals[i] != 0) {
            test_case_record_error(&ArenaZeroClearTest, TEST_RESULT_INCORRECT_VALUE,
                "Memory not zeroed",
                "\t-- Index %d: Expected 0, Actual %u\n", i, vals[i]);
        }
    }
    return 0;
}

TEST(ArenaNoClear, "Memory", "Arena") {
	Arena arena = {0};
    u64 prev_used = arena.used;
    u8 *vals = (u8 *)_arena_push(&arena, 16, alignof(u8), false);

    for (int i = 0; i < 16; i++) vals[i] = 0xAA;

    arena.used = prev_used;
    u8 *vals2 = (u8 *)_arena_push(&arena, 16, alignof(u8), false);

    bool all_zero = true;
    for (int i = 0; i < 16; i++) if (vals2[i] != 0) { all_zero = false; break; }

    if (all_zero) {
        test_case_record_error(&ArenaNoClearTest, TEST_RESULT_INCORRECT_VALUE,
            "Memory unexpectedly zeroed", "");
    }
    return 0;
}

TEST(ArenaClear, "Memory", "Arena") {
	Arena arena = {0};
	void *p = arena_push(&arena, 1024);
	xtd_ignore_unused(p);
    u64 reserved_prev = arena.reserved;
    arena_clear(&arena);

    if (arena.reserved != reserved_prev) {
        test_case_record_error(&ArenaClearTest, TEST_RESULT_INVALID_STATE,
            "Reserved changed after clear",
            "\t-- Expected: %llu\n\t-- Actual: %llu",
            reserved_prev, arena.reserved);
    }
    if (arena.used != 0) {
        test_case_record_error(&ArenaClearTest, TEST_RESULT_INVALID_STATE,
            "Used not reset after clear",
            "\t-- Expected: 0\n\t-- Actual: %llu", arena.used);
    }
    return 0;
}

TEST(ArenaExactFit, "Memory", "Arena") {
	Arena arena = {0};
	_arena_init(&arena);
    arena_clear(&arena);
    arena_commit_memory(&arena, ARENA_COMMIT_CHUNK_SIZE);

    void *p = _arena_push(&arena, ARENA_COMMIT_CHUNK_SIZE, 8, true);
    if (p == NULL) {
        test_case_record_error(&ArenaExactFitTest, TEST_RESULT_NULL_VALUE,
            "arena_push returned NULL", "");
    }
    if (arena.used != ARENA_COMMIT_CHUNK_SIZE) {
        test_case_record_error(&ArenaExactFitTest, TEST_RESULT_INCORRECT_VALUE,
            "arena.used mismatch",
            "\t-- Expected: %llu\n\t-- Actual: %llu",
            (u64)ARENA_COMMIT_CHUNK_SIZE, arena.used);
    }
    return 0;
}

TEST(ArenaRelease, "Memory", "Arena") {
	Arena arena = {0};
	void *p = arena_push(&arena, 1024);
	xtd_ignore_unused(p);
    arena_release(&arena);

    if (arena.used != 0) {
        test_case_record_error(&ArenaReleaseTest, TEST_RESULT_INVALID_STATE,
            "arena.used not zero after release",
            "\t-- Expected: 0\n\t-- Actual: %llu", arena.used);
    }
    if (arena.reserved != 0) {
        test_case_record_error(&ArenaReleaseTest, TEST_RESULT_INVALID_STATE,
            "arena.reserved not zero after release",
            "\t-- Expected: 0\n\t-- Actual: %llu", arena.reserved);
    }
    if (arena.base != NULL) {
        test_case_record_error(&ArenaReleaseTest, TEST_RESULT_INVALID_STATE,
            "arena.base not NULL after release",
            "\t-- Expected: NULL\n\t-- Actual: %p", arena.base);
    }
    return 0;
}

TEST(ArenaPushAfterRelease, "Memory", "Arena") {
	Arena arena = {0};
    arena_release(&arena);

    void *p = arena_push(&arena, 64);
    if (p == NULL) {
        test_case_record_error(&ArenaPushAfterReleaseTest, TEST_RESULT_NULL_VALUE,
            "arena_push after release returned NULL", "");
    }
    if (arena.base == NULL) {
        test_case_record_error(&ArenaPushAfterReleaseTest, TEST_RESULT_INVALID_STATE,
            "arena.base not re-initialized", "");
    }
    return 0;
}

//=============================================================================
// Storage Module
//=============================================================================

// -- Stack Component ---------------------------------------------------------

typedef struct IdiomaticStackNode {
	int data;
	struct IdiomaticStackNode *next;
} IdiomaticStackNode;

typedef struct CustomStackNode {
	int data;
	struct CustomStackNode *n;
} CustomStackNode;

TEST(StackPush, "Storage", "Stack") {
    Arena arena = {0};

    IdiomaticStackNode *stack = NULL;
    IdiomaticStackNode *new_node = arena_push_struct(&arena, IdiomaticStackNode);

    if (new_node == NULL) {
        test_case_record_error(&StackPushTest, TEST_RESULT_NULL_VALUE,
            "stack_push new node was NULL", "");
    }
    new_node->data = 1;
    new_node->next = NULL;

    stack_push(stack, new_node);

    if (stack == NULL) {
        test_case_record_error(&StackPushTest, TEST_RESULT_INVALID_STATE,
            "Stack is NULL after push", "");
    }
    if (stack != new_node) {
        test_case_record_error(&StackPushTest, TEST_RESULT_INVALID_STATE,
            "Stack pointer not updated to new node",
            "\t-- Expected: %p\n\t-- Actual: %p", new_node, stack);
    }
    return 0;
}

TEST(StackPop, "Storage", "Stack") {
    Arena arena = {0};
    IdiomaticStackNode *stack = NULL;
    IdiomaticStackNode *n = arena_push_struct(&arena, IdiomaticStackNode);
    n->data = 42; n->next = NULL;
    stack_push(stack, n);

    stack_pop(stack);

    if (stack != NULL) {
        test_case_record_error(&StackPopTest, TEST_RESULT_INVALID_STATE,
            "Stack not NULL after popping only node", "");
    }
    return 0;
}

TEST(StackLIFO, "Storage", "Stack") {
    Arena arena = {0};
    IdiomaticStackNode *stack = NULL;
    IdiomaticStackNode *a = arena_push_struct(&arena, IdiomaticStackNode);
    IdiomaticStackNode *b = arena_push_struct(&arena, IdiomaticStackNode);
    IdiomaticStackNode *c = arena_push_struct(&arena, IdiomaticStackNode);
    a->data = 1; b->data = 2; c->data = 3;
    a->next = b->next = c->next = NULL;

    stack_push(stack, a);
    stack_push(stack, b);
    stack_push(stack, c);

    if (stack->data != 3) {
        test_case_record_error(&StackLIFOTest, TEST_RESULT_INCORRECT_VALUE,
            "Top of stack incorrect after pushes",
            "\t-- Expected: 3\n\t-- Actual: %d", stack->data);
    }
    stack_pop(stack);
    if (stack->data != 2) {
        test_case_record_error(&StackLIFOTest, TEST_RESULT_INCORRECT_VALUE,
            "Expected 2 after popping 3",
            "\t-- Actual: %d", stack->data);
    }
    stack_pop(stack);
    if (stack->data != 1) {
        test_case_record_error(&StackLIFOTest, TEST_RESULT_INCORRECT_VALUE,
            "Expected 1 after popping 2",
            "\t-- Actual: %d", stack->data);
    }
    stack_pop(stack);
    if (stack != NULL) {
        test_case_record_error(&StackLIFOTest, TEST_RESULT_INVALID_STATE,
            "Stack not empty after popping all elements", "");
    }
    return 0;
}

TEST(StackPopEmpty, "Storage", "Stack") {
    IdiomaticStackNode *stack = NULL;
    stack_pop(stack);

    if (stack != NULL) {
        test_case_record_error(&StackPopEmptyTest, TEST_RESULT_INVALID_STATE,
            "Pop on empty stack modified pointer", "");
    }
    return 0;
}

TEST(StackInterleaved, "Storage", "Stack") {
    Arena arena = {0};
    IdiomaticStackNode *stack = NULL;
    IdiomaticStackNode *x = arena_push_struct(&arena, IdiomaticStackNode);
    IdiomaticStackNode *y = arena_push_struct(&arena, IdiomaticStackNode);
    IdiomaticStackNode *z = arena_push_struct(&arena, IdiomaticStackNode);
    x->data = 1; y->data = 2; z->data = 3;
    x->next = y->next = z->next = NULL;

    stack_push(stack, x); // 1
    stack_push(stack, y); // 2,1
    stack_pop(stack);     // 1
    stack_push(stack, z); // 3,1

    if (stack->data != 3) {
        test_case_record_error(&StackInterleavedTest, TEST_RESULT_INCORRECT_VALUE,
            "Expected 3 at top", "\t-- Actual: %d", stack->data);
    }
    stack_pop(stack);
    if (stack->data != 1) {
        test_case_record_error(&StackInterleavedTest, TEST_RESULT_INCORRECT_VALUE,
            "Expected 1 after popping 3",
            "\t-- Actual: %d", stack->data);
    }
    stack_pop(stack);
    if (stack != NULL) {
        test_case_record_error(&StackInterleavedTest, TEST_RESULT_INVALID_STATE,
            "Stack not empty after sequence", "");
    }
    return 0;
}

TEST(StackPushN, "Storage", "Stack") {
    Arena arena = {0};
    CustomStackNode *stack = NULL;
    CustomStackNode *new_node = arena_push_struct(&arena, CustomStackNode);

    if (new_node == NULL) {
        test_case_record_error(&StackPushNTest, TEST_RESULT_NULL_VALUE,
            "stack_push_n new node was NULL", "");
    }
    new_node->data = 1;
    new_node->n = NULL;

    stack_push_n(stack, new_node, n);

    if (stack == NULL) {
        test_case_record_error(&StackPushNTest, TEST_RESULT_INVALID_STATE,
            "Stack is NULL after push_n", "");
    }
    if (stack != new_node) {
        test_case_record_error(&StackPushNTest, TEST_RESULT_INVALID_STATE,
            "Stack pointer not updated to new node (push_n)",
            "\t-- Expected: %p\n\t-- Actual: %p", new_node, stack);
    }
    return 0;
}

TEST(StackPopN, "Storage", "Stack") {
    Arena arena = {0};
    CustomStackNode *stack = NULL;
    CustomStackNode *n1 = arena_push_struct(&arena, CustomStackNode);
    n1->data = 99; n1->n = NULL;
    stack_push_n(stack, n1, n);

    stack_pop_n(stack, n);

    if (stack != NULL) {
        test_case_record_error(&StackPopNTest, TEST_RESULT_INVALID_STATE,
            "Stack not NULL after popping only node", "");
    }
    return 0;
}

TEST(StackLIFON, "Storage", "Stack") {
    Arena arena = {0};
    CustomStackNode *stack = NULL;
    CustomStackNode *a = arena_push_struct(&arena, CustomStackNode);
    CustomStackNode *b = arena_push_struct(&arena, CustomStackNode);
    CustomStackNode *c = arena_push_struct(&arena, CustomStackNode);
    a->data = 1; b->data = 2; c->data = 3;
    a->n = b->n = c->n = NULL;

    stack_push_n(stack, a, n);
    stack_push_n(stack, b, n);
    stack_push_n(stack, c, n);

    if (stack->data != 3) {
        test_case_record_error(&StackLIFONTest, TEST_RESULT_INCORRECT_VALUE,
            "Expected 3 at top", "\t-- Actual: %d", stack->data);
    }
    stack_pop_n(stack, n);
    if (stack->data != 2) {
        test_case_record_error(&StackLIFONTest, TEST_RESULT_INCORRECT_VALUE,
            "Expected 2 after popping 3", "\t-- Actual: %d", stack->data);
    }
    stack_pop_n(stack, n);
    if (stack->data != 1) {
        test_case_record_error(&StackLIFONTest, TEST_RESULT_INCORRECT_VALUE,
            "Expected 1 after popping 2", "\t-- Actual: %d", stack->data);
    }
    stack_pop_n(stack, n);
    if (stack != NULL) {
        test_case_record_error(&StackLIFONTest, TEST_RESULT_INVALID_STATE,
            "Stack not empty after popping all", "");
    }
    return 0;
}

TEST(StackPopEmptyN, "Storage", "Stack") {
    CustomStackNode *stack = NULL;
    stack_pop_n(stack, n);

    if (stack != NULL) {
        test_case_record_error(&StackPopEmptyNTest, TEST_RESULT_INVALID_STATE,
            "Pop_n on empty stack modified pointer", "");
    }
    return 0;
}

// -- Queue Component ---------------------------------------------------------

typedef struct IdiomaticQueueNode {
    int data;
    struct IdiomaticQueueNode *next;
	struct IdiomaticQueueNode *last;
} IdiomaticQueueNode;

typedef struct CustomQueueNode {
    int data;
    struct CustomQueueNode *n;    
} CustomQueueNode;

TEST(QueuePush, "Storage", "Queue") {
    Arena arena = {0};
    IdiomaticQueueNode *first = NULL;
    IdiomaticQueueNode *last  = NULL;

    IdiomaticQueueNode *new_node = arena_push_struct(&arena, IdiomaticQueueNode);
    if (!new_node) {
        test_case_record_error(&QueuePushTest, TEST_RESULT_NULL_VALUE,
            "New node allocation failed",
            "\t-- arena_push_struct returned NULL");
    } else {
        new_node->data = 1;
        new_node->next = NULL;
        queue_push(first, last, new_node);
        if (!(first == new_node && last == new_node)) {
            test_case_record_error(&QueuePushTest, TEST_RESULT_INVALID_STATE,
                "Incorrect first/last after push",
                "\t-- Expected: first=%p, last=%p\n\t-- Actual: first=%p, last=%p",
                (void*)new_node, (void*)new_node, (void*)first, (void*)last);
        }
    }

    arena_release(&arena);
    return 0;
}

TEST(QueuePop, "Storage", "Queue") {
    Arena arena = {0};
    IdiomaticQueueNode *first = NULL;
    IdiomaticQueueNode *last  = NULL;

    IdiomaticQueueNode *node = arena_push_struct(&arena, IdiomaticQueueNode);
    node->data = 1; node->next = NULL;
    queue_push(first, last, node);
    queue_pop(first, last);

    if (!(first == NULL && last == NULL)) {
        test_case_record_error(&QueuePopTest, TEST_RESULT_INVALID_STATE,
            "Queue not empty after popping only element",
            "\t-- Expected: first=NULL, last=NULL\n\t-- Actual: first=%p, last=%p",
            (void*)first, (void*)last);
    }

    arena_release(&arena);
    return 0;
}

TEST(QueueFIFO, "Storage", "Queue") {
    Arena arena = {0};
    IdiomaticQueueNode *first = NULL;
    IdiomaticQueueNode *last  = NULL;

    IdiomaticQueueNode *a = arena_push_struct(&arena, IdiomaticQueueNode);
    IdiomaticQueueNode *b = arena_push_struct(&arena, IdiomaticQueueNode);
    IdiomaticQueueNode *c = arena_push_struct(&arena, IdiomaticQueueNode);
    a->data = 1; b->data = 2; c->data = 3;
    a->next = b->next = c->next = NULL;

    queue_push(first, last, a);
    queue_push(first, last, b);
    queue_push(first, last, c);

    if (!(first->data == 1 && last->data == 3)) {
        test_case_record_error(&QueueFIFOTest, TEST_RESULT_INCORRECT_VALUE,
            "Wrong first/last after pushes",
            "\t-- Expected: first=1, last=3\n\t-- Actual: first=%d, last=%d",
            first->data, last->data);
    }

    queue_pop(first, last);
    if (first->data != 2) {
        test_case_record_error(&QueueFIFOTest, TEST_RESULT_INCORRECT_VALUE,
            "Wrong value after popping first",
            "\t-- Expected: 2\n\t-- Actual: %d", first->data);
    }
    queue_pop(first, last);
    if (first->data != 3) {
        test_case_record_error(&QueueFIFOTest, TEST_RESULT_INCORRECT_VALUE,
            "Wrong value after popping second",
            "\t-- Expected: 3\n\t-- Actual: %d", first->data);
    }
    queue_pop(first, last);
    if (!(first == NULL && last == NULL)) {
        test_case_record_error(&QueueFIFOTest, TEST_RESULT_INVALID_STATE,
            "Queue not empty at end",
            "\t-- Expected: first=NULL, last=NULL\n\t-- Actual: first=%p, last=%p",
            (void*)first, (void*)last);
    }

    arena_release(&arena);
    return 0;
}

TEST(QueuePopEmpty, "Storage", "Queue") {
    Arena arena = {0};
    IdiomaticQueueNode *first = NULL;
    IdiomaticQueueNode *last  = NULL;

    queue_pop(first, last);
    if (!(first == NULL && last == NULL)) {
        test_case_record_error(&QueuePopEmptyTest, TEST_RESULT_INVALID_STATE,
            "queue_pop on empty queue modified pointers",
            "\t-- Expected: first=NULL, last=NULL\n\t-- Actual: first=%p, last=%p",
            (void*)first, (void*)last);
    }

    arena_release(&arena);
    return 0;
}

TEST(QueueInterleaved, "Storage", "Queue") {
    Arena arena = {0};
    IdiomaticQueueNode *first = NULL;
    IdiomaticQueueNode *last  = NULL;

    IdiomaticQueueNode *x = arena_push_struct(&arena, IdiomaticQueueNode);
    IdiomaticQueueNode *y = arena_push_struct(&arena, IdiomaticQueueNode);
    IdiomaticQueueNode *z = arena_push_struct(&arena, IdiomaticQueueNode);
    x->data = 10; y->data = 20; z->data = 30;
    x->next = y->next = z->next = NULL;

    queue_push(first, last, x); // 10
    queue_push(first, last, y); // 10,20
    queue_pop(first, last);     // 20
    queue_push(first, last, z); // 20,30

    if (!(first->data == 20 && last->data == 30)) {
        test_case_record_error(&QueueInterleavedTest, TEST_RESULT_INCORRECT_VALUE,
            "Interleaved push/pop incorrect",
            "\t-- Expected: first=20, last=30\n\t-- Actual: first=%d, last=%d",
            first->data, last->data);
    }
    queue_pop(first, last);
    if (first->data != 30) {
        test_case_record_error(&QueueInterleavedTest, TEST_RESULT_INCORRECT_VALUE,
            "Interleaved push/pop wrong after removing 20",
            "\t-- Expected: 30\n\t-- Actual: %d", first->data);
    }
    queue_pop(first, last);
    if (!(first == NULL && last == NULL)) {
        test_case_record_error(&QueueInterleavedTest, TEST_RESULT_INVALID_STATE,
            "Interleaved push/pop not empty at end",
            "\t-- Expected: first=NULL, last=NULL\n\t-- Actual: first=%p, last=%p",
            (void*)first, (void*)last);
    }

    arena_release(&arena);
    return 0;
}

TEST(QueuePushN, "Storage", "Queue") {
    Arena arena = {0};
    CustomQueueNode *first = NULL;
    CustomQueueNode *last  = NULL;

    CustomQueueNode *new_node = arena_push_struct(&arena, CustomQueueNode);
    if (!new_node) {
        test_case_record_error(&QueuePushNTest, TEST_RESULT_NULL_VALUE,
            "New node allocation failed",
            "\t-- arena_push_struct returned NULL");
    } else {
        new_node->data = 1;
        new_node->n = NULL;
        queue_push_n(first, last, new_node, n);
        if (!(first == new_node && last == new_node)) {
            test_case_record_error(&QueuePushNTest, TEST_RESULT_INVALID_STATE,
                "Incorrect first/last after push",
                "\t-- Expected: first=%p, last=%p\n\t-- Actual: first=%p, last=%p",
                (void*)new_node, (void*)new_node, (void*)first, (void*)last);
        }
    }

    arena_release(&arena);
    return 0;
}

TEST(QueuePopN, "Storage", "Queue") {
    Arena arena = {0};
    CustomQueueNode *first = NULL;
    CustomQueueNode *last  = NULL;

    CustomQueueNode *node = arena_push_struct(&arena, CustomQueueNode);
    node->data = 1; node->n = NULL;
    queue_push_n(first, last, node, n);
    queue_pop_n(first, last, n);

    if (!(first == NULL && last == NULL)) {
        test_case_record_error(&QueuePopNTest, TEST_RESULT_INVALID_STATE,
            "Queue not empty after popping only element",
            "\t-- Expected: first=NULL, last=NULL\n\t-- Actual: first=%p, last=%p",
            (void*)first, (void*)last);
    }

    arena_release(&arena);
    return 0;
}

TEST(QueueFIFON, "Storage", "Queue") {
    Arena arena = {0};
    CustomQueueNode *first = NULL;
    CustomQueueNode *last  = NULL;

    CustomQueueNode *a = arena_push_struct(&arena, CustomQueueNode);
    CustomQueueNode *b = arena_push_struct(&arena, CustomQueueNode);
    CustomQueueNode *c = arena_push_struct(&arena, CustomQueueNode);
    a->data = 1; b->data = 2; c->data = 3;
    a->n = b->n = c->n = NULL;

    queue_push_n(first, last, a, n);
    queue_push_n(first, last, b, n);
    queue_push_n(first, last, c, n);

    if (!(first->data == 1 && last->data == 3)) {
        test_case_record_error(&QueueFIFONTest, TEST_RESULT_INCORRECT_VALUE,
            "Wrong first/last after pushes",
            "\t-- Expected: first=1, last=3\n\t-- Actual: first=%d, last=%d",
            first->data, last->data);
    }

    queue_pop_n(first, last, n);
    if (first->data != 2) {
        test_case_record_error(&QueueFIFONTest, TEST_RESULT_INCORRECT_VALUE,
            "Wrong value after popping first",
            "\t-- Expected: 2\n\t-- Actual: %d", first->data);
    }
    queue_pop_n(first, last, n);
    if (first->data != 3) {
        test_case_record_error(&QueueFIFONTest, TEST_RESULT_INCORRECT_VALUE,
            "Wrong value after popping second",
            "\t-- Expected: 3\n\t-- Actual: %d", first->data);
    }
    queue_pop_n(first, last, n);
    if (!(first == NULL && last == NULL)) {
        test_case_record_error(&QueueFIFONTest, TEST_RESULT_INVALID_STATE,
            "Queue not empty at end",
            "\t-- Expected: first=NULL, last=NULL\n\t-- Actual: first=%p, last=%p",
            (void*)first, (void*)last);
    }

    arena_release(&arena);
    return 0;
}

TEST(QueuePopEmptyN, "Storage", "Queue") {
    Arena arena = {0};
    CustomQueueNode *first = NULL;
    CustomQueueNode *last  = NULL;

    queue_pop_n(first, last, n);
    if (!(first == NULL && last == NULL)) {
        test_case_record_error(&QueuePopEmptyNTest, TEST_RESULT_INVALID_STATE,
            "queue_pop_n on empty queue modified pointers",
            "\t-- Expected: first=NULL, last=NULL\n\t-- Actual: first=%p, last=%p",
            (void*)first, (void*)last);
    }

    arena_release(&arena);
    return 0;
}

// -- Doubly Linked List Component --------------------------------------------

typedef struct IdiomaticDllNode {
    int data;
    struct IdiomaticDllNode *next;
    struct IdiomaticDllNode *prev;
} IdiomaticDllNode;

typedef struct CustomDllNode {
    int id;
    struct CustomDllNode *nxt;
    struct CustomDllNode *prv;
} CustomDllNode;

TEST(DLLPushBack, "Storage", "DLL") {
    Arena arena = {0};
    IdiomaticDllNode *first = NULL;
    IdiomaticDllNode *last  = NULL;

    IdiomaticDllNode *n1 = arena_push_struct(&arena, IdiomaticDllNode);
    if (!n1) {
        test_case_record_error(&DLLPushBackTest, TEST_RESULT_NULL_VALUE,
            "New node allocation failed",
            "\t-- arena_push_struct returned NULL");
    } else {
        n1->data = 1;
        dll_push_back(first, last, n1);

        if (!(first == n1 && last == n1)) {
            test_case_record_error(&DLLPushBackTest, TEST_RESULT_INVALID_STATE,
                "Wrong first/last after first push",
                "\t-- Expected: first==last==%p\n\t-- Actual: first=%p, last=%p",
                (void*)n1, (void*)first, (void*)last);
        }
        if (!(n1->next == NULL && n1->prev == NULL)) {
            test_case_record_error(&DLLPushBackTest, TEST_RESULT_INCORRECT_VALUE,
                "Node linkage incorrect",
                "\t-- Expected: n1->next=NULL, n1->prev=NULL\n\t-- Actual: next=%p, prev=%p",
                (void*)n1->next, (void*)n1->prev);
        }
    }

    arena_release(&arena);
    return 0;
}

TEST(DLLPushFront, "Storage", "DLL") {
    Arena arena = {0};
    IdiomaticDllNode *first = NULL;
    IdiomaticDllNode *last  = NULL;

    IdiomaticDllNode *n1 = arena_push_struct(&arena, IdiomaticDllNode);
    n1->data = 1; dll_push_back(first, last, n1);

    IdiomaticDllNode *n0 = arena_push_struct(&arena, IdiomaticDllNode);
    n0->data = 0;
    dll_push_front(first, last, n0);

    if (!(first == n0 && last == n1)) {
        test_case_record_error(&DLLPushFrontTest, TEST_RESULT_INVALID_STATE,
            "Wrong first/last after push front",
            "\t-- Expected: first=%p, last=%p\n\t-- Actual: first=%p, last=%p",
            (void*)n0, (void*)n1, (void*)first, (void*)last);
    }
    if (!(n0->next == n1 && n1->prev == n0)) {
        test_case_record_error(&DLLPushFrontTest, TEST_RESULT_INCORRECT_VALUE,
            "Wrong linkage after push",
            "\t-- Expected: n0->next=%p, n1->prev=%p\n\t-- Actual: n0->next=%p, n1->prev=%p",
            (void*)n1, (void*)n0, (void*)n0->next, (void*)n1->prev);
    }

    arena_release(&arena);
    return 0;
}

TEST(DLLInsertAfter, "Storage", "DLL") {
    Arena arena = {0};
    IdiomaticDllNode *first=NULL, *last=NULL;

    IdiomaticDllNode *n1 = arena_push_struct(&arena, IdiomaticDllNode);
    dll_push_back(first, last, n1);

    IdiomaticDllNode *n2 = arena_push_struct(&arena, IdiomaticDllNode);
    dll_insert_after(first, last, n1, n2);

    if (!(n1->next == n2 && n2->prev == n1)) {
        test_case_record_error(&DLLInsertAfterTest, TEST_RESULT_INCORRECT_VALUE,
            "Wrong linkage",
            "\t-- Expected: n1->next=n2, n2->prev=n1\n\t-- Actual: n1->next=%p, n2->prev=%p",
            (void*)n1->next, (void*)n2->prev);
    }
    if (last != n2) {
        test_case_record_error(&DLLInsertAfterTest, TEST_RESULT_INVALID_STATE,
            "Last not updated",
            "\t-- Expected: last=%p\n\t-- Actual: %p", (void*)n2, (void*)last);
    }

    arena_release(&arena);
    return 0;
}

TEST(DLLInsertBefore, "Storage", "DLL") {
    Arena arena = {0};
    IdiomaticDllNode *first=NULL, *last=NULL;

    IdiomaticDllNode *n1 = arena_push_struct(&arena, IdiomaticDllNode);
    IdiomaticDllNode *n2 = arena_push_struct(&arena, IdiomaticDllNode);
    dll_push_back(first, last, n1);
    dll_push_back(first, last, n2);

    IdiomaticDllNode *nMid = arena_push_struct(&arena, IdiomaticDllNode);
    dll_insert_before(first, last, n2, nMid);

    if (!(n1->next == nMid && nMid->next == n2)) {
        test_case_record_error(&DLLInsertBeforeTest, TEST_RESULT_INCORRECT_VALUE,
            "Wrong forward linkage",
            "\t-- Expected: n1->next=nMid, nMid->next=n2\n\t-- Actual: n1->next=%p, nMid->next=%p",
            (void*)n1->next, (void*)nMid->next);
    }
    if (!(nMid->prev == n1 && n2->prev == nMid)) {
        test_case_record_error(&DLLInsertBeforeTest, TEST_RESULT_INCORRECT_VALUE,
            "Wrong backward linkage",
            "\t-- Expected: nMid->prev=n1, n2->prev=nMid\n\t-- Actual: nMid->prev=%p, n2->prev=%p",
            (void*)nMid->prev, (void*)n2->prev);
    }

    arena_release(&arena);
    return 0;
}

TEST(DLLRemove, "Storage", "DLL") {
    Arena arena = {0};
    IdiomaticDllNode *first=NULL, *last=NULL;

    IdiomaticDllNode *n0 = arena_push_struct(&arena, IdiomaticDllNode);
    IdiomaticDllNode *n1 = arena_push_struct(&arena, IdiomaticDllNode);
    IdiomaticDllNode *n2 = arena_push_struct(&arena, IdiomaticDllNode);
    dll_push_back(first, last, n0);
    dll_push_back(first, last, n1);
    dll_push_back(first, last, n2);

    // remove middle
    dll_remove(first, last, n1);
    if (!(n0->next == n2 && n2->prev == n0)) {
        test_case_record_error(&DLLRemoveTest, TEST_RESULT_INVALID_STATE,
            "Middle removal broke linkage",
            "\t-- Expected: n0->next=n2, n2->prev=n0\n\t-- Actual: n0->next=%p, n2->prev=%p",
            (void*)n0->next, (void*)n2->prev);
    }

    // remove first
    dll_remove(first, last, n0);
    if (!(first == n2 && n2->prev == NULL)) {
        test_case_record_error(&DLLRemoveTest, TEST_RESULT_INVALID_STATE,
            "First removal wrong",
            "\t-- Expected: first=%p, prev=NULL\n\t-- Actual: first=%p, prev=%p",
            (void*)n2, (void*)first, (void*)first ? (void*)first->prev : NULL);
    }

    // remove last
    dll_remove(first, last, n2);
    if (!(first == NULL && last == NULL)) {
        test_case_record_error(&DLLRemoveTest, TEST_RESULT_INVALID_STATE,
            "Last removal wrong",
            "\t-- Expected: first=NULL, last=NULL\n\t-- Actual: first=%p, last=%p",
            (void*)first, (void*)last);
    }

    arena_release(&arena);
    return 0;
}

TEST(DLLOrdering, "Storage", "DLL") {
    Arena arena = {0};
    IdiomaticDllNode *first=NULL, *last=NULL;

    IdiomaticDllNode *a = arena_push_struct(&arena, IdiomaticDllNode);
    IdiomaticDllNode *b = arena_push_struct(&arena, IdiomaticDllNode);
    IdiomaticDllNode *c = arena_push_struct(&arena, IdiomaticDllNode);
    a->data=1; b->data=2; c->data=3;

    dll_push_back(first, last, a);
    dll_push_back(first, last, b);
    dll_push_back(first, last, c);

    if (!(first->data==1 && last->data==3)) {
        test_case_record_error(&DLLOrderingTest, TEST_RESULT_INCORRECT_VALUE,
            "Wrong first/last values",
            "\t-- Expected: first=1, last=3\n\t-- Actual: first=%d, last=%d",
            first->data, last->data);
    }
    if (!(a->next==b && b->next==c && c->prev==b)) {
        test_case_record_error(&DLLOrderingTest, TEST_RESULT_INCORRECT_VALUE,
            "Wrong linkage",
            "\t-- Expected: a->next=b, b->next=c, c->prev=b\n\t-- Actual: a->next=%p, b->next=%p, c->prev=%p",
            (void*)a->next, (void*)b->next, (void*)c->prev);
    }

    arena_release(&arena);
    return 0;
}

TEST(DLLCustom, "Storage", "DLL") {
    Arena arena = {0};
    CustomDllNode *first=NULL, *last=NULL;

    CustomDllNode *x = arena_push_struct(&arena, CustomDllNode);
    CustomDllNode *y = arena_push_struct(&arena, CustomDllNode);
    x->id=10; y->id=20;

    dll_push_front_np(first, last, x, nxt, prv);
    dll_push_back_np(first, last, y, nxt, prv);

    if (!(first==x && last==y)) {
        test_case_record_error(&DLLCustomTest, TEST_RESULT_INVALID_STATE,
            "Wrong first/last",
            "\t-- Expected: first=%p, last=%p\n\t-- Actual: first=%p, last=%p",
            (void*)x, (void*)y, (void*)first, (void*)last);
    }
    if (!(x->nxt==y && y->prv==x)) {
        test_case_record_error(&DLLCustomTest, TEST_RESULT_INCORRECT_VALUE,
            "Wrong linkage",
            "\t-- Expected: x->nxt=y, y->prv=x\n\t-- Actual: x->nxt=%p, y->prv=%p",
            (void*)x->nxt, (void*)y->prv);
    }

    CustomDllNode *z = arena_push_struct(&arena, CustomDllNode);
    z->id=15;
    dll_insert_after_np(first, last, x, z, nxt, prv);
    if (!(x->nxt==z && z->nxt==y)) {
        test_case_record_error(&DLLCustomTest, TEST_RESULT_INCORRECT_VALUE,
            "Wrong linkage",
            "\t-- Expected: x->nxt=z, z->nxt=y\n\t-- Actual: x->nxt=%p, z->nxt=%p",
            (void*)x->nxt, (void*)z->nxt);
    }
    dll_remove_np(first, last, z, nxt, prv);
    dll_insert_before_np(first, last, y, z, nxt, prv);
    if (!(z->prv==x && z->nxt==y)) {
        test_case_record_error(&DLLCustomTest, TEST_RESULT_INCORRECT_VALUE,
            "Wrong linkage",
            "\t-- Expected: z->prv=x, z->nxt=y\n\t-- Actual: z->prv=%p, z->nxt=%p",
            (void*)z->prv, (void*)z->nxt);
    }

    dll_remove_np(first, last, x, nxt, prv);
    dll_remove_np(first, last, y, nxt, prv);
    dll_remove_np(first, last, z, nxt, prv);

    if (!(first==NULL && last==NULL)) {
        test_case_record_error(&DLLCustomTest, TEST_RESULT_INVALID_STATE,
            "List not empty",
            "\t-- Expected: first=NULL, last=NULL\n\t-- Actual: first=%p, last=%p",
            (void*)first, (void*)last);
    }

    arena_release(&arena);
    return 0;
}

// -- Array Component ---------------------------------------------------------

typedef struct { 
    _ArrayHeader_; 
    int *v; 
} IntArray;

typedef struct {
    char c;
    double d; // adds padding/alignment
} WeirdStruct;

typedef struct {
    _ArrayHeader_;
    WeirdStruct *v;
} WeirdArray;

TEST(ArrayPush, "Storage", "Array") {
    Arena arena = {0};
    IntArray arr = {0};

    for (int i = 0; i < 20; i++) {
        array_push(&arena, arr, i);
    }
    if (arr.count != 20) {
        test_case_record_error(&ArrayPushTest, TEST_RESULT_INCORRECT_VALUE,
            "Count mismatch",
            "\t-- Expected: 20\n\t-- Actual: %llu", (u64)arr.count);
    }
    for (int i = 0; i < 20; i++) {
        if (arr.v[i] != i) {
            test_case_record_error(&ArrayPushTest, TEST_RESULT_INCORRECT_VALUE,
                "Wrong value at index",
                "\t-- Index %d: Expected %d\n\t-- Actual: %d", i, i, arr.v[i]);
        }
    }

    arena_release(&arena);
    return 0;
}

TEST(ArrayAdd, "Storage", "Array") {
    Arena arena = {0};
    IntArray arr = {0};

    for (int i = 0; i < 20; i++) array_push(&arena, arr, i);

    int *extra = array_add(&arena, arr, 5);
    if (arr.count != 25) {
        test_case_record_error(&ArrayAddTest, TEST_RESULT_INCORRECT_VALUE,
            "Count mismatch",
            "\t-- Expected: 25\n\t-- Actual: %llu", (u64)arr.count);
    }
    for (int i = 0; i < 5; i++) extra[i] = 100 + i;
    if (!(arr.v[20] == 100 && arr.v[24] == 104)) {
        test_case_record_error(&ArrayAddTest, TEST_RESULT_INCORRECT_VALUE,
            "Wrong values in added block",
            "\t-- Expected: 100..104 at indices 20..24\n\t-- Actual: %d..%d", arr.v[20], arr.v[24]);
    }

    arena_release(&arena);
    return 0;
}

TEST(ArrayAddClear, "Storage", "Array") {
    Arena arena = {0};
    IntArray arr = {0};

    for (int i = 0; i < 20; i++) array_push(&arena, arr, i);
    int *clear_block = array_add_clear(&arena, arr, 3);

    if (arr.count != 23) {
        test_case_record_error(&ArrayAddClearTest, TEST_RESULT_INCORRECT_VALUE,
            "Count mismatch",
            "\t-- Expected: 23\n\t-- Actual: %llu", (u64)arr.count);
    }
    for (int i = 0; i < 3; i++) {
        if (clear_block[i] != 0) {
            test_case_record_error(&ArrayAddClearTest, TEST_RESULT_INCORRECT_VALUE,
                "Non-zero entry",
                "\t-- Index %d: Expected 0\n\t-- Actual: %d", i, clear_block[i]);
        }
    }

    arena_release(&arena);
    return 0;
}

TEST(ArrayReserve, "Storage", "Array") {
    Arena arena = {0};
    IntArray arr = {0};

    for (int i = 0; i < 10; i++) array_push(&arena, arr, i);
    u64 prev_capacity = arr.capacity;
    array_reserve(&arena, arr, prev_capacity * 2 + 10);

    if (arr.capacity < prev_capacity * 2 + 10) {
        test_case_record_error(&ArrayReserveTest, TEST_RESULT_INCORRECT_VALUE,
            "Capacity too small",
            "\t-- Expected: >= %llu\n\t-- Actual: %llu",
            (u64)(prev_capacity * 2 + 10), (u64)arr.capacity);
    }
    if (arr.count != 10) {
        test_case_record_error(&ArrayReserveTest, TEST_RESULT_INVALID_STATE,
            "Count modified",
            "\t-- Expected: 10\n\t-- Actual: %llu", (u64)arr.count);
    }

    arena_release(&arena);
    return 0;
}

TEST(ArrayInsert, "Storage", "Array") {
    Arena arena = {0};
    IntArray arr = {0};
    for (int i = 0; i < 10; i++) array_push(&arena, arr, i);

    array_insert(&arena, arr, 5, 999);
    if (arr.v[5] != 999) {
        test_case_record_error(&ArrayInsertTest, TEST_RESULT_INCORRECT_VALUE,
            "Wrong value inserted",
            "\t-- Index 5: Expected 999\n\t-- Actual: %d", arr.v[5]);
    }
    if (arr.v[6] != 5) {
        test_case_record_error(&ArrayInsertTest, TEST_RESULT_INVALID_STATE,
            "Shift failed",
            "\t-- Index 6: Expected old value 5\n\t-- Actual: %d", arr.v[6]);
    }

    arena_release(&arena);
    return 0;
}

TEST(ArrayRemove, "Storage", "Array") {
    Arena arena = {0};
    IntArray arr = {0};
    for (int i = 0; i < 10; i++) array_push(&arena, arr, i);

    int val_before = arr.v[5];
    array_insert(&arena, arr, 5, 999);
    array_remove(arr, 5, 1);

    if (arr.v[5] != val_before) {
        test_case_record_error(&ArrayRemoveTest, TEST_RESULT_INCORRECT_VALUE,
            "Wrong shift result",
            "\t-- Index 5: Expected %d\n\t-- Actual: %d", val_before, arr.v[5]);
    }

    arena_release(&arena);
    return 0;
}

TEST(ArrayRemoveAll, "Storage", "Array") {
    Arena arena = {0};
    IntArray arr = {0};
    for (int i = 0; i < 10; i++) array_push(&arena, arr, i);

    array_remove(arr, 0, arr.count);
    if (arr.count != 0) {
        test_case_record_error(&ArrayRemoveAllTest, TEST_RESULT_INVALID_STATE,
            "Count not reset",
            "\t-- Expected: 0\n\t-- Actual: %llu", (u64)arr.count);
    }
    if (arr.capacity == 0) {
        test_case_record_error(&ArrayRemoveAllTest, TEST_RESULT_INVALID_STATE,
            "Capacity lost",
            "\t-- Expected: capacity > 0\n\t-- Actual: 0");
    }

    arena_release(&arena);
    return 0;
}

TEST(ArrayCapacityGrowth, "Storage", "Array") {
    Arena arena = {0};
    IntArray arr = {0};

    u64 old_cap = arr.capacity;
    for (int i = 0; i < 1000; i++) array_push(&arena, arr, i);

    if (arr.count != 1000) {
        test_case_record_error(&ArrayCapacityGrowthTest, TEST_RESULT_INCORRECT_VALUE,
            "Wrong count",
            "\t-- Expected: 1000\n\t-- Actual: %llu", (u64)arr.count);
    }
    if (arr.capacity <= old_cap) {
        test_case_record_error(&ArrayCapacityGrowthTest, TEST_RESULT_INVALID_STATE,
            "Capacity did not grow",
            "\t-- Expected: > %llu\n\t-- Actual: %llu",
            (u64)old_cap, (u64)arr.capacity);
    }
    for (int i = 0; i < 1000; i++) {
        if (arr.v[i] != i) {
            test_case_record_error(&ArrayCapacityGrowthTest, TEST_RESULT_INCORRECT_VALUE,
                "Value corruption",
                "\t-- Index %d: Expected %d\n\t-- Actual: %d", i, i, arr.v[i]);
        }
    }

    arena_release(&arena);
    return 0;
}

TEST(ArrayClear, "Storage", "Array") {
    Arena arena = {0};
    IntArray arr = {0};
    for (int i = 0; i < 50; i++) array_push(&arena, arr, i);

    array_clear(arr);
    if (arr.count != 0) {
        test_case_record_error(&ArrayClearTest, TEST_RESULT_INVALID_STATE,
            "Count not reset",
            "\t-- Expected: 0\n\t-- Actual: %llu", (u64)arr.count);
    }
    if (arr.capacity == 0) {
        test_case_record_error(&ArrayClearTest, TEST_RESULT_INVALID_STATE,
            "Capacity lost",
            "\t-- Expected: capacity > 0\n\t-- Actual: 0");
    }

    arena_release(&arena);
    return 0;
}

TEST(ArrayCustomStruct, "Storage", "Array") {
    Arena arena = {0};
    WeirdArray arr = {0};

    WeirdStruct ws1 = {'A', 3.14};
    WeirdStruct ws2 = {'B', 6.28};
    array_push(&arena, arr, ws1);
    array_push(&arena, arr, ws2);

    if (!(arr.v[0].c == 'A' && arr.v[0].d == 3.14)) {
        test_case_record_error(&ArrayCustomStructTest, TEST_RESULT_INCORRECT_VALUE,
            "Wrong element 0",
            "\t-- Expected: {'A', 3.14}\n\t-- Actual: {%c, %f}", arr.v[0].c, arr.v[0].d);
    }
    if (!(arr.v[1].c == 'B' && arr.v[1].d == 6.28)) {
        test_case_record_error(&ArrayCustomStructTest, TEST_RESULT_INCORRECT_VALUE,
            "Wrong element 1",
            "\t-- Expected: {'B', 6.28}\n\t-- Actual: {%c, %f}", arr.v[1].c, arr.v[1].d);
    }

    array_insert(&arena, arr, 1, ((WeirdStruct){'C', 9.99}));
    if (!(arr.v[1].c == 'C' && arr.v[1].d == 9.99)) {
        test_case_record_error(&ArrayCustomStructTest, TEST_RESULT_INCORRECT_VALUE,
            "Wrong inserted values",
            "\t-- Expected: {'C', 9.99}\n\t-- Actual: {%c, %f}", arr.v[1].c, arr.v[1].d);
    }

    array_remove(arr, 1, 1);
    if (arr.v[1].c != 'B') {
        test_case_record_error(&ArrayCustomStructTest, TEST_RESULT_INCORRECT_VALUE,
            "Wrong value after remove",
            "\t-- Expected: 'B'\n\t-- Actual: '%c'", arr.v[1].c);
    }

    arena_release(&arena);
    return 0;
}

// -- Map Component -----------------------------------------------------------

// TODO

TEST(MapMock, "Storage", "Map") {
	return 0;
}

//=============================================================================
// String Module
//=============================================================================

// -- String Component --------------------------------------------------------

TEST(StringSlice, "String", "String") {
	const char *text = "Hello, World!";

	String s;
	s.value = malloc(13);
	memory_copy(s.value, (void *) text, 13);
	s.length = 13;

	String slice = string_slice(&s, 0, 5);
	if (slice.value == NULL) {
		test_case_record_error(&StringSliceTest, TEST_RESULT_NULL_VALUE,
			"string slice is NULL", "");
	}
	if (slice.length != 5) {
		test_case_record_error(&StringSliceTest, TEST_RESULT_INVALID_STATE,
			"string slice length is incorrect", 
			"\t-- Expected: 5\n\t-- Actual %llu", slice.length);
	}
	if (memory_compare(slice.value, s.value, 5) != 0) {
		test_case_record_error(&StringSliceTest, TEST_RESULT_INCORRECT_VALUE,
			"string slice contains incorrect memory",
			"\t-- Expected: Hello\n\t-- Actual %s", slice.value);
	}

	slice = string_slice(&s, 7, 12);
	if (slice.value == NULL) {
		test_case_record_error(&StringSliceTest, TEST_RESULT_NULL_VALUE,
			"string slice is NULL", "");
	}
	if (slice.length != 5) {
		test_case_record_error(&StringSliceTest, TEST_RESULT_INVALID_STATE,
			"string slice length is incorrect", 
			"\t-- Expected: 5\n\t-- Actual %llu", slice.length);
	}
	if (memory_compare(slice.value, s.value + 7, 5) != 0) {
		test_case_record_error(&StringSliceTest, TEST_RESULT_INCORRECT_VALUE,
			"string slice contains incorrect memory",
			"\t-- Expected: World\n\t-- Actual %s", slice.value);
	}

	slice = string_slice(&s, 0, 40);
	if (slice.value == NULL) {
		test_case_record_error(&StringSliceTest, TEST_RESULT_NULL_VALUE,
			"string slice is NULL", "");
	}
	if (slice.length != 13) {
		test_case_record_error(&StringSliceTest, TEST_RESULT_INVALID_STATE,
			"string slice length is incorrect", 
			"\t-- Expected: 13\n\t-- Actual %llu", slice.length);
	}
	if (memory_compare(slice.value, s.value, 13) != 0) {
		test_case_record_error(&StringSliceTest, TEST_RESULT_INCORRECT_VALUE,
			"string slice contains incorrect memory",
			"\t-- Expected: Hello, World!\n\t-- Actual %s", slice.value);
	}

	return 0;
}

TEST(StringCopy, "String", "String") {
	const char *text = "Hello, World!";

	String s;
	s.value = malloc(13);
	memory_copy(s.value, (void *) text, 13);
	s.length = 13;

	String *copy = string_copy(&s);
	if (copy->value == NULL) {
		test_case_record_error(&StringCopyTest, TEST_RESULT_NULL_VALUE,
			"string copy is NULL", "");
	}
	if (copy->length != s.length) {
		test_case_record_error(&StringCopyTest, TEST_RESULT_INVALID_STATE,
			"string copy length is incorrect", 
			"\t-- Expected: %llu\n\t-- Actual %llu", s.length, copy->length);
	}
	if (memory_compare(copy->value, s.value, s.length) != 0) {
		test_case_record_error(&StringCopyTest, TEST_RESULT_INCORRECT_VALUE,
			"string copy contains incorrect memory",
			"\t-- Expected: %s\n\t-- Actual %s", s.value, copy->value);
	}
	if (copy->value == s.value) {
		test_case_record_error(&StringCopyTest, TEST_RESULT_INVALID_STATE,
			"string copy points to the same memory as the original", "");
	}

	String empty;
	empty.value = malloc(0);
	empty.length = 0;

	String *copy_empty = string_copy(&empty);
	if (copy_empty->value == NULL && empty.length > 0) {
		test_case_record_error(&StringCopyTest, TEST_RESULT_NULL_VALUE,
			"string copy (empty) is NULL", "");
	}
	if (copy_empty->length != 0) {
		test_case_record_error(&StringCopyTest, TEST_RESULT_INVALID_STATE,
			"string copy (empty) length is incorrect", 
			"\t-- Expected: 0\n\t-- Actual %llu", copy_empty->length);
	}

	return 0;
}

TEST(StringCopySlice, "String", "String") {
	const char *text = "Hello, World!";

	String s;
	s.value = malloc(13);
	memory_copy(s.value, (void *) text, 13);
	s.length = 13;

	// Copy first 5 chars: "Hello"
	String *copy = string_copy_slice(&s, 0, 5);
	if (copy->value == NULL) {
		test_case_record_error(&StringCopySliceTest, TEST_RESULT_NULL_VALUE,
			"string copy slice is NULL", "");
	}
	if (copy->length != 5) {
		test_case_record_error(&StringCopySliceTest, TEST_RESULT_INVALID_STATE,
			"string copy slice length is incorrect",
			"\t-- Expected: 5\n\t-- Actual %llu", copy->length);
	}
	if (memory_compare(copy->value, s.value, 5) != 0) {
		test_case_record_error(&StringCopySliceTest, TEST_RESULT_INCORRECT_VALUE,
			"string copy slice contains incorrect memory",
			"\t-- Expected: Hello\n\t-- Actual %s", copy->value);
	}
	if (copy->value == s.value) {
		test_case_record_error(&StringCopySliceTest, TEST_RESULT_INVALID_STATE,
			"string copy slice points to same memory as original", "");
	}

	// Copy "World"
	copy = string_copy_slice(&s, 7, 12);
	if (copy->value == NULL) {
		test_case_record_error(&StringCopySliceTest, TEST_RESULT_NULL_VALUE,
			"string copy slice is NULL", "");
	}
	if (copy->length != 5) {
		test_case_record_error(&StringCopySliceTest, TEST_RESULT_INVALID_STATE,
			"string copy slice length is incorrect",
			"\t-- Expected: 5\n\t-- Actual %llu", copy->length);
	}
	if (memory_compare(copy->value, s.value + 7, 5) != 0) {
		test_case_record_error(&StringCopySliceTest, TEST_RESULT_INCORRECT_VALUE,
			"string copy slice contains incorrect memory",
			"\t-- Expected: World\n\t-- Actual %s", copy->value);
	}
	if (copy->value == s.value + 7) {
		test_case_record_error(&StringCopySliceTest, TEST_RESULT_INVALID_STATE,
			"string copy slice points to same memory as original slice", "");
	}

	// Copy past-the-end (should clamp to full string)
	copy = string_copy_slice(&s, 0, 40);
	if (copy->value == NULL) {
		test_case_record_error(&StringCopySliceTest, TEST_RESULT_NULL_VALUE,
			"string copy slice is NULL", "");
	}
	if (copy->length != 13) {
		test_case_record_error(&StringCopySliceTest, TEST_RESULT_INVALID_STATE,
			"string copy slice length is incorrect",
			"\t-- Expected: 13\n\t-- Actual %llu", copy->length);
	}
	if (memory_compare(copy->value, s.value, 13) != 0) {
		test_case_record_error(&StringCopySliceTest, TEST_RESULT_INCORRECT_VALUE,
			"string copy slice contains incorrect memory",
			"\t-- Expected: Hello, World!\n\t-- Actual %s", copy->value);
	}

	// Empty slice
	copy = string_copy_slice(&s, 5, 5);
	if (copy->value == NULL && copy->length != 0) {
		test_case_record_error(&StringCopySliceTest, TEST_RESULT_NULL_VALUE,
			"string copy slice (empty) is NULL", "");
	}
	if (copy->length != 0) {
		test_case_record_error(&StringCopySliceTest, TEST_RESULT_INVALID_STATE,
			"string copy slice (empty) length is incorrect",
			"\t-- Expected: 0\n\t-- Actual %llu", copy->length);
	}

	return 0;
}

TEST(StringIsEqual, "String", "String") {
	const char *text1 = "Hello, World!";
	const char *text2 = "Hello, World!";
	const char *text3 = "Hello, world!"; // differs by case
	const char *text4 = "Hello";

	String s1, s2, s3, s4, empty;

	// Setup strings
	s1.value = malloc(13);
	memory_copy(s1.value, (void *) text1, 13);
	s1.length = 13;

	s2.value = malloc(13);
	memory_copy(s2.value, (void *) text2, 13);
	s2.length = 13;

	s3.value = malloc(13);
	memory_copy(s3.value, (void *) text3, 13);
	s3.length = 13;

	s4.value = malloc(5);
	memory_copy(s4.value, (void *) text4, 5);
	s4.length = 5;

	empty.value = malloc(0);
	empty.length = 0;

	// Identical strings
	if (!string_is_equal(&s1, &s2)) {
		test_case_record_error(&StringIsEqualTest, TEST_RESULT_INCORRECT_VALUE,
			"identical strings not equal", "");
	}

	// Different length
	if (string_is_equal(&s1, &s4)) {
		test_case_record_error(&StringIsEqualTest, TEST_RESULT_INCORRECT_VALUE,
			"strings of different length reported equal", "");
	}

	// Same length, different content
	if (string_is_equal(&s1, &s3)) {
		test_case_record_error(&StringIsEqualTest, TEST_RESULT_INCORRECT_VALUE,
			"strings with different content reported equal", "");
	}

	// Both empty
	if (!string_is_equal(&empty, &empty)) {
		test_case_record_error(&StringIsEqualTest, TEST_RESULT_INCORRECT_VALUE,
			"empty strings not equal", "");
	}

	// One empty, one non-empty
	if (string_is_equal(&s1, &empty)) {
		test_case_record_error(&StringIsEqualTest, TEST_RESULT_INCORRECT_VALUE,
			"non-empty and empty string reported equal", "");
	}

	return 0;
}

TEST(StringFindNextChar, "String", "String") {
	const char *text = "Hello, World!";

	String s;
	s.value = malloc(13);
	memory_copy(s.value, (void *) text, 13);
	s.length = 13;

	// Character at start ('H')
	u64 index = string_find_next_char(&s, 'H');
	if (index != 0) {
		test_case_record_error(&StringFindNextCharTest, TEST_RESULT_INCORRECT_VALUE,
			"failed to find character at start",
			"\t-- Expected: 0\n\t-- Actual %llu", index);
	}

	// Character in middle ('W')
	index = string_find_next_char(&s, 'W');
	if (index != 7) {
		test_case_record_error(&StringFindNextCharTest, TEST_RESULT_INCORRECT_VALUE,
			"failed to find character in middle",
			"\t-- Expected: 7\n\t-- Actual %llu", index);
	}

	// Character at end ('!')
	index = string_find_next_char(&s, '!');
	if (index != 12) {
		test_case_record_error(&StringFindNextCharTest, TEST_RESULT_INCORRECT_VALUE,
			"failed to find character at end",
			"\t-- Expected: 12\n\t-- Actual %llu", index);
	}

	// Character not present ('x')
	index = string_find_next_char(&s, 'x');
	if (index != (u64)-1) {
		test_case_record_error(&StringFindNextCharTest, TEST_RESULT_INCORRECT_VALUE,
			"not-found character should return -1",
			"\t-- Expected: %llu\n\t-- Actual %llu", (u64)-1, index);
	}

	// Empty string
	String empty;
	empty.value = malloc(0);
	empty.length = 0;

	index = string_find_next_char(&empty, 'a');
	if (index != (u64)-1) {
		test_case_record_error(&StringFindNextCharTest, TEST_RESULT_INCORRECT_VALUE,
			"search in empty string should return -1",
			"\t-- Expected: %llu\n\t-- Actual %llu", (u64)-1, index);
	}

	return 0;
}

TEST(StringFindLastChar, "String", "String") {
    const char *text = "Hello, World!";

    String s;
    s.value = malloc(13);
    memory_copy(s.value, (void *) text, 13);
    s.length = 13;

    // Character at start ('H')
    u64 index = string_find_last_char(&s, 'H');
    if (index != 0) {
        test_case_record_error(&StringFindLastCharTest, TEST_RESULT_INCORRECT_VALUE,
            "failed to find last character at start",
            "\t-- Expected: 0\n\t-- Actual %llu", index);
    }

    // Character in middle with multiple occurrences ('l' -> last at index 10)
    index = string_find_last_char(&s, 'l');
    if (index != 10) {
        test_case_record_error(&StringFindLastCharTest, TEST_RESULT_INCORRECT_VALUE,
            "failed to find last occurrence in middle",
            "\t-- Expected: 10\n\t-- Actual %llu", index);
    }

    // Character at end ('!')
    index = string_find_last_char(&s, '!');
    if (index != 12) {
        test_case_record_error(&StringFindLastCharTest, TEST_RESULT_INCORRECT_VALUE,
            "failed to find last character at end",
            "\t-- Expected: 12\n\t-- Actual %llu", index);
    }

    // Character not present ('x')
    index = string_find_last_char(&s, 'x');
    if (index != (u64)-1) {
        test_case_record_error(&StringFindLastCharTest, TEST_RESULT_INCORRECT_VALUE,
            "not-found character should return -1",
            "\t-- Expected: %llu\n\t-- Actual %llu", (u64)-1, index);
    }

    // Empty string
    String empty;
    empty.value = malloc(0);
    empty.length = 0;

    index = string_find_last_char(&empty, 'a');
    if (index != (u64)-1) {
        test_case_record_error(&StringFindLastCharTest, TEST_RESULT_INCORRECT_VALUE,
            "search in empty string should return -1",
            "\t-- Expected: %llu\n\t-- Actual %llu", (u64)-1, index);
    }

    return 0;
}

TEST(StringFindNextSubstring, "String", "String") {
    const char *text = "Hello, World!";
    const char *needle1 = "Hello";
    const char *needle2 = "World";
    const char *needle3 = "lo, W";
    const char *needle4 = "NotFound";

    String s;
    s.value = malloc(13);
    memory_copy(s.value, (void *) text, 13);
    s.length = 13;

    String sub1, sub2, sub3, sub4;

    sub1.value = malloc(5);
    memory_copy(sub1.value, (void *) needle1, 5);
    sub1.length = 5;

    sub2.value = malloc(5);
    memory_copy(sub2.value, (void *) needle2, 5);
    sub2.length = 5;

    sub3.value = malloc(5);
    memory_copy(sub3.value, (void *) needle3, 5);
    sub3.length = 5;

    sub4.value = malloc(8);
    memory_copy(sub4.value, (void *) needle4, 8);
    sub4.length = 8;

    // Substring at start
    u64 index = string_find_next_substring(&s, &sub1);
    if (index != 0) {
        test_case_record_error(&StringFindNextSubstringTest, TEST_RESULT_INCORRECT_VALUE,
            "failed to find substring at start",
            "\t-- Expected: 0\n\t-- Actual %llu", index);
    }

    // Substring in middle 
	index = string_find_next_substring(&s, &sub3);
    if (index != 3) {
        test_case_record_error(&StringFindNextSubstringTest, TEST_RESULT_INCORRECT_VALUE,
            "failed to find substring in middle",
            "\t-- Expected: 3\n\t-- Actual %llu", index);
    }

    // Substring at end
    index = string_find_next_substring(&s, &sub2);
    if (index != 7) {
        test_case_record_error(&StringFindNextSubstringTest, TEST_RESULT_INCORRECT_VALUE,
            "failed to find substring at end",
            "\t-- Expected: 7\n\t-- Actual %llu", index);
    }

    // Substring not present
    index = string_find_next_substring(&s, &sub4);
    if (index != (u64)-1) {
        test_case_record_error(&StringFindNextSubstringTest, TEST_RESULT_INCORRECT_VALUE,
            "not-found substring should return -1",
            "\t-- Expected: %llu\n\t-- Actual %llu", (u64)-1, index);
    }

    // Empty substring
    String empty;
    empty.value = malloc(0);
    empty.length = 0;
    index = string_find_next_substring(&s, &empty);
    if (index != 0) {
        test_case_record_error(&StringFindNextSubstringTest, TEST_RESULT_INCORRECT_VALUE,
            "empty substring should match at index 0",
            "\t-- Expected: 0\n\t-- Actual %llu", index);
    }

    return 0;
}

TEST(StringFindLastSubstring, "String", "String") {
    const char *text = "Hello, World! Hello";
    const char *needle1 = "Hello";
    const char *needle2 = "World";
    const char *needle3 = "lo, W";
    const char *needle4 = "NotFound";

    String s;
    s.value = malloc(19);
    memory_copy(s.value, (void *) text, 19);
    s.length = 19;

    String sub1, sub2, sub3, sub4;

    sub1.value = malloc(5);
    memory_copy(sub1.value, (void *) needle1, 5);
    sub1.length = 5;

    sub2.value = malloc(5);
    memory_copy(sub2.value, (void *) needle2, 5);
    sub2.length = 5;

    sub3.value = malloc(5);
    memory_copy(sub3.value, (void *) needle3, 5);
    sub3.length = 5;

    sub4.value = malloc(8);
    memory_copy(sub4.value, (void *) needle4, 8);
    sub4.length = 8;

    // Last substring at end
    u64 index = string_find_last_substring(&s, &sub1);
    if (index != 14) {
        test_case_record_error(&StringFindLastSubstringTest, TEST_RESULT_INCORRECT_VALUE,
            "failed to find substring at end",
            "\t-- Expected: 14\n\t-- Actual %llu", index);
    }

    // Substring in middle
    index = string_find_last_substring(&s, &sub3);
    if (index != 3) {
        test_case_record_error(&StringFindLastSubstringTest, TEST_RESULT_INCORRECT_VALUE,
            "failed to find substring in middle",
            "\t-- Expected: 3\n\t-- Actual %llu", index);
    }

    // Substring only occurs once
    index = string_find_last_substring(&s, &sub2);
    if (index != 7) {
        test_case_record_error(&StringFindLastSubstringTest, TEST_RESULT_INCORRECT_VALUE,
            "failed to find unique substring",
            "\t-- Expected: 7\n\t-- Actual %llu", index);
    }

    // Substring not present
    index = string_find_last_substring(&s, &sub4);
    if (index != (u64)-1) {
        test_case_record_error(&StringFindLastSubstringTest, TEST_RESULT_INCORRECT_VALUE,
            "not-found substring should return -1",
            "\t-- Expected: %llu\n\t-- Actual %llu", (u64)-1, index);
    }

    // Empty substring
    String empty;
    empty.value = malloc(0);
    empty.length = 0;
    index = string_find_last_substring(&s, &empty);
    if (index != s.length) {
        test_case_record_error(&StringFindLastSubstringTest, TEST_RESULT_INCORRECT_VALUE,
            "empty substring should match at string length",
            "\t-- Expected: %llu\n\t-- Actual %llu", s.length, index);
    }

    return 0;
}

// -- String Conversion Component ---------------------------------------------

TEST(StringFromCStr, "String", "Conversion") {
    const char *text1 = "Hello, World!";
    const char *text2 = "";
    const char *text3 = NULL;

    // Non-empty string
    String s1 = string_from_cstr(text1, 13);
    if (s1.length != 13 || !s1.value || memory_compare(s1.value, (void *) text1, 13) != 0) {
        test_case_record_error(&StringFromCStrTest, TEST_RESULT_INCORRECT_VALUE,
            "failed to construct string from valid C string",
            "\t-- Expected length: 13\n\t-- Actual length: %llu", s1.length);
    }

    // Empty string
    String s2 = string_from_cstr(text2, 0);
    if (s2.length != 0 || !s2.value || s2.value != text2) {
        test_case_record_error(&StringFromCStrTest, TEST_RESULT_INCORRECT_VALUE,
            "failed to construct string from empty C string",
            "\t-- Expected length: 0\n\t-- Actual length: %llu", s2.length);
    }

    // NULL string
    String s3 = string_from_cstr(text3, 0);
    if (s3.length != 0 || s3.value != NULL) {
        test_case_record_error(&StringFromCStrTest, TEST_RESULT_INCORRECT_VALUE,
            "failed to handle NULL C string",
            "\t-- Expected: NULL\n\t-- Actual: %p", s3.value);
    }

    return 0;
}

TEST(StringFromCStrAlloc, "String", "Conversion") {
    const char *text1 = "Hello, World!";
    const char *text2 = "";
    const char *text3 = NULL;

    // Non-empty string
    String *s1 = string_from_cstr_alloc(text1, 13);
    if (!s1 || s1->length != 13 || !s1->value || memory_compare(s1->value, (void *) text1, 13) != 0) {
        test_case_record_error(&StringFromCStrAllocTest, TEST_RESULT_INCORRECT_VALUE,
            "failed to allocate string from valid C string",
            "\t-- Expected length: 13\n\t-- Actual length: %llu", s1 ? s1->length : 0);
    }
    if (s1) { 
        memory_free(s1->value); 
        memory_free(s1); 
    }

    // Empty string
    String *s2 = string_from_cstr_alloc(text2, 0);
    if (!s2 || s2->length != 0 || !s2->value) {
        test_case_record_error(&StringFromCStrAllocTest, TEST_RESULT_INCORRECT_VALUE,
            "failed to allocate string from empty C string",
            "\t-- Expected length: 0\n\t-- Actual length: %llu", s2 ? s2->length : 0);
    }
    if (s2) { 
        memory_free(s2->value); 
        memory_free(s2); 
    }

    // NULL input
    String *s3 = string_from_cstr_alloc(text3, 0);
    if (s3 != NULL) {
        test_case_record_error(&StringFromCStrAllocTest, TEST_RESULT_INCORRECT_VALUE,
            "NULL input should return NULL",
            "\t-- Actual: %p", s3);
    }

    return 0;
}

TEST(StringFromCStrInto, "String", "Conversion") {
    const char *text1 = "Hello, World!";
    const char *text3 = NULL;

    // Allocate buffers for testing
    String s1;
    s1.value = malloc(20);
    s1.length = 20;

    String s2;
    s2.value = malloc(5);
    s2.length = 5;

    String s3;
    s3.value = NULL;
    s3.length = 10;

    // Normal copy: text shorter than buffer
    string_from_cstr_into(text1, 13, &s1);
    if (s1.length != 13 || memory_compare(s1.value, text1, 13) != 0) {
        test_case_record_error(&StringFromCStrIntoTest, TEST_RESULT_INCORRECT_VALUE,
            "failed to copy C string into larger buffer",
            "\t-- Expected length: 13\n\t-- Actual length: %llu", s1.length);
    }

    // Copy into smaller buffer: only copy fits
    string_from_cstr_into(text1, 13, &s2);
    if (s2.length != 5 || memory_compare(s2.value, text1, 5) != 0) {
        test_case_record_error(&StringFromCStrIntoTest, TEST_RESULT_INCORRECT_VALUE,
            "failed to copy C string into smaller buffer",
            "\t-- Expected length: 5\n\t-- Actual length: %llu", s2.length);
    }

    // NULL input string: should do nothing
    string_from_cstr_into(text3, 0, &s1);  // no crash expected
    // NULL String pointer: should do nothing
    string_from_cstr_into(text1, 13, NULL); // no crash expected
    // String with NULL value: should do nothing
    string_from_cstr_into(text1, 13, &s3); // no crash expected

    // Free allocated memory
    free(s1.value);
    free(s2.value);

    return 0;
}

TEST(StringToCStrAlloc, "String", "Conversion") {
    const char *text1 = "Hello, World!";
    const char *text3 = NULL;

    // Normal string
    String s1;
    s1.value = malloc(13);
    memory_copy(s1.value, (void *) text1, 13);
    s1.length = 13;

    char *cstr1 = string_to_cstr_alloc(&s1);
    if (!cstr1 || strcmp(cstr1, text1) != 0) {
        test_case_record_error(&StringToCStrAllocTest, TEST_RESULT_INCORRECT_VALUE,
            "failed to convert string to C string",
            "\t-- Expected: %s\n\t-- Actual: %s", text1, cstr1 ? cstr1 : "(null)");
    }
    free(cstr1);
    free(s1.value);

    // Empty string
    String s2;
    s2.value = malloc(0);
    s2.length = 0;

    char *cstr2 = string_to_cstr_alloc(&s2);
    if (!cstr2 || cstr2[0] != '\0') {
        test_case_record_error(&StringToCStrAllocTest, TEST_RESULT_INCORRECT_VALUE,
            "failed to convert empty string to C string",
            "\t-- Expected: empty string\n\t-- Actual: %s", cstr2 ? cstr2 : "(null)");
    }
    free(cstr2);
    free(s2.value);

    // NULL string pointer
    char *cstr3 = string_to_cstr_alloc((String *) text3); // intentionally NULL
    if (cstr3 != NULL) {
        test_case_record_error(&StringToCStrAllocTest, TEST_RESULT_INCORRECT_VALUE,
            "NULL input should return NULL",
            "\t-- Actual: %p", cstr3);
    }

    return 0;
}

TEST(StringToCStrInto, "String", "Conversion") {
    const char *text1 = "Hello, World!";

    // Normal string
    String s1;
    s1.value = malloc(13);
    memory_copy(s1.value, (void *) text1, 13);
    s1.length = 13;

    char buffer1[20];
    string_to_cstr_into(&s1, buffer1, sizeof(buffer1));
    if (strcmp(buffer1, text1) != 0) {
        test_case_record_error(&StringToCStrIntoTest, TEST_RESULT_INCORRECT_VALUE,
            "failed to copy string into larger buffer",
            "\t-- Expected: %s\n\t-- Actual: %s", text1, buffer1);
    }

    // Buffer smaller than string
    String s2;
    s2.value = malloc(13);
    memory_copy(s2.value, (void *) text1, 13);
    s2.length = 13;

    char buffer2[6]; // can hold 5 chars + null
    string_to_cstr_into(&s2, buffer2, sizeof(buffer2));
    if (strncmp(buffer2, text1, 5) != 0 || buffer2[5] != '\0') {
        test_case_record_error(&StringToCStrIntoTest, TEST_RESULT_INCORRECT_VALUE,
            "failed to truncate string to fit smaller buffer",
            "\t-- Expected first 5 chars: 'Hello'\n\t-- Actual: '%s'", buffer2);
    }

    // Empty string
    String s3;
    s3.value = malloc(0);
    s3.length = 0;

    char buffer3[5];
    string_to_cstr_into(&s3, buffer3, sizeof(buffer3));
    if (buffer3[0] != '\0') {
        test_case_record_error(&StringToCStrIntoTest, TEST_RESULT_INCORRECT_VALUE,
            "empty string should result in empty C string",
            "\t-- Actual: '%s'", buffer3);
    }

    // NULL string or buffer: should not crash
    string_to_cstr_into(NULL, buffer1, sizeof(buffer1));
    string_to_cstr_into(&s1, NULL, sizeof(buffer1));
    string_to_cstr_into(&s1, buffer1, 0);

    // Free memory
    free(s1.value);
    free(s2.value);
    free(s3.value);

    return 0;
}

TEST(StringFromWStr, "String", "Conversion") {
    const wchar *text1 = L"Hello, World!";
    const wchar *text2 = L"";
    const wchar *text3 = NULL;

    // Non-empty string
    String s1 = string_from_wstr(text1, 13);
    wchar *buf1 = string_to_wstr_alloc(&s1);
    if (s1.length == 0 || !s1.value || !buf1 || memory_compare(buf1, text1, 13) != 0) {
        test_case_record_error(&StringFromWStrTest, TEST_RESULT_INCORRECT_VALUE,
            "failed to construct string from valid wchar string",
            "\t-- Expected length: 13\n\t-- Actual length: %llu", s1.length);
    }
    free(s1.value);
    free(buf1);

    // Empty string
    String s2 = string_from_wstr(text2, 0);
    if (s2.length != 0 || !s2.value) {
        test_case_record_error(&StringFromWStrTest, TEST_RESULT_INCORRECT_VALUE,
            "failed to construct string from empty wchar string",
            "\t-- Expected length: 0\n\t-- Actual length: %llu", s2.length);
    }
    free(s2.value);

    // NULL string
    String s3 = string_from_wstr(text3, 0);
    if (s3.length != 0 || s3.value != NULL) {
        test_case_record_error(&StringFromWStrTest, TEST_RESULT_INCORRECT_VALUE,
            "failed to handle NULL wchar string",
            "\t-- Expected: NULL\n\t-- Actual: %p", s3.value);
    }

    return 0;
}

TEST(StringFromWStrAlloc, "String", "Conversion") {
    const wchar *text1 = L"Hello, World!";
    const wchar *text2 = L"";
    const wchar *text3 = NULL;

    // Non-empty string
    String *s1 = string_from_wstr_alloc(text1, 13);
    wchar *buf1 = s1 ? string_to_wstr_alloc(s1) : NULL;
    if (!s1 || s1->length != 13 || !s1->value || !buf1 || memory_compare(buf1, text1, 13) != 0) {
        test_case_record_error(&StringFromWStrAllocTest, TEST_RESULT_INCORRECT_VALUE,
            "failed to allocate string from valid wchar string",
            "\t-- Expected length: 13\n\t-- Actual length: %llu", s1 ? s1->length : 0);
    }
    if (s1) { free(s1->value); free(s1); }
    free(buf1);

    // Empty string
    String *s2 = string_from_wstr_alloc(text2, 0);
    if (!s2 || s2->length != 0 || !s2->value) {
        test_case_record_error(&StringFromWStrAllocTest, TEST_RESULT_INCORRECT_VALUE,
            "failed to allocate string from empty wchar string",
            "\t-- Expected length: 0\n\t-- Actual length: %llu", s2 ? s2->length : 0);
    }
    if (s2) { free(s2->value); free(s2); }

    // NULL input
    String *s3 = string_from_wstr_alloc(text3, 0);
    if (s3 != NULL) {
        test_case_record_error(&StringFromWStrAllocTest, TEST_RESULT_INCORRECT_VALUE,
            "NULL input should return NULL",
            "\t-- Actual: %p", s3);
    }

    return 0;
}

TEST(StringFromWStrInto, "String", "Conversion") {
    const wchar *text1 = L"Hello, World!";
    const wchar *text3 = NULL;

    String s1;
    String s2;
    String s3;

    // Allocate buffers
    s1.value = malloc(20); s1.length = 20;
    s2.value = malloc(5);  s2.length = 5;
    s3.value = NULL;       s3.length = 10;

    // Normal copy: text shorter than buffer
    string_from_wstr_into(text1, 13, &s1);
    if (s1.length != 13) {
        test_case_record_error(&StringFromWStrIntoTest, TEST_RESULT_INCORRECT_VALUE,
            "failed to copy wchar string into larger buffer: wrong length",
            "\t-- Expected length: 13\n\t-- Actual length: %llu", s1.length);
    }
    for (u64 i = 0; i < s1.length; ++i) {
        if (s1.value[i] != text1[i]) {
            test_case_record_error(&StringFromWStrIntoTest, TEST_RESULT_INCORRECT_VALUE,
                "failed to copy wchar string into larger buffer: content mismatch",
                "\t-- At index %llu: expected %lc, actual %lc", i, text1[i], s1.value[i]);
        }
    }

    // Copy into smaller buffer: only copy fits
    string_from_wstr_into(text1, 13, &s2);
    if (s2.length != 5) {
        test_case_record_error(&StringFromWStrIntoTest, TEST_RESULT_INCORRECT_VALUE,
            "failed to copy wchar string into smaller buffer: wrong length",
            "\t-- Expected length: 5\n\t-- Actual length: %llu", s2.length);
    }
    for (u64 i = 0; i < s2.length; ++i) {
        if (s2.value[i] != text1[i]) {
            test_case_record_error(&StringFromWStrIntoTest, TEST_RESULT_INCORRECT_VALUE,
                "failed to copy wchar string into smaller buffer: content mismatch",
                "\t-- At index %llu: expected %lc, actual %lc", i, text1[i], s2.value[i]);
        }
    }

    // NULL input string: should do nothing (no crash)
    string_from_wstr_into(text3, 0, &s1);
    string_from_wstr_into(text1, 13, NULL);
    string_from_wstr_into(text1, 13, &s3);

    free(s1.value);
    free(s2.value);

    return 0;
}

TEST(StringToWStrAlloc, "String", "Conversion") {
    const wchar *text1 = L"Hello, World!";

    // Normal string
    String s1;
    s1.value = malloc(13);
    for (int i = 0; i < 13; ++i) s1.value[i] = (char)text1[i];
    s1.length = 13;

    wchar *wbuf1 = string_to_wstr_alloc(&s1);
    if (!wbuf1 || memory_compare(wbuf1, text1, 13) != 0) {
        test_case_record_error(&StringToWStrAllocTest, TEST_RESULT_INCORRECT_VALUE,
            "failed to convert string to wchar_t array",
            "\t-- Expected: Hello, World!\n\t-- Actual first char: %lc", wbuf1 ? wbuf1[0] : L'?');
    }
    free(wbuf1);
    free(s1.value);

    // Empty string
    String s2;
    s2.value = malloc(0); s2.length = 0;
    wchar *wbuf2 = string_to_wstr_alloc(&s2);
    if (wbuf2 != NULL) {
        test_case_record_error(&StringToWStrAllocTest, TEST_RESULT_INCORRECT_VALUE,
            "empty string should return empty wchar_t buffer", "");
    }
    free(wbuf2);
    free(s2.value);

    // NULL input
    wchar *wbuf3 = string_to_wstr_alloc(NULL);
    if (wbuf3 != NULL) {
        test_case_record_error(&StringToWStrAllocTest, TEST_RESULT_INCORRECT_VALUE,
            "NULL input should return NULL",
            "\t-- Actual: %p", wbuf3);
    }

    return 0;
}

TEST(StringToWStrInto, "String", "Conversion") {
    const wchar *text1 = L"Hello, World!";

    // Normal string
    String s1;
    s1.value = malloc(13);
    for (int i = 0; i < 13; ++i) s1.value[i] = (char)text1[i];
    s1.length = 13;

    wchar buffer1[20];
    string_to_wstr_into(&s1, buffer1, 20);
    if (memory_compare(buffer1, text1, 13) != 0) {
        test_case_record_error(&StringToWStrIntoTest, TEST_RESULT_INCORRECT_VALUE,
            "failed to copy string into larger wchar buffer",
            "\t-- Expected first char: %lc\n\t-- Actual first char: %lc", text1[0], buffer1[0]);
    }

    // Buffer smaller than string
    wchar buffer2[6]; // 5 chars + null
    string_to_wstr_into(&s1, buffer2, 6);
    if (memory_compare(buffer2, text1, 5) != 0) {
        test_case_record_error(&StringToWStrIntoTest, TEST_RESULT_INCORRECT_VALUE,
            "failed to truncate string to fit smaller wchar buffer",
            "\t-- Expected first char: %lc\n\t-- Actual first char: %lc", text1[0], buffer2[0]);
    }

    // NULL string or buffer: should not crash
    string_to_wstr_into(NULL, buffer1, 20);
    string_to_wstr_into(&s1, NULL, 20);
    string_to_wstr_into(&s1, buffer1, 0);

    free(s1.value);

    return 0;
}

TEST(StringFromUTF16, "String", "Conversion") {
    // Sample UTF-16 data: ASCII, BMP, and surrogate pair (😊 U+1F60A)
    char16 text1[] = { 'H','e','l','l','o', 0x0020, 0x0041, 0xD83D, 0xDE0A }; // "Hello A😊"
    char16 text2[] = {0}; // empty
    char16 *text3 = NULL;

    // Non-empty UTF-16
    String s1 = string_from_utf16(text1, sizeof(text1)/sizeof(char16));
    if (s1.length == 0 || !s1.value) {
        test_case_record_error(&StringFromUTF16Test, TEST_RESULT_INCORRECT_VALUE,
            "failed to construct string from valid UTF-16", "");
    }

    // Empty UTF-16
    String s2 = string_from_utf16(text2, 0);
    if (s2.length != 0 || s2.value != NULL) {
        test_case_record_error(&StringFromUTF16Test, TEST_RESULT_INCORRECT_VALUE,
            "failed to construct string from empty UTF-16", "");
    }

    // NULL UTF-16
    String s3 = string_from_utf16(text3, 0);
    if (s3.length != 0 || s3.value != NULL) {
        test_case_record_error(&StringFromUTF16Test, TEST_RESULT_INCORRECT_VALUE,
            "failed to handle NULL UTF-16",
            "\t-- Actual value: %p", s3.value);
    }

    return 0;
}

TEST(StringFromUTF16Alloc, "String", "Conversion") {
    char16 text1[] = { 'T','e','s','t',0x20,0xD83D,0xDE03 }; // "Test 😃"
    char16 text2[] = {0};
    char16 *text3 = NULL;

    // Non-empty UTF-16
    String *s1 = string_from_utf16_alloc(text1, sizeof(text1)/sizeof(char16));
    if (!s1 || s1->length == 0 || !s1->value) {
        test_case_record_error(&StringFromUTF16AllocTest, TEST_RESULT_INCORRECT_VALUE,
            "failed to allocate string from valid UTF-16", "");
    }
    if (s1) { 
		if (s1->value) free(s1->value); 
		free(s1); 
	}

    // Empty UTF-16
    String *s2 = string_from_utf16_alloc(text2, 0);
    if (!s2 || s2->length != 0 || s2->value) {
        test_case_record_error(&StringFromUTF16AllocTest, TEST_RESULT_INCORRECT_VALUE,
            "failed to allocate string from empty UTF-16", "");
    }
    if (s2) { 
		if (s2->value) free(s2->value); 
		free(s2); 
	}

    // NULL input
    String *s3 = string_from_utf16_alloc(text3, 0);
    if (s3 != NULL) {
        test_case_record_error(&StringFromUTF16AllocTest, TEST_RESULT_INCORRECT_VALUE,
            "NULL input should return NULL",
            "\t-- Actual: %p", s3);
    }

    return 0;
}

TEST(StringFromUTF16Into, "String", "Conversion") {
    char16 text1[] = { 'A','B','C',0x20,0xD83D,0xDE09 }; // "ABC 😉"
    char16 *text3 = NULL;

    // Allocate buffers
    String s1; s1.value = malloc(20); s1.length = 20;
    String s2; s2.value = malloc(3);  s2.length = 3;
    String s3; s3.value = NULL;       s3.length = 0;

    // Copy into buffer
    string_from_utf16_into(text1, sizeof(text1)/sizeof(char16), &s1);
    if (s1.length == 0 || !s1.value) {
        test_case_record_error(&StringFromUTF16IntoTest, TEST_RESULT_INCORRECT_VALUE,
            "failed to copy UTF-16 into buffer", "");
    }

    // Copy again into s2 (note: function currently allocates new buffer)
    string_from_utf16_into(text1, sizeof(text1)/sizeof(char16), &s2);
    if (s2.length == 0 || !s2.value) {
        test_case_record_error(&StringFromUTF16IntoTest, TEST_RESULT_INCORRECT_VALUE,
            "failed to copy UTF-16 into smaller buffer", "");
    }

    // NULL input string
    string_from_utf16_into(text3, 0, &s1);  // no crash
    // NULL String pointer
    string_from_utf16_into(text1, sizeof(text1)/sizeof(char16), NULL); // no crash
    // String with NULL value
    string_from_utf16_into(text1, sizeof(text1)/sizeof(char16), &s3); // no crash

    free(s1.value);
    free(s2.value);
    free(s3.value);

    return 0;
}

TEST(StringToUTF16Alloc, "String", "Conversion") {
    
	const char *text = "Hi 😄";
    String s = string_from_cstr(text, strlen(text));

	
    char16 *c16 = string_to_utf16_alloc(&s);
    
	if (!c16) {
        test_case_record_error(&StringToUTF16AllocTest, TEST_RESULT_INCORRECT_VALUE,
            "failed to allocate UTF-16 from String", "");
    }
    if (c16[0] != 'H' || c16[1] != 'i') {
        test_case_record_error(&StringToUTF16AllocTest, TEST_RESULT_INCORRECT_VALUE,
            "UTF-16 encoding incorrect at start", "");
    }
	
    free(c16);
    return 0;
}

TEST(StringToUTF16Into, "String", "Conversion") {
    const char *text = "Hello 😉";
    String s = string_from_cstr(text, 7);
    char16 buffer[10] = {0};

    string_to_utf16_into(&s, buffer, 10);
    if (buffer[0] != 'H' || buffer[5] == 0) {
        test_case_record_error(&StringToUTF16IntoTest, TEST_RESULT_INCORRECT_VALUE,
            "failed to copy String into UTF-16 buffer", "");
    }

    return 0;
}

//=============================================================================
// Math Module
//=============================================================================

// -- Utilities Component -----------------------------------------------------

TEST(UtilsMax, "Math", "Utils") {
    int a = 5, b = 3;
    int result = xtd_max(a, b);
    if (result != a) {
        test_case_record_error(&UtilsMaxTest, TEST_RESULT_INCORRECT_VALUE,
            "max of integers incorrect",
            "\t-- Expected: %d\n\t-- Actual: %d", a, result);
    }

    float fa = 3.14f, fb = 2.71f;
    float fresult = xtd_max(fa, fb);
    if (fresult != fa) {
        test_case_record_error(&UtilsMaxTest, TEST_RESULT_INCORRECT_VALUE,
            "max of floats incorrect",
            "\t-- Expected: %.2f\n\t-- Actual: %.2f", fa, fresult);
    }

    // Test equal values
    int equal_result = xtd_max(5, 5);
    if (equal_result != 5) {
        test_case_record_error(&UtilsMaxTest, TEST_RESULT_INCORRECT_VALUE,
            "max of equal values incorrect",
            "\t-- Expected: 5\n\t-- Actual: %d", equal_result);
    }
    return 0;
}

TEST(UtilsMin, "Math", "Utils") {
    int a = 5, b = 3;
    int result = xtd_min(a, b);
    if (result != b) {
        test_case_record_error(&UtilsMinTest, TEST_RESULT_INCORRECT_VALUE,
            "min of integers incorrect",
            "\t-- Expected: %d\n\t-- Actual: %d", b, result);
    }

    float fa = 3.14f, fb = 2.71f;
    float fresult = xtd_min(fa, fb);
    if (fresult != fb) {
        test_case_record_error(&UtilsMinTest, TEST_RESULT_INCORRECT_VALUE,
            "min of floats incorrect",
            "\t-- Expected: %.2f\n\t-- Actual: %.2f", fb, fresult);
    }

    // Test equal values
    int equal_result = xtd_min(5, 5);
    if (equal_result != 5) {
        test_case_record_error(&UtilsMinTest, TEST_RESULT_INCORRECT_VALUE,
            "min of equal values incorrect",
            "\t-- Expected: 5\n\t-- Actual: %d", equal_result);
    }
    return 0;
}

TEST(UtilsIsBetween, "Math", "Utils") {
    int value = 5;
    int lower = 3;
    int upper = 7;

    bool result = xtd_is_between(value, lower, upper);
    if (!result) {
        test_case_record_error(&UtilsIsBetweenTest, TEST_RESULT_INCORRECT_VALUE,
            "value should be between bounds",
            "\t-- Value: %d, Lower: %d, Upper: %d", value, lower, upper);
    }

    // Test boundary conditions
    bool lower_bound = xtd_is_between(3, 3, 7);
    if (!lower_bound) {
        test_case_record_error(&UtilsIsBetweenTest, TEST_RESULT_INCORRECT_VALUE,
            "lower boundary should be inclusive",
            "\t-- Value: 3, Lower: 3, Upper: 7");
    }

    bool upper_bound = xtd_is_between(7, 3, 7);
    if (!upper_bound) {
        test_case_record_error(&UtilsIsBetweenTest, TEST_RESULT_INCORRECT_VALUE,
            "upper boundary should be inclusive",
            "\t-- Value: 7, Lower: 3, Upper: 7");
    }

    // Test outside bounds
    bool below = xtd_is_between(2, 3, 7);
    if (below) {
        test_case_record_error(&UtilsIsBetweenTest, TEST_RESULT_INCORRECT_VALUE,
            "value below range should return false",
            "\t-- Value: 2, Lower: 3, Upper: 7");
    }

    bool above = xtd_is_between(8, 3, 7);
    if (above) {
        test_case_record_error(&UtilsIsBetweenTest, TEST_RESULT_INCORRECT_VALUE,
            "value above range should return false",
            "\t-- Value: 8, Lower: 3, Upper: 7");
    }

    // Test floating point
    float fvalue = 3.5f;
    bool float_result = xtd_is_between(fvalue, 3.0f, 4.0f);
    if (!float_result) {
        test_case_record_error(&UtilsIsBetweenTest, TEST_RESULT_INCORRECT_VALUE,
            "float value should be between bounds",
            "\t-- Value: %.1f, Lower: %.1f, Upper: %.1f", fvalue, 3.0f, 4.0f);
    }
    return 0;
}

TEST(UtilsIsWithinMargin, "Math", "Utils") {
    
	int value;
    int target = 10;
    int margin = 3;

	value = 9;
    bool result = xtd_is_within_margin(value, target, margin);
    if (!result) {
        test_case_record_error(&UtilsIsWithinMarginTest, TEST_RESULT_INCORRECT_VALUE,
            "Value was within margin of target but result was false",
            "\t-- Value: %d, Target: %d, Margin: %d\n\t-- Result: %d", value, target, margin, result);
    }

	value = 11;
	result = xtd_is_within_margin(value, target, margin);
	if (!result) {
        test_case_record_error(&UtilsIsWithinMarginTest, TEST_RESULT_INCORRECT_VALUE,
            "Value was within margin of target but result was false",
            "\t-- Value: %d, Target: %d, Margin: %d\n\t-- Result: %d", value, target, margin, result);
    }

	value = 6;
	result = xtd_is_within_margin(value, target, margin);
	if (result) {
        test_case_record_error(&UtilsIsWithinMarginTest, TEST_RESULT_INCORRECT_VALUE,
            "Value was outside the margin but result was true",
            "\t-- Value: %d, Target: %d, Margin: %d\n\t-- Result: %d", value, target, margin, result);
    }

	value = 14;
	result = xtd_is_within_margin(value, target, margin);
	if (result) {
        test_case_record_error(&UtilsIsWithinMarginTest, TEST_RESULT_INCORRECT_VALUE,
            "Value was outside the margin but result was true",
            "\t-- Value: %d, Target: %d, Margin: %d\n\t-- Result: %d", value, target, margin, result);
    }

	value = 7;
	result = xtd_is_within_margin(value, target, margin);
	if (!result) {
        test_case_record_error(&UtilsIsWithinMarginTest, TEST_RESULT_INCORRECT_VALUE,
            "Value was within margin of target but result was false",
            "\t-- Value: %d, Target: %d, Margin: %d\n\t-- Result: %d", value, target, margin, result);
    }

	value = 13;
	result = xtd_is_within_margin(value, target, margin);
	if (!result) {
        test_case_record_error(&UtilsIsWithinMarginTest, TEST_RESULT_INCORRECT_VALUE,
            "Value was within margin of target but result was false",
            "\t-- Value: %d, Target: %d, Margin: %d\n\t-- Result: %d", value, target, margin, result);
    }

    return 0;
}
TEST(UtilsIsPowerOfTwo, "Math", "Utils") {
    // Test powers of two
    int powers[] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024};
    int num_powers = sizeof(powers) / sizeof(powers[0]);
    
    for (int i = 0; i < num_powers; i++) {
        bool result = xtd_is_power_of_two(powers[i]);
        if (!result) {
            test_case_record_error(&UtilsIsPowerOfTwoTest, TEST_RESULT_INCORRECT_VALUE,
                "power of two not recognized",
                "\t-- Value: %d should be power of two", powers[i]);
        }
    }

    // Test non-powers of two
    int non_powers[] = {3, 5, 6, 7, 9, 10, 11, 12, 13, 14, 15};
    int num_non_powers = sizeof(non_powers) / sizeof(non_powers[0]);
    
    for (int i = 0; i < num_non_powers; i++) {
        bool result = xtd_is_power_of_two(non_powers[i]);
        if (result) {
            test_case_record_error(&UtilsIsPowerOfTwoTest, TEST_RESULT_INCORRECT_VALUE,
                "non-power of two incorrectly identified",
                "\t-- Value: %d should not be power of two", non_powers[i]);
        }
    }

    // Test zero
    bool zero_result = xtd_is_power_of_two(0);
    if (zero_result) {
        test_case_record_error(&UtilsIsPowerOfTwoTest, TEST_RESULT_INCORRECT_VALUE,
            "zero should not be power of two", "");
    }

    // Test negative numbers
    bool neg_result = xtd_is_power_of_two(-8);
    if (neg_result) {
        test_case_record_error(&UtilsIsPowerOfTwoTest, TEST_RESULT_INCORRECT_VALUE,
            "negative number should not be power of two",
            "\t-- Value: -8");
    }
    return 0;
}

TEST(UtilsSquare, "Math", "Utils") {
    int a = 5;
    int result = xtd_sq(a);
    if (result != 25) {
        test_case_record_error(&UtilsSquareTest, TEST_RESULT_INCORRECT_VALUE,
            "square of integer incorrect",
            "\t-- Expected: 25\n\t-- Actual: %d", result);
    }

    float fa = 3.0f;
    float fresult = xtd_sq(fa);
    if (fresult != 9.0f) {
        test_case_record_error(&UtilsSquareTest, TEST_RESULT_INCORRECT_VALUE,
            "square of float incorrect",
            "\t-- Expected: 9.0\n\t-- Actual: %.1f", fresult);
    }

    // Test negative number
    int neg = -4;
    int neg_result = xtd_sq(neg);
    if (neg_result != 16) {
        test_case_record_error(&UtilsSquareTest, TEST_RESULT_INCORRECT_VALUE,
            "square of negative number incorrect",
            "\t-- Expected: 16\n\t-- Actual: %d", neg_result);
    }

    // Test zero
    int zero_result = xtd_sq(0);
    if (zero_result != 0) {
        test_case_record_error(&UtilsSquareTest, TEST_RESULT_INCORRECT_VALUE,
            "square of zero incorrect",
            "\t-- Expected: 0\n\t-- Actual: %d", zero_result);
    }

    // Test fractional
    float frac = 0.5f;
    float frac_result = xtd_sq(frac);
    if (frac_result != 0.25f) {
        test_case_record_error(&UtilsSquareTest, TEST_RESULT_INCORRECT_VALUE,
            "square of fraction incorrect",
            "\t-- Expected: 0.25\n\t-- Actual: %.2f", frac_result);
    }
    return 0;
}

TEST(UtilsSqrt, "Math", "Utils") {
    float a = 25.0f;
    float result = xtd_sqrt(a);
    if (result != 5.0f) {
        test_case_record_error(&UtilsSqrtTest, TEST_RESULT_INCORRECT_VALUE,
            "square root incorrect",
            "\t-- Expected: 5.0\n\t-- Actual: %.1f", result);
    }

    // Test perfect squares
    float values[] = {1.0f, 4.0f, 9.0f, 16.0f, 36.0f, 49.0f, 64.0f, 81.0f, 100.0f};
    float expected[] = {1.0f, 2.0f, 3.0f, 4.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f};
    int num_values = sizeof(values) / sizeof(values[0]);

    for (int i = 0; i < num_values; i++) {
        float sqrt_result = xtd_sqrt(values[i]);
        if (sqrt_result != expected[i]) {
            test_case_record_error(&UtilsSqrtTest, TEST_RESULT_INCORRECT_VALUE,
                "square root of perfect square incorrect",
                "\t-- Input: %.1f, Expected: %.1f\n\t-- Actual: %.1f", 
                values[i], expected[i], sqrt_result);
        }
    }

    // Test zero
    float zero_result = xtd_sqrt(0.0f);
    if (zero_result != 0.0f) {
        test_case_record_error(&UtilsSqrtTest, TEST_RESULT_INCORRECT_VALUE,
            "square root of zero incorrect",
            "\t-- Expected: 0.0\n\t-- Actual: %.1f", zero_result);
    }

    // Test fractional
    float frac_result = xtd_sqrt(0.25f);
    if (frac_result != 0.5f) {
        test_case_record_error(&UtilsSqrtTest, TEST_RESULT_INCORRECT_VALUE,
            "square root of fraction incorrect",
            "\t-- Expected: 0.5\n\t-- Actual: %.2f", frac_result);
    }
    return 0;
}

// -- Vec2 Component ----------------------------------------------------------

TEST(Vec2Mul, "Math", "Vec2") {
    Vec2u32 a = { .x=3, .y=5 };
    Vec2u32 b = { .x=7, .y=11 };

    Vec2u32 result = { vec2_mul(a, b) };
    if (result.x != (a.x * b.x) || result.y != (a.y * b.y)) {
        test_case_record_error(&Vec2MulTest, TEST_RESULT_INCORRECT_VALUE,
            "product incorrect",
            "\t-- Expected: (%u, %u)\n\t-- Actual: (%u, %u)", a.x * b.x, a.y * b.y, result.x, result.y);
    }
    return 0;
}

TEST(Vec2CastMul, "Math", "Vec2") {
    Vec2u32 a = { .x=3, .y=5 };
    Vec2u32 b = { .x=7, .y=11 };

    Vec2f64 result = { vec2_cast_mul(f64, a, b) };
    if (result.x != (f64)(a.x * b.x) || result.y != (f64)(a.y * b.y)) {
        test_case_record_error(&Vec2CastMulTest, TEST_RESULT_INCORRECT_VALUE,
            "product incorrect",
            "\t-- Expected: (%.2f, %.2f)\n\t-- Actual: (%.2f, %.2f)", (f64)(a.x * b.x), (f64)(a.y * b.y), result.x, result.y);
    }
    if (sizeof(result.x) != sizeof(f64) || sizeof(result.y) != sizeof(f64)) {
        test_case_record_error(&Vec2CastMulTest, TEST_RESULT_INVALID_STATE,
            "vec2_cast_mul type cast incorrect",
            "\t-- Expected element size: %zu\n\t-- Actual element size: %zu", sizeof(f64), sizeof(result.x));
    }
    return 0;
}

TEST(Vec2Add, "Math", "Vec2") {
    Vec2u32 a = { .x=3, .y=5 };
    Vec2u32 b = { .x=7, .y=11 };

    Vec2u32 result = { vec2_add(a, b) };
    if (result.x != (a.x + b.x) || result.y != (a.y + b.y)) {
        test_case_record_error(&Vec2AddTest, TEST_RESULT_INCORRECT_VALUE,
            "sum incorrect",
            "\t-- Expected: (%u, %u)\n\t-- Actual: (%u, %u)", a.x + b.x, a.y + b.y, result.x, result.y);
    }
    return 0;
}

TEST(Vec2CastAdd, "Math", "Vec2") {
    Vec2u32 a = { .x=3, .y=5 };
    Vec2u32 b = { .x=7, .y=11 };

    Vec2f64 result = { vec2_cast_add(f64, a, b) };
	if (result.x != (f64)(a.x + b.x) || result.y != (f64)(a.y + b.y)) {
        test_case_record_error(&Vec2CastAddTest, TEST_RESULT_INCORRECT_VALUE,
            "sum incorrect",
            "\t-- Expected: (%.2f, %.2f)\n\t-- Actual: (%.2f, %.2f)", (f64)(a.x + b.x), (f64)(a.y + b.y), result.x, result.y);
    }
    if (sizeof(result.x) != sizeof(f64) || sizeof(result.y) != sizeof(f64)) {
        test_case_record_error(&Vec2CastAddTest, TEST_RESULT_INVALID_STATE,
            "type cast incorrect",
            "\t-- Expected element sizes: (%zu, %zu)\n\t-- Actual element size: (%zu, %zu)", sizeof(f64), sizeof(f64), sizeof(result.x), sizeof(result.y));
    }
    return 0;
}

TEST(Vec2Sub, "Math", "Vec2") {
    Vec2u32 a = { .x=3, .y=5 };
    Vec2u32 b = { .x=7, .y=11 };

    Vec2u32 result = { vec2_sub(b, a) };
    if (result.x != (b.x - a.x) || result.y != (b.y - a.y)) {
        test_case_record_error(&Vec2SubTest, TEST_RESULT_INCORRECT_VALUE,
            "result incorrect",
            "\t-- Expected: (%u, %u)\n\t-- Actual: (%u, %u)", b.x - a.x, b.y - a.y, result.x, result.y);
    }
    return 0;
}

TEST(Vec2CastSub, "Math", "Vec2") {
    Vec2u32 a = { .x=3, .y=5 };
    Vec2u32 b = { .x=7, .y=11 };

    Vec2f64 result = { vec2_cast_sub(f64, b, a) };
    if (result.x != (f64)(b.x - a.x) || result.y != (f64)(b.y - a.y)) {
        test_case_record_error(&Vec2CastSubTest, TEST_RESULT_INCORRECT_VALUE,
            "result incorrect",
            "\t-- Expected: (%.2f, %.2f)\n\t-- Actual: (%.2f, %.2f)",
            (f64)(b.x - a.x), (f64)(b.y - a.y), result.x, result.y);
    }
    if (sizeof(result.x) != sizeof(f64) || sizeof(result.y) != sizeof(f64)) {
        test_case_record_error(&Vec2CastSubTest, TEST_RESULT_INVALID_STATE,
            "type cast incorrect",
            "\t-- Expected element sizes: (%zu, %zu)\n\t-- Actual element sizes: (%zu, %zu)",
            sizeof(f64), sizeof(f64), sizeof(result.x), sizeof(result.y));
    }
    return 0;
}

TEST(Vec2Min, "Math", "Vec2") {
    Vec2u32 a = { .x=3, .y=5 };
    Vec2u32 b = { .x=7, .y=11 };

    Vec2u32 result = { vec2_min(a, b) };
    if (result.x != a.x || result.y != a.y) {
        test_case_record_error(&Vec2MinTest, TEST_RESULT_INCORRECT_VALUE,
            "result incorrect",
            "\t-- Expected: (%u, %u)\n\t-- Actual: (%u, %u)", a.x, a.y, result.x, result.y);
    }
    return 0;
}

TEST(Vec2CastMin, "Math", "Vec2") {
    Vec2u32 a = { .x=3, .y=5 };
    Vec2u32 b = { .x=7, .y=11 };

    Vec2f64 result = { vec2_cast_min(f64, a, b) };
    if (result.x != (f64)a.x || result.y != (f64)a.y) {
        test_case_record_error(&Vec2CastMinTest, TEST_RESULT_INCORRECT_VALUE,
            "result incorrect",
            "\t-- Expected: (%.2f, %.2f)\n\t-- Actual: (%.2f, %.2f)", (f64)a.x, (f64)a.y, result.x, result.y);
    }
    if (sizeof(result.x) != sizeof(f64) || sizeof(result.y) != sizeof(f64)) {
        test_case_record_error(&Vec2CastMinTest, TEST_RESULT_INVALID_STATE,
            "type cast incorrect",
            "\t-- Expected element sizes: (%zu, %zu)\n\t-- Actual element sizes: (%zu, %zu)",
            sizeof(f64), sizeof(f64), sizeof(result.x), sizeof(result.y));
    }
    return 0;
}

TEST(Vec2Max, "Math", "Vec2") {
    Vec2u32 a = { .x=3, .y=5 };
    Vec2u32 b = { .x=7, .y=11 };

    Vec2u32 result = { vec2_max(a, b) };
    if (result.x != b.x || result.y != b.y) {
        test_case_record_error(&Vec2MaxTest, TEST_RESULT_INCORRECT_VALUE,
            "result incorrect",
            "\t-- Expected: (%u, %u)\n\t-- Actual: (%u, %u)", b.x, b.y, result.x, result.y);
    }
    return 0;
}

TEST(Vec2CastMax, "Math", "Vec2") {
    Vec2u32 a = { .x=3, .y=5 };
    Vec2u32 b = { .x=7, .y=11 };

    Vec2f64 result = { vec2_cast_max(f64, a, b) };
    if (result.x != (f64)b.x || result.y != (f64)b.y) {
        test_case_record_error(&Vec2CastMaxTest, TEST_RESULT_INCORRECT_VALUE,
            "result incorrect",
            "\t-- Expected: (%.2f, %.2f)\n\t-- Actual: (%.2f, %.2f)", (f64)b.x, (f64)b.y, result.x, result.y);
    }
    if (sizeof(result.x) != sizeof(f64) || sizeof(result.y) != sizeof(f64)) {
        test_case_record_error(&Vec2CastMaxTest, TEST_RESULT_INVALID_STATE,
            "type cast incorrect",
            "\t-- Expected element sizes: (%zu, %zu)\n\t-- Actual element sizes: (%zu, %zu)",
            sizeof(f64), sizeof(f64), sizeof(result.x), sizeof(result.y));
    }
    return 0;
}

TEST(Vec2Dot, "Math", "Vec2") {
    Vec2u32 a = { .x=3, .y=5 };
    Vec2u32 b = { .x=7, .y=11 };

    u32 result = vec2_dot(a, b);
    u32 expected = (a.x * b.x) + (a.y * b.y);
    if (result != expected) {
        test_case_record_error(&Vec2DotTest, TEST_RESULT_INCORRECT_VALUE,
            "result incorrect",
            "\t-- Expected: %u\n\t-- Actual: %u", expected, result);
    }
    return 0;
}

TEST(Vec2Len, "Math", "Vec2") {
    Vec2u32 a = { .x=3, .y=5 };

    f32 result = vec2_len(a);
    f32 expected = xtd_sqrt((a.x * a.x) + (a.y * a.y));
    if (result != expected) {
        test_case_record_error(&Vec2LenTest, TEST_RESULT_INCORRECT_VALUE,
            "result incorrect",
            "\t-- Expected: %.4f\n\t-- Actual: %.4f", expected, result);
    }
    return 0;
}

TEST(Vec2LenSq, "Math", "Vec2") {
    Vec2u32 a = { .x=3, .y=5 };

    f32 result = vec2_len_sq(a);
    f32 expected = (a.x * a.x) + (a.y * a.y);
    if (result != expected) {
        test_case_record_error(&Vec2LenSqTest, TEST_RESULT_INCORRECT_VALUE,
            "result incorrect",
            "\t-- Expected: %.2f\n\t-- Actual: %.2f", expected, result);
    }
    return 0;
}

TEST(Vec2Dist, "Math", "Vec2") {
    Vec2u32 a = { .x=3, .y=5 };
    Vec2u32 b = { .x=7, .y=11 };

    f32 result = vec2_dist(a, b);
    f32 expected = xtd_sqrt(xtd_sq(a.x - b.x) + xtd_sq(a.y - b.y));
    if (result != expected) {
        test_case_record_error(&Vec2DistTest, TEST_RESULT_INCORRECT_VALUE,
            "result incorrect",
            "\t-- Expected: %.2f\n\t-- Actual: %.2f", expected, result);
    }
    return 0;
}

TEST(Vec2DistSq, "Math", "Vec2") {
    Vec2u32 a = { .x=3, .y=5 };
    Vec2u32 b = { .x=7, .y=11 };

    f32 result = vec2_dist_sq(a, b);
    f32 expected = xtd_sq(a.x - b.x) + xtd_sq(a.y - b.y);
    if (result != expected) {
        test_case_record_error(&Vec2DistSqTest, TEST_RESULT_INCORRECT_VALUE,
            "result incorrect",
            "\t-- Expected: %.2f\n\t-- Actual: %.2f", expected, result);
    }
    return 0;
}

// -- Vec3 Component ----------------------------------------------------------

TEST(Vec3Mul, "Math", "Vec3") {
    Vec3u32 a = { .x=3, .y=5, .z=2 };
    Vec3u32 b = { .x=7, .y=11, .z=13 };

    Vec3u32 result = { vec3_mul(a, b) };
    if (result.x != a.x * b.x || result.y != a.y * b.y || result.z != a.z * b.z) {
        test_case_record_error(&Vec3MulTest, TEST_RESULT_INCORRECT_VALUE,
            "product incorrect",
            "\t-- Expected: (%u, %u, %u)\n\t-- Actual: (%u, %u, %u)",
            a.x * b.x, a.y * b.y, a.z * b.z,
            result.x, result.y, result.z);
    }
    return 0;
}

TEST(Vec3CastMul, "Math", "Vec3") {
    Vec3u32 a = { .x=3, .y=5, .z=2 };
    Vec3u32 b = { .x=7, .y=11, .z=13 };

    Vec3f64 result = { vec3_cast_mul(f64, a, b) };
    if (result.x != a.x * b.x || result.y != a.y * b.y || result.z != a.z * b.z) {
        test_case_record_error(&Vec3CastMulTest, TEST_RESULT_INCORRECT_VALUE,
            "product incorrect",
            "\t-- Expected: (%u, %u, %u)\n\t-- Actual: (%u, %u, %u)",
            a.x * b.x, a.y * b.y, a.z * b.z,
            result.x, result.y, result.z);
    }
    if (sizeof(result.x) != sizeof(f64) || sizeof(result.y) != sizeof(f64) || sizeof(result.z) != sizeof(f64)) {
        test_case_record_error(&Vec3CastMulTest, TEST_RESULT_INVALID_STATE,
            "type cast incorrect",
            "\t-- Expected element sizes: (%zu, %zu, %zu)\n\t-- Actual element sizes: (%zu, %zu, %zu)",
            sizeof(f64), sizeof(f64), sizeof(f64),
            sizeof(result.x), sizeof(result.y), sizeof(result.z));
    }
    return 0;
}

TEST(Vec3Add, "Math", "Vec3") {
    Vec3u32 a = { .x=3, .y=5, .z=2 };
    Vec3u32 b = { .x=7, .y=11, .z=13 };

    Vec3u32 result = { vec3_add(a, b) };
    if (result.x != a.x + b.x || result.y != a.y + b.y || result.z != a.z + b.z) {
        test_case_record_error(&Vec3AddTest, TEST_RESULT_INCORRECT_VALUE,
            "sum incorrect",
            "\t-- Expected: (%u, %u, %u)\n\t-- Actual: (%u, %u, %u)",
            a.x + b.x, a.y + b.y, a.z + b.z,
            result.x, result.y, result.z);
    }
    return 0;
}

TEST(Vec3CastAdd, "Math", "Vec3") {
    Vec3u32 a = { .x=3, .y=5, .z=2 };
    Vec3u32 b = { .x=7, .y=11, .z=13 };

    Vec3f64 result = { vec3_cast_add(f64, a, b) };
    if (result.x != a.x + b.x || result.y != a.y + b.y || result.z != a.z + b.z) {
        test_case_record_error(&Vec3CastAddTest, TEST_RESULT_INCORRECT_VALUE,
            "sum incorrect",
            "\t-- Expected: (%u, %u, %u)\n\t-- Actual: (%u, %u, %u)",
            a.x + b.x, a.y + b.y, a.z + b.z,
            result.x, result.y, result.z);
    }
    if (sizeof(result.x) != sizeof(f64) || sizeof(result.y) != sizeof(f64) || sizeof(result.z) != sizeof(f64)) {
        test_case_record_error(&Vec3CastAddTest, TEST_RESULT_INVALID_STATE,
            "type cast incorrect",
            "\t-- Expected element sizes: (%zu, %zu, %zu)\n\t-- Actual element sizes: (%zu, %zu, %zu)",
            sizeof(f64), sizeof(f64), sizeof(f64),
            sizeof(result.x), sizeof(result.y), sizeof(result.z));
    }
    return 0;
}

TEST(Vec3Sub, "Math", "Vec3") {
    Vec3u32 a = { .x=3, .y=5, .z=2 };
    Vec3u32 b = { .x=7, .y=11, .z=13 };

    Vec3u32 result = { vec3_sub(b, a) };
    if (result.x != b.x - a.x || result.y != b.y - a.y || result.z != b.z - a.z) {
        test_case_record_error(&Vec3SubTest, TEST_RESULT_INCORRECT_VALUE,
            "result incorrect",
            "\t-- Expected: (%u, %u, %u)\n\t-- Actual: (%u, %u, %u)",
            b.x - a.x, b.y - a.y, b.z - a.z,
            result.x, result.y, result.z);
    }
    return 0;
}

TEST(Vec3CastSub, "Math", "Vec3") {
    Vec3u32 a = { .x=3, .y=5, .z=2 };
    Vec3u32 b = { .x=7, .y=11, .z=13 };

    Vec3f64 result = { vec3_cast_sub(f64, b, a) };
    if (result.x != (f64)(b.x - a.x) || result.y != (f64)(b.y - a.y) || result.z != (f64)(b.z - a.z)) {
        test_case_record_error(&Vec3CastSubTest, TEST_RESULT_INCORRECT_VALUE,
            "result incorrect",
            "\t-- Expected: (%.2f, %.2f, %.2f)\n\t-- Actual: (%.2f, %.2f, %.2f)",
            (f64)(b.x - a.x), (f64)(b.y - a.y), (f64)(b.z - a.z),
            result.x, result.y, result.z);
    }
    if (sizeof(result.x) != sizeof(f64) || sizeof(result.y) != sizeof(f64) || sizeof(result.z) != sizeof(f64)) {
        test_case_record_error(&Vec3CastSubTest, TEST_RESULT_INVALID_STATE,
            "type cast incorrect",
            "\t-- Expected element sizes: (%zu, %zu, %zu)\n\t-- Actual element sizes: (%zu, %zu, %zu)",
            sizeof(f64), sizeof(f64), sizeof(f64),
            sizeof(result.x), sizeof(result.y), sizeof(result.z));
    }
    return 0;
}

TEST(Vec3Min, "Math", "Vec3") {
    Vec3u32 a = { .x=3, .y=5, .z=2 };
    Vec3u32 b = { .x=7, .y=11, .z=13 };

    Vec3u32 result = { vec3_min(a, b) };
    if (result.x != a.x || result.y != a.y || result.z != a.z) {
        test_case_record_error(&Vec3MinTest, TEST_RESULT_INCORRECT_VALUE,
            "result incorrect",
            "\t-- Expected: (%u, %u, %u)\n\t-- Actual: (%u, %u, %u)",
            a.x, a.y, a.z,
            result.x, result.y, result.z);
    }
    return 0;
}

TEST(Vec3CastMin, "Math", "Vec3") {
    Vec3u32 a = { .x=3, .y=5, .z=2 };
    Vec3u32 b = { .x=7, .y=11, .z=13 };

    Vec3f64 result = { vec3_cast_min(f64, a, b) };
    if (result.x != (f64)a.x || result.y != (f64)a.y || result.z != (f64)a.z) {
        test_case_record_error(&Vec3CastMinTest, TEST_RESULT_INCORRECT_VALUE,
            "result incorrect",
            "\t-- Expected: (%.2f, %.2f, %.2f)\n\t-- Actual: (%.2f, %.2f, %.2f)",
            (f64)a.x, (f64)a.y, (f64)a.z,
            result.x, result.y, result.z);
    }
    if (sizeof(result.x) != sizeof(f64) || sizeof(result.y) != sizeof(f64) || sizeof(result.z) != sizeof(f64)) {
        test_case_record_error(&Vec3CastMinTest, TEST_RESULT_INVALID_STATE,
            "type cast incorrect",
            "\t-- Expected element sizes: (%zu, %zu, %zu)\n\t-- Actual element sizes: (%zu, %zu, %zu)",
            sizeof(f64), sizeof(f64), sizeof(f64),
            sizeof(result.x), sizeof(result.y), sizeof(result.z));
    }
    return 0;
}

TEST(Vec3Max, "Math", "Vec3") {
    Vec3u32 a = { .x=3, .y=5, .z=2 };
    Vec3u32 b = { .x=7, .y=11, .z=13 };

    Vec3u32 result = { vec3_max(a, b) };
    if (result.x != b.x || result.y != b.y || result.z != b.z) {
        test_case_record_error(&Vec3MaxTest, TEST_RESULT_INCORRECT_VALUE,
            "result incorrect",
            "\t-- Expected: (%u, %u, %u)\n\t-- Actual: (%u, %u, %u)",
            b.x, b.y, b.z,
            result.x, result.y, result.z);
    }
    return 0;
}

TEST(Vec3CastMax, "Math", "Vec3") {
    Vec3u32 a = { .x=3, .y=5, .z=2 };
    Vec3u32 b = { .x=7, .y=11, .z=13 };

    Vec3f64 result = { vec3_cast_max(f64, a, b) };
    if (result.x != (f64)b.x || result.y != (f64)b.y || result.z != (f64)b.z) {
        test_case_record_error(&Vec3CastMaxTest, TEST_RESULT_INCORRECT_VALUE,
            "result incorrect",
            "\t-- Expected: (%.2f, %.2f, %.2f)\n\t-- Actual: (%.2f, %.2f, %.2f)",
            (f64)b.x, (f64)b.y, (f64)b.z,
            result.x, result.y, result.z);
    }
    if (sizeof(result.x) != sizeof(f64) || sizeof(result.y) != sizeof(f64) || sizeof(result.z) != sizeof(f64)) {
        test_case_record_error(&Vec3CastMaxTest, TEST_RESULT_INVALID_STATE,
            "type cast incorrect",
            "\t-- Expected element sizes: (%zu, %zu, %zu)\n\t-- Actual element sizes: (%zu, %zu, %zu)",
            sizeof(f64), sizeof(f64), sizeof(f64),
            sizeof(result.x), sizeof(result.y), sizeof(result.z));
    }
    return 0;
}

TEST(Vec3Dot, "Math", "Vec3") {
    Vec3u32 a = { .x=3, .y=5, .z=2 };
    Vec3u32 b = { .x=7, .y=11, .z=13 };

    u32 result = vec3_dot(a, b);
    u32 expected = (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
    if (result != expected) {
        test_case_record_error(&Vec3DotTest, TEST_RESULT_INCORRECT_VALUE,
            "result incorrect",
            "\t-- Expected: %u\n\t-- Actual: %u", expected, result);
    }
    return 0;
}

TEST(Vec3Len, "Math", "Vec3") {
    Vec3u32 a = { .x=3, .y=5, .z=2 };

    f32 result = vec3_len(a);
    f32 expected = xtd_sqrt((a.x * a.x) + (a.y * a.y) + (a.z * a.z));
    if (result != expected) {
        test_case_record_error(&Vec3LenTest, TEST_RESULT_INCORRECT_VALUE,
            "result incorrect",
            "\t-- Expected: %.4f\n\t-- Actual: %.4f", expected, result);
    }
    return 0;
}

TEST(Vec3LenSq, "Math", "Vec3") {
    Vec3u32 a = { .x=3, .y=5, .z=2 };

    f32 result = vec3_len_sq(a);
    f32 expected = (a.x * a.x) + (a.y * a.y) + (a.z * a.z);
    if (result != expected) {
        test_case_record_error(&Vec3LenSqTest, TEST_RESULT_INCORRECT_VALUE,
            "result incorrect",
            "\t-- Expected: %.2f\n\t-- Actual: %.2f", expected, result);
    }
    return 0;
}

TEST(Vec3Dist, "Math", "Vec3") {
    Vec3u32 a = { .x=3, .y=5, .z=2 };
    Vec3u32 b = { .x=7, .y=11, .z=13 };

    f32 result = vec3_dist(a, b);
    f32 expected = xtd_sqrt(xtd_sq(a.x - b.x) + xtd_sq(a.y - b.y) + xtd_sq(a.z - b.z));
    if (result != expected) {
        test_case_record_error(&Vec3DistTest, TEST_RESULT_INCORRECT_VALUE,
            "result incorrect",
            "\t-- Expected: %.2f\n\t-- Actual: %.2f", expected, result);
    }
    return 0;
}

TEST(Vec3DistSq, "Math", "Vec3") {
    Vec3u32 a = { .x=3, .y=5, .z=2 };
    Vec3u32 b = { .x=7, .y=11, .z=13 };

    f32 result = vec3_dist_sq(a, b);
    f32 expected = xtd_sq(a.x - b.x) + xtd_sq(a.y - b.y) + xtd_sq(a.z - b.z);
    if (result != expected) {
        test_case_record_error(&Vec3DistSqTest, TEST_RESULT_INCORRECT_VALUE,
            "result incorrect",
            "\t-- Expected: %.2f\n\t-- Actual: %.2f", expected, result);
    }
    return 0;
}

// -- Matrix Component --------------------------------------------------------

// TODO

TEST(MatrixMock, "Math", "Matrix") {
	return 0;
}

// -- DSP Component -----------------------------------------------------------

TEST(DspMock, "Math", "DSP") {
	return 0;
}

//=============================================================================
// Binary Module
//=============================================================================

// -- Bit Component -----------------------------------------------------------

// TODO

TEST(BitMock, "Binary", "Bits") {
	return 0;
}

// -- Flag Component ----------------------------------------------------------

TEST(FlagMock, "Binary", "Flags") {
	return 0;
}

//=============================================================================
// Multithreading Module
//=============================================================================

// -- Atomic Component --------------------------------------------------------

TEST(AtomicLoad32, "Multithreading", "Atomics") {
	volatile i32 value = 42;
    i32 loaded = atomic_load_32(&value);
    
    if (loaded != 42) {
        test_case_record_error(&AtomicLoad32Test, TEST_RESULT_INCORRECT_VALUE,
            "wrong value returned",
            "\t-- Expected: 42\n\t-- Actual: %d", loaded);
    }

    return 0;
}

TEST(AtomicLoad64, "Multithreading", "Atomics") {
    volatile i64 value = 0x123456789ABCDEF0LL;
    i64 loaded = atomic_load_64(&value);
    
    if (loaded != 0x123456789ABCDEF0LL) {
        test_case_record_error(&AtomicLoad64Test, TEST_RESULT_INCORRECT_VALUE,
            "wrong value returned",
            "\t-- Expected: %lld\n\t-- Actual: %lld", 0x123456789ABCDEF0LL, loaded);
    }
    return 0;
}

TEST(AtomicStore32, "Multithreading", "Atomics") {
    volatile i32 value = 0;
    atomic_store_32(&value, 123);
    
    if (value != 123) {
        test_case_record_error(&AtomicStore32Test, TEST_RESULT_INCORRECT_VALUE,
            "value not stored correctly",
            "\t-- Expected: 123\n\t-- Actual: %d", (i32)value);
    }
    return 0;
}

TEST(AtomicStore64, "Multithreading", "Atomics") {
    volatile i64 value = 0;
    i64 test_val = 0xDEADBEEFCAFEBABELL;
    atomic_store_64(&value, test_val);
    
    if (value != test_val) {
        test_case_record_error(&AtomicStore64Test, TEST_RESULT_INCORRECT_VALUE,
            "value not stored correctly",
            "\t-- Expected: %lld\n\t-- Actual: %lld", test_val, (i64)value);
    }
    return 0;
}

TEST(AtomicLoadPtr, "Multithreading", "Atomics") {
    void* ptr = (void*)0xDEADBEEF;
    void* loaded = atomic_load_ptr(&ptr);
    
    if (loaded != (void*)0xDEADBEEF) {
        test_case_record_error(&AtomicLoadPtrTest, TEST_RESULT_INCORRECT_VALUE,
            "wrong value returned",
            "\t-- Expected: %p\n\t-- Actual: %p", (void*)0xDEADBEEF, loaded);
    }
    return 0;
}

TEST(AtomicStorePtr, "Multithreading", "Atomics") {
    void* ptr = NULL;
    void* test_val = (void*)0xCAFEBABE;
    atomic_store_ptr(&ptr, test_val);
    
    if (ptr != test_val) {
        test_case_record_error(&AtomicStorePtrTest, TEST_RESULT_INCORRECT_VALUE,
            "value was not stored correctly",
            "\t-- Expected: %p\n\t-- Actual: %p", test_val, ptr);
    }
    return 0;
}

TEST(AtomicCompEx32Success, "Multithreading", "Atomics") {
    volatile i32 value = 10;
    i32 expected = 10;
    i32 desired = 20;
    
    i32 old_val = atomic_comp_ex_32(&value, expected, desired);
    
    if (old_val != expected) {
        test_case_record_error(&AtomicCompEx32SuccessTest, TEST_RESULT_INCORRECT_VALUE,
            "wrong old value returned",
            "\t-- Expected: %d\n\t-- Actual: %d", expected, old_val);
    }
    if (value != desired) {
        test_case_record_error(&AtomicCompEx32SuccessTest, TEST_RESULT_INCORRECT_VALUE,
            "compre exchange did not update value",
            "\t-- Expected: %d\n\t-- Actual: %d", desired, (i32)value);
    }
    return 0;
}

TEST(AtomicCompEx32Failure, "Multithreading", "Atomics") {
    volatile i32 value = 10;
    i32 expected = 15; // Wrong expected value
    i32 desired = 20;
    i32 original = 10;
    
    i32 old_val = atomic_comp_ex_32(&value, expected, desired);
    
    if (old_val != original) {
        test_case_record_error(&AtomicCompEx32FailureTest, TEST_RESULT_INCORRECT_VALUE,
            "wrong value returned on failure",
            "\t-- Expected: %d\n\t-- Actual: %d", original, old_val);
    }
    if (value != original) {
        test_case_record_error(&AtomicCompEx32FailureTest, TEST_RESULT_INCORRECT_VALUE,
            "compare exchange modified value unexpectedly",
            "\t-- Expected: %d\n\t-- Actual: %d", original, (i32)value);
    }
    return 0;
}

TEST(AtomicCompEx64Success, "Multithreading", "Atomics") {
    volatile i64 value = 0x100000000LL;
    i64 expected = 0x100000000LL;
    i64 desired = 0x200000000LL;
    
    i64 old_val = atomic_comp_ex_64(&value, expected, desired);
    
    if (old_val != expected) {
        test_case_record_error(&AtomicCompEx64SuccessTest, TEST_RESULT_INCORRECT_VALUE,
            "wrong value returned",
            "\t-- Expected: %lld\n\t-- Actual: %lld", expected, old_val);
    }
    if (value != desired) {
        test_case_record_error(&AtomicCompEx64SuccessTest, TEST_RESULT_INCORRECT_VALUE,
            "compare exchange did not update value",
            "\t-- Expected: %lld\n\t-- Actual: %lld", desired, (i64)value);
    }
    return 0;
}

TEST(AtomicCompExPtr, "Multithreading", "Atomics") {
    int dummy1 = 1, dummy2 = 2;
    void* volatile ptr = &dummy1;
    void* expected = &dummy1;
    void* desired = &dummy2;
    
    void* old_ptr = atomic_comp_ex_ptr(&ptr, expected, desired);
    
    if (old_ptr != expected) {
        test_case_record_error(&AtomicCompExPtrTest, TEST_RESULT_INCORRECT_VALUE,
            "wrong value returned",
            "\t-- Expected: %p\n\t-- Actual: %p", expected, old_ptr);
    }
    if (ptr != desired) {
        test_case_record_error(&AtomicCompExPtrTest, TEST_RESULT_INCORRECT_VALUE,
            "compare exchange did not update pointer",
            "\t-- Expected: %p\n\t-- Actual: %p", desired, ptr);
    }
    return 0;
}

TEST(AtomicEx32, "Multithreading", "Atomics") {
    volatile i32 value = 100;
    i32 new_val = 200;
    
    i32 old_val = atomic_ex_32(&value, new_val);
    
    if (old_val != 100) {
        test_case_record_error(&AtomicEx32Test, TEST_RESULT_INCORRECT_VALUE,
            "wrong old value returned",
            "\t-- Expected: 100\n\t-- Actual: %d", old_val);
    }
    if (value != new_val) {
        test_case_record_error(&AtomicEx32Test, TEST_RESULT_INCORRECT_VALUE,
            "exchange did not update value",
            "\t-- Expected: %d\n\t-- Actual: %d", new_val, (i32)value);
    }
    return 0;
}

TEST(AtomicEx64, "Multithreading", "Atomics") {
    volatile i64 value = 0x300000000LL;
    i64 new_val = 0x400000000LL;
    
    i64 old_val = atomic_ex_64(&value, new_val);
    
    if (old_val != 0x300000000LL) {
        test_case_record_error(&AtomicEx64Test, TEST_RESULT_INCORRECT_VALUE,
            "wrong value returned",
            "\t-- Expected: %lld\n\t-- Actual: %lld", 0x300000000LL, old_val);
    }
    if (value != new_val) {
        test_case_record_error(&AtomicEx64Test, TEST_RESULT_INCORRECT_VALUE,
            "exchange did not update value",
            "\t-- Expected: %lld\n\t-- Actual: %lld", new_val, (i64)value);
    }
    return 0;
}

TEST(AtomicExPtr, "Multithreading", "Atomics") {
    int dummy1 = 1, dummy2 = 2;
    void* volatile ptr = &dummy1;
    void* new_ptr = &dummy2;
    
    void* old_ptr = atomic_ex_ptr(&ptr, new_ptr);
    
    if (old_ptr != &dummy1) {
        test_case_record_error(&AtomicExPtrTest, TEST_RESULT_INCORRECT_VALUE,
            "wrong value returned",
            "\t-- Expected: %p\n\t-- Actual: %p", (void*)&dummy1, old_ptr);
    }
    if (ptr != new_ptr) {
        test_case_record_error(&AtomicExPtrTest, TEST_RESULT_INCORRECT_VALUE,
            "exchange did not update pointer",
            "\t-- Expected: %p\n\t-- Actual: %p", new_ptr, ptr);
    }
    return 0;
}

TEST(AtomicFetchAdd32, "Multithreading", "Atomics") {
    volatile i32 value = 50;
    i32 add_val = 25;
    
    i32 old_val = atomic_fetch_add_32(&value, add_val);
    
    if (old_val != 50) {
        test_case_record_error(&AtomicFetchAdd32Test, TEST_RESULT_INCORRECT_VALUE,
            "wrong value returned",
            "\t-- Expected: 50\n\t-- Actual: %d", old_val);
    }
    if (value != 75) {
        test_case_record_error(&AtomicFetchAdd32Test, TEST_RESULT_INCORRECT_VALUE,
            "fetch add did not add correctly",
            "\t-- Expected: 75\n\t-- Actual: %d", (i32)value);
    }
    return 0;
}

TEST(AtomicFetchAdd64, "Multithreading", "Atomics") {
    volatile i64 value = 0x500000000LL;
    i64 add_val = 0x100000000LL;
    
    i64 old_val = atomic_fetch_add_64(&value, add_val);
    
    if (old_val != 0x500000000LL) {
        test_case_record_error(&AtomicFetchAdd64Test, TEST_RESULT_INCORRECT_VALUE,
            "wrong value returned",
            "\t-- Expected: %lld\n\t-- Actual: %lld", 0x500000000LL, old_val);
    }
    if (value != 0x600000000LL) {
        test_case_record_error(&AtomicFetchAdd64Test, TEST_RESULT_INCORRECT_VALUE,
            "fetch add did not add correctly",
            "\t-- Expected: %lld\n\t-- Actual: %lld", 0x600000000LL, (i64)value);
    }
    return 0;
}

TEST(AtomicInc32, "Multithreading", "Atomics") {
    volatile i32 value = 99;
    
    i32 new_val = atomic_inc_32(&value);
    
    if (new_val != 100) {
        test_case_record_error(&AtomicInc32Test, TEST_RESULT_INCORRECT_VALUE,
            "wrong value returned",
            "\t-- Expected: 100\n\t-- Actual: %d", new_val);
    }
    if (value != 100) {
        test_case_record_error(&AtomicInc32Test, TEST_RESULT_INCORRECT_VALUE,
            "did not increment correctly",
            "\t-- Expected: 100\n\t-- Actual: %d", (i32)value);
    }
    return 0;
}

TEST(AtomicInc64, "Multithreading", "Atomics") {
    volatile i64 value = 0x7FFFFFFFFFFFFFFFLL - 1;
    
    i64 new_val = atomic_inc_64(&value);
    
    if (new_val != 0x7FFFFFFFFFFFFFFFLL) {
        test_case_record_error(&AtomicInc64Test, TEST_RESULT_INCORRECT_VALUE,
            "wrong value returned",
            "\t-- Expected: %lld\n\t-- Actual: %lld", 0x7FFFFFFFFFFFFFFFLL, new_val);
    }
    if (value != 0x7FFFFFFFFFFFFFFFLL) {
        test_case_record_error(&AtomicInc64Test, TEST_RESULT_INCORRECT_VALUE,
            "did not increment correctly",
            "\t-- Expected: %lld\n\t-- Actual: %lld", 0x7FFFFFFFFFFFFFFFLL, (i64)value);
    }
    return 0;
}

TEST(AtomicDec32, "Multithreading", "Atomics") {
    volatile i32 value = 101;
    
    i32 new_val = atomic_dec_32(&value);
    
    if (new_val != 100) {
        test_case_record_error(&AtomicDec32Test, TEST_RESULT_INCORRECT_VALUE,
            "wrong value returned",
            "\t-- Expected: 100\n\t-- Actual: %d", new_val);
    }
    if (value != 100) {
        test_case_record_error(&AtomicDec32Test, TEST_RESULT_INCORRECT_VALUE,
            "did not decrement correctly",
            "\t-- Expected: 100\n\t-- Actual: %d", (i32)value);
    }
    return 0;
}

TEST(AtomicDec64, "Multithreading", "Atomics") {
    volatile i64 value = 1000;
    
    i64 new_val = atomic_dec_64(&value);
    
    if (new_val != 999) {
        test_case_record_error(&AtomicDec64Test, TEST_RESULT_INCORRECT_VALUE,
            "wrong value returned",
            "\t-- Expected: 999\n\t-- Actual: %lld", new_val);
    }
    if (value != 999) {
        test_case_record_error(&AtomicDec64Test, TEST_RESULT_INCORRECT_VALUE,
            "did not decrement correctly",
            "\t-- Expected: 999\n\t-- Actual: %lld", (i64)value);
    }
    return 0;
}

TEST(AtomicFetchOr32, "Multithreading", "Atomics") {
    volatile i32 value = 0x0F0F;
    i32 mask = 0xF0F0;
    
    i32 old_val = atomic_fetch_or_32(&value, mask);
    
    if (old_val != 0x0F0F) {
        test_case_record_error(&AtomicFetchOr32Test, TEST_RESULT_INCORRECT_VALUE,
            "wrong value returned",
            "\t-- Expected: 0x%X\n\t-- Actual: 0x%X", 0x0F0F, old_val);
    }
    if (value != 0xFFFF) {
        test_case_record_error(&AtomicFetchOr32Test, TEST_RESULT_INCORRECT_VALUE,
            "did not OR correctly",
            "\t-- Expected: 0x%X\n\t-- Actual: 0x%X", 0xFFFF, (i32)value);
    }
    return 0;
}

TEST(AtomicFetchOr64, "Multithreading", "Atomics") {
    volatile i64 value = 0x0F0F0F0F0F0F0F0FLL;
    i64 mask = 0xF0F0F0F0F0F0F0F0LL;
    
    i64 old_val = atomic_fetch_or_64(&value, mask);
    
    if (old_val != 0x0F0F0F0F0F0F0F0FLL) {
        test_case_record_error(&AtomicFetchOr64Test, TEST_RESULT_INCORRECT_VALUE,
            "wrong value returned",
            "\t-- Expected: 0x%llX\n\t-- Actual: 0x%llX", 0x0F0F0F0F0F0F0F0FLL, old_val);
    }
    if (value != 0xFFFFFFFFFFFFFFFFLL) {
        test_case_record_error(&AtomicFetchOr64Test, TEST_RESULT_INCORRECT_VALUE,
            "did not OR correctly",
            "\t-- Expected: 0x%llX\n\t-- Actual: 0x%llX", 0xFFFFFFFFFFFFFFFFLL, (i64)value);
    }
    return 0;
}

TEST(AtomicFetchAnd32, "Multithreading", "Atomics") {
    volatile i32 value = 0xFFFF;
    i32 mask = 0x0F0F;
    
    i32 old_val = atomic_fetch_and_32(&value, mask);
    
    if (old_val != 0xFFFF) {
        test_case_record_error(&AtomicFetchAnd32Test, TEST_RESULT_INCORRECT_VALUE,
            "wrong value returned",
            "\t-- Expected: 0x%X\n\t-- Actual: 0x%X", 0xFFFF, old_val);
    }
    if (value != 0x0F0F) {
        test_case_record_error(&AtomicFetchAnd32Test, TEST_RESULT_INCORRECT_VALUE,
            "did not AND correctly",
            "\t-- Expected: 0x%X\n\t-- Actual: 0x%X", 0x0F0F, (i32)value);
    }
    return 0;
}

TEST(AtomicFetchAnd64, "Multithreading", "Atomics") {
    volatile i64 value = 0xFFFFFFFFFFFFFFFFLL;
    i64 mask = 0x0F0F0F0F0F0F0F0FLL;
    
    i64 old_val = atomic_fetch_and_64(&value, mask);
    
    if (old_val != 0xFFFFFFFFFFFFFFFFLL) {
        test_case_record_error(&AtomicFetchAnd64Test, TEST_RESULT_INCORRECT_VALUE,
            "wrong value returned",
            "\t-- Expected: 0x%llX\n\t-- Actual: 0x%llX", 0xFFFFFFFFFFFFFFFFLL, old_val);
    }
    if (value != 0x0F0F0F0F0F0F0F0FLL) {
        test_case_record_error(&AtomicFetchAnd64Test, TEST_RESULT_INCORRECT_VALUE,
            "did not AND correctly",
            "\t-- Expected: 0x%llX\n\t-- Actual: 0x%llX", 0x0F0F0F0F0F0F0F0FLL, (i64)value);
    }
    return 0;
}

TEST(AtomicFetchXor32, "Multithreading", "Atomics") {
    volatile i32 value = 0xAAAA;
    i32 mask = 0x5555;
    
    i32 old_val = atomic_fetch_xor_32(&value, mask);
    
    if (old_val != 0xAAAA) {
        test_case_record_error(&AtomicFetchXor32Test, TEST_RESULT_INCORRECT_VALUE,
            "wrong value returned",
            "\t-- Expected: 0x%X\n\t-- Actual: 0x%X", 0xAAAA, old_val);
    }
    if (value != 0xFFFF) {
        test_case_record_error(&AtomicFetchXor32Test, TEST_RESULT_INCORRECT_VALUE,
            "did not XOR correctly",
            "\t-- Expected: 0x%X\n\t-- Actual: 0x%X", 0xFFFF, (i32)value);
    }
    return 0;
}

TEST(AtomicFetchXor64, "Multithreading", "Atomics") {
    volatile i64 value = 0xAAAAAAAAAAAAAAAALL;
    i64 mask = 0x5555555555555555LL;
    
    i64 old_val = atomic_fetch_xor_64(&value, mask);
    
    if (old_val != 0xAAAAAAAAAAAAAAAALL) {
        test_case_record_error(&AtomicFetchXor64Test, TEST_RESULT_INCORRECT_VALUE,
            "wrong value returned",
            "\t-- Expected: 0x%llX\n\t-- Actual: 0x%llX", 0xAAAAAAAAAAAAAAAALL, old_val);
    }
    if (value != 0xFFFFFFFFFFFFFFFFLL) {
        test_case_record_error(&AtomicFetchXor64Test, TEST_RESULT_INCORRECT_VALUE,
            "did not XOR correctly",
            "\t-- Expected: 0x%llX\n\t-- Actual: 0x%llX", 0xFFFFFFFFFFFFFFFFLL, (i64)value);
    }
    return 0;
}

TEST(AtomicSequentialConsistency, "Multithreading", "Atomics") {
    volatile i32 x = 0, y = 0;
    volatile i32 r1, r2;
    
    // Test basic sequential consistency with simple operations
    atomic_store_32(&x, 1);
    atomic_store_32(&y, 1);
    r1 = atomic_load_32(&x);
    r2 = atomic_load_32(&y);
    
    if (r1 != 1 || r2 != 1) {
        test_case_record_error(&AtomicSequentialConsistencyTest, TEST_RESULT_INCORRECT_VALUE,
            "Sequential operations did not maintain consistency",
            "\t-- Expected: r1=1, r2=1\n\t-- Actual: r1=%d, r2=%d", r1, r2);
    }
    return 0;
}

TEST(AtomicMemoryOrdering, "Multithreading", "Atomics") {
	volatile i32 flag = 0;
    volatile i32 data = 0;
    
    atomic_store_32(&data, 42);
    atomic_barrier_release();
    atomic_store_32(&flag, 1);
    
    atomic_barrier_acquire();
    i32 flag_val = atomic_load_32(&flag);
    i32 data_val = atomic_load_32(&data);
    
    if (flag_val == 1 && data_val != 42) {
        test_case_record_error(&AtomicMemoryOrderingTest, TEST_RESULT_INCORRECT_VALUE,
            "Memory ordering not preserved",
            "\t-- Expected: data=42 when flag=1\n\t-- Actual: data=%d when flag=%d", 
            data_val, flag_val);
    }
    return 0;
}

// -- Mutex Component ---------------------------------------------------------

TEST(MutexMock, "Multithreading", "Mutex") {
	return 0;
}

// -- Thread Component --------------------------------------------------------

TEST(TheadMock, "Multithreading", "Thread") {
	return 0;
}

// -- Shared Queue Component --------------------------------------------------

TEST(SharedQueueMock, "Multithreading", "SharedQueue") {
	return 0;
}

int main (void) {
	xtd_run_all_tests();
    return 0;
}
