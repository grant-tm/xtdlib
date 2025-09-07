#ifndef XTDLIB_H
#define XTDLIB_H

// -- Standard Library Imports ------------------------------------------------
#include <stdlib.h> // malloc
#include <stdio.h> // printf
#include <stdint.h> // int types
#include <uchar.h> // char types
#include <stdbool.h> // bool
#include <stdalign.h> // alignof, alignas
#include <math.h> // TODO: replace sqrt & delete this

// -- Windows Imports ---------------------------------------------------------
#if defined(_WIN32) || defined(_WIN64)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <avrt.h>
	#include <intrin.h>
#endif

//=============================================================================
// STYLE DECLARATIONS
//=============================================================================

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef int8_t  b8;
typedef int16_t b16;
typedef int32_t b32;
typedef int64_t b64;
typedef unsigned char byte;

typedef float   f32;
typedef double  f64;

typedef wchar_t wchar;
typedef char16_t char16;
typedef char32_t char32;

//=============================================================================
// TESTING DECLARATIONS
//=============================================================================

typedef enum TestLoggingVerbosity {
	TEST_LOGGING_VERBOSITY_LOW = 0,
	TEST_LOGGING_VERBOSITY_MEDIUM,
	TEST_LOGGING_VERBOSITY_HIGH,
	NUM_TEST_LOGGING_VERBOSITY_OPTIONS
} TestLoggingVerbosity;

#ifndef XTD_TEST_LOGGING_VERBOSITY
	#define XTD_TEST_LOGGING_VERBOSITY TEST_LOGGING_VERBOSITY_LOW
#endif

typedef u32 (*TestFunction)(void);

typedef enum TestResult {
    //TEST_RESULT_PASS,
	TEST_RESULT_FAIL,		     // general fail case
    TEST_RESULT_ERROR,       	 // exception, segfault, etc.
	TEST_RESULT_INCORRECT_VALUE, // incorrect computation result
    TEST_RESULT_INVALID_STATE,	 // entered invalid state
	TEST_RESULT_OUT_OF_RANGE,    // value outside expected range
	TEST_RESULT_NULL_VALUE,		 // unexpected NULL
    TEST_RESULT_RESOURCE_ERROR,  // file/db/socket missing/failed
    TEST_NOT_IMPLEMENTED,   	 // test not yet written
	NUM_TEST_RESULT_OPTIONS
} TestResult;

typedef struct TestCase {	
	// identifiers
	char *name;
	TestFunction function;
	const char **tags;
	u64 num_tags;
	
	// results
	bool passed;
	u32 num_errors;
	char **errors;
	char **error_details;
} TestCase;

char *test_result_to_string (TestResult result);

void test_case_record_error (TestCase *test_case, TestResult error_type, const char *message, const char *details, ...);

void test_case_run_test (TestCase *test_case);

void test_group_output_errors (TestCase **test_cases, u32 num_test_cases);

void test_group_run_tests (TestCase **test_cases, u32 num_test_cases);

static TestCase **xtd_global_test_collector = NULL;
static unsigned xtd_global_test_collector_num_tests = 0;
static unsigned xtd_global_test_collector_capacity = 0;

void xtd_run_all_tests (void);

static inline void register_test(TestCase *test) {
    if (test == NULL) return;
	if (xtd_global_test_collector_num_tests >= xtd_global_test_collector_capacity) {
        xtd_global_test_collector_capacity = xtd_global_test_collector_capacity ? xtd_global_test_collector_capacity * 2 : 16;
        xtd_global_test_collector = realloc(xtd_global_test_collector, sizeof(TestCase*) * xtd_global_test_collector_capacity);
    }
	xtd_global_test_collector[xtd_global_test_collector_num_tests++] = test;
}

#if defined(_MSC_VER)
    #pragma section(".CRT$XCU",read)
    #define TEST_CONSTRUCTOR(f) \
        static void __cdecl f(void); \
        __declspec(allocate(".CRT$XCU")) void (*f##_)(void) = f; \
        static void __cdecl f(void)
#elif defined(__GNUC__) || defined(__clang__)
    #define TEST_CONSTRUCTOR(f) \
        static void f(void) __attribute__((constructor)); \
        static void f(void)
#else
    #error "Constructor mechanism not defined for this compiler"
#endif

#define TEST(name, ...) \
    u32 name(void); \
    static const char *name##_tags[] = { __VA_ARGS__ }; \
    static TestCase name##Test = { #name, name, name##_tags, sizeof(name##_tags)/sizeof(name##_tags[0]), false, 0, NULL, NULL }; \
    TEST_CONSTRUCTOR(register_##name) { register_test(&name##Test); } \
    u32 name(void)

//=============================================================================
// SYSTEM DECLARATIONS
//=============================================================================

static inline void 
xtd_exit(int code) {
    #if defined(_WIN32) || defined(_WIN64)
        ExitProcess((UINT)code);
    #else
        exit(code);
    #endif
}

#ifdef XTD_DISABLE_ASSERT
    #define xtd_assert(condition)
    #define xtd_assert_message(condition, message)
#else
    #define xtd_assert(condition) \
        ((condition) ? (void) 0 : xtd_assert_fail(#condition, NULL, __FILE__, __LINE__))
    #define xtd_assert_message(condition, message) \
        ((condition) ? (void) 0 : xtd_assert_fail(#condition, message, __FILE__, __LINE__))
#endif

static inline void 
xtd_assert_fail (const char *condition, const char *message, const char *file, u32 line) { 
    fprintf(stderr, "\nAssertion failed: (%s), file %s, line %d\n", condition, file, line);
    if (message) { 
        fprintf(stderr, "\t -- %s", message); 
    }
    xtd_exit(1);
}

#define xtd_ignore_unused(x) (void)(x)

//=============================================================================
// MEMORY DECLARATIONS 
//=============================================================================

// -- Memory Management -------------------------------------------------------

static inline void *memory_reserve (u64 size) {
    #if defined(_WIN32) || defined(_WIN64)
        return VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_READWRITE);
    #else
        void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
        return (ptr == MAP_FAILED) ? NULL : ptr;
    #endif
}

static inline void *memory_commit_reserved (void *addr, u64 size) {
    #if defined(_WIN32) || defined(_WIN64)
        return VirtualAlloc(addr, size, MEM_COMMIT, PAGE_READWRITE);
    #else
        int result = mprotect(addr, size, PROT_READ | PROT_WRITE);
        return (result == 0) ? addr : NULL;
    #endif
}

#define memory_allocate(size) malloc(size);
#define memory_free(ptr) free(ptr);

static inline void memory_release (void *addr, u64 size) {
    #if defined(_WIN32) || defined(_WIN64)
        xtd_ignore_unused(size);
        VirtualFree(addr, 0, MEM_RELEASE);
    #else
        munmap(addr, size);
    #endif
}

#define memory_set(addr, value, size) memset((addr), (value), (size));

static inline void memory_zero (void *addr, u64 size) {
    #if defined(_WIN32) || defined(_WIN64)
        ZeroMemory(addr, size);
    #elif defined(__unix__) || defined(__APPLE__)
        bzero(addr, size);
    #else
        memset(addr, 0, size);
    #endif
}

static inline void memory_copy (void *dest, const void *source, u64 size) {
	memcpy(dest, source, size);
}

static inline i32 memory_compare (const void *addr_a, const void *addr_b, u64 size) {
    return size == 0 ? 0 : memcmp(addr_a, addr_b, size);
}

// -- Alignment ---------------------------------------------------------------

#ifndef DefaultAlignment
	#define DefaultAlignment 8
#endif

#define memory_size_of(value) sizeof(value)
#define memory_alignment_of(value) alignof(value)
#define memory_align_as(value) alignas(value)
static inline u64 memory_align_forward (u64 ptr, u64 alignment) {
    u64 modulo = ptr % alignment;
    if (modulo != 0) {
        ptr += (alignment - modulo);
    }
    return ptr;
}

// -- Arena -------------------------------------------------------------------

#define ARENA_DEFAULT_RESERVE_SIZE ((u64)64 * (u64)1024 * (u64)1024) // 64 MB
#define ARENA_COMMIT_CHUNK_SIZE ((u64)64 * (u64)1024) // 64 KB
#define ARENA_CLEAR_TO_ZERO 1

typedef struct Arena {
    u64 reserved;
    u64 committed;
    u64 used;
    void *base;
} Arena;

b32 arena_init(Arena *arena);

static inline void arena_clear(Arena *arena) {
    arena->used = 0;
}

b32 arena_commit_memory (Arena *arena, u64 size);

static inline void arena_release (Arena *arena) {
    memory_release(arena->base, arena->committed);
    arena->base = NULL;
    arena->used = 0;
    arena->committed = 0;
    arena->reserved = 0;
}

void *_arena_push (Arena *arena, u64 size, u64 alignment, b32 clearToZero);

#define arena_push(arena, size) _arena_push(arena, size, DefaultAlignment, true)
#define arena_push_struct(arena, type) (type *)_arena_push(arena, sizeof(type), alignof(type), true)
#define arena_push_array(arena, count, type) (type *)_arena_push(arena, (count) * sizeof(type), alignof(type), true)

//=============================================================================
// STORAGE DECLARATIONS
//=============================================================================

// -- Stack -------------------------------------------------------------------

// struct Node {
//     // Node Data
//     Node *next;
// };
// 
// Node *first;

#define stack_push_n(first, node, next) \
    ((node->next) = (first), (first) = (node)) 

#define stack_pop_n(first, next) \
    ((first) = ((first) ? (first)->next : NULL))

#define stack_push(first, node) stack_push_n(first, node, next)

#define stack_pop(first) stack_pop_n(first, next)

// -- Queue -------------------------------------------------------------------

// struct Node {
//     // Node Data
//     Node *next;
// };
//
// Node *first;
// Node *last;

#define queue_push_n(first, last, node, next) ((first) == NULL ?   \
    ((first) = (last) = (node), (node)->next = NULL) :             \
    ((last)->next = (node), (last) = (node), (node)->next = NULL))

#define queue_pop_n(first, last, next) (((first) == NULL) ? NULL : \
    ((first) == (last)) ? ((last) = NULL, (first) = NULL) :        \
    ((first) = (first)->next))

#define queue_push(first, last, node) queue_push_n(first, last, node, next)

#define queue_pop(first, last) queue_pop_n(first, last, next)

// -- Doubly Linked List ------------------------------------------------------

// struct Node {
//     // Node Data
//     Node *next;
//     Node *prev;
// };
// 
// Node *first;
// Node *last;

#define dll_push_back_np(first, last, node, next, prev) ((first) == NULL) ?     \
    ((first) = node, (last) = node, (node)->next = NULL, (node)->prev = NULL) : \
    ((node)->prev = last, (node)->next = NULL,                                  \
    (last)->next = (node), (last) = (node))
#define dll_push_back(first, last, node) \
    dll_push_back_np(first, last, node, next, prev)


#define dll_push_front_np(first, last, node, next, prev) ((first) == NULL) ?    \
    ((first) = node, (last) = node, (node)->next = NULL, (node)->prev = NULL) : \
    ((node)->next = first, (node)->prev = NULL,                                 \
    (first)->prev = (node), (first) = (node))
#define dll_push_front(first, last, node) \
    dll_push_front_np(first, last, node, next, prev)


// if refNode is NULL, insert at end of linked list
#define dll_insert_after_np(first, last, refNode, node, next, prev) \
    ((refNode) == NULL || (refNode == last)) ?                      \
    dll_push_back_np(first, last, node, next, prev) :               \
    ((node)->next = (refNode)->next, (node)->prev = (refNode),      \
    (refNode)->next->prev = (node), (refNode)->next = (node))
#define dll_insert_after(first, last, refNode, node) \
    dll_insert_after_np(first, last, refNode, node, next, prev)


// if refNode is NULL, insert at beginning of linked list
#define dll_insert_before_np(first, last, refNode, node, next, prev) \
    ((refNode) == NULL || (refNode == first)) ?                      \
    dll_push_front_np(first, last, node, next, prev) :               \
    ((node)->prev = (refNode)->prev, (node)->next = (refNode),       \
    (refNode)->prev->next = (node), (refNode)->prev = (node))
#define dll_insert_before(first, last, refNode, node) \
    dll_insert_before_np(first, last, refNode, node, next, prev)

#define dll_remove_np(first, last, node, next, prev)              \
    do {                                                          \
        if ((node) == (first)) (first) = (node)->next;            \
        if ((node) == (last))  (last) = (node)->prev;             \
        if ((node)->prev)      (node)->prev->next = (node)->next; \
        if ((node)->next)      (node)->next->prev = (node)->prev; \
        (node)->next = (node)->prev = NULL;                       \
    } while (0)
#define dll_remove(first, last, node) \
    dll_remove_np(first, last, node, next, prev)

// -- Array -------------------------------------------------------------------

#define _ArrayHeader_ struct { u64 count; u64 capacity; }
typedef struct ArrayHeader { u64 count; u64 capacity; } ArrayHeader;

#define array_header_cast(a) ((ArrayHeader *)&((a).count))
#define array_item_size(a)   (sizeof(*(a).v))

void *array_grow (Arena *arena, ArrayHeader *header, void *array, u64 item_size, u64 count, b32 clear_to_zero);
void array_shift_right(ArrayHeader *header, void *array, u64 item_size, u64 from_index, u64 n);
void array_shift_left(ArrayHeader *header, void *array, u64 item_size, u64 from_index, u64 n);

#define array_push(arena, a, value) (*((void **)&(a).v) =                       \
    array_grow((arena), array_header_cast(a), (a).v, array_item_size(a), 1, false), \
    (a).v[(a).count++] = (value))   

#define array_add(arena, a, n) (*((void **)&(a).v) =                              \
    array_grow((arena), array_header_cast(a), (a).v, array_item_size(a), (n), false), \
    (a).count += (n), &(a).v[(a).count - (n)])

#define array_add_clear(arena, a, n) (*((void **)&(a).v) =                        \
    array_grow((arena), array_header_cast(a), (a).v, array_item_size(a), (n), true), \
    (a).count += (n), &(a).v[(a).count - (n)])

#define array_reserve(arena, a, n) (((n) > (a).capacity) ? (*((void **)&(a).v) =                   \
    array_grow((arena), array_header_cast(a), (a).v, array_item_size(a), (n) - (a).capacity, false)) : \
    (a).v)

#define array_insert(arena, a, i, value) (*((void **)&(a).v) =                  \
    array_grow((arena), array_header_cast(a), (a).v, array_item_size(a), 1, false), \
    array_shift_right(array_header_cast(a), (a).v, array_item_size(a), (i), 1),              \
    (a).v[(i)] = (value), (a).count++)

#define array_remove(a, i, n) (array_shift_left(array_header_cast(a), (a).v, array_item_size(a), (i), (n)), \
    (a).count -= (n))

#define array_clear(a) a.count = 0

//=============================================================================
// STRING DECLARATIONS 
//=============================================================================

typedef struct String {
    char *value;
    u64 length;
} String;

#define STRING_START(string) (((string).length) ? ((string).value[(string).length]) : NULL)
#define STRING_END(string) ((string).value)
#define STRING_INVALID_INDEX ((u64) -1)

// -- string ------------------------------------

String string_slice (const String *s, u64 start, u64 end);

String *string_copy (const String *s);
String *string_copy_into (const String *string_to_copy, String *string_to_fill);

String *string_copy_slice (const String *s, u64 start, u64 end);
String *string_copy_slice_into (const String *string_to_copy, u64 start, u64 end, String *string_to_fill);

bool string_is_equal (const String *a, const String *b);	

u64 string_find_next_char (const String *s, const char c);
u64 string_find_last_char (const String *s, const char c);

u64 string_find_next_substring (const String *s, const String *substr);
u64 string_find_last_substring (const String *s, const String *substr);

// u64 string_find_next_regex (const String *s, const String *regex);
// u64 string_find_last_regex (const String *s, const String *regex);

// -- conversion --------------------------------

String string_from_cstr (const char *cstr, u64 cstr_size);
String *string_from_cstr_alloc (const char *cstr, u64 cstr_size);
void string_from_cstr_into (const char *cstr, u64 cstr_size, String *string);

char *string_to_cstr_alloc (const String *string);
void string_to_cstr_into (const String *string, char *cstr, u64 cstr_size);

String string_from_wstr (const wchar *wstr, u64 wstr_size);
String *string_from_wstr_alloc (const wchar *wstr, u64 wstr_size);
void string_from_wstr_into (const wchar *wstr, u64 wstr_size, String *string); 

wchar *string_to_wstr_alloc (const String *string);
void string_to_wstr_into (const String *string, wchar *wstr, u64 wstr_size);

String string_from_utf16 (const char16 *c16str, u64 c16str_size);
String *string_from_utf16_alloc (const char16 *c16str, u64 c16str_size);
void string_from_utf16_into (const char16 *c16str, u64 c16str_size, String *string);

char16 *string_to_utf16_alloc (const String *string);
void string_to_utf16_into (const String *string, char16 *c16str, u64 c16str_size);

String string_from_utf32 (const char32 *c32str, u64 c32str_length);
String *string_from_utf32_alloc (const char32 *c32str, u64 c32str_length);
void string_from_utf32_into (const char32 *c32str, u64 c32str_length, String *string);

char32 *string_to_utf32_alloc (const String *string);
void string_to_utf32_into (const String *string, char32 *c32str, u64 c32str_size);

String string_from_format (const char* fmt, ...);
String *string_from_format_alloc (const char *fmt, ...);
void string_from_format_into (String *string, const char *fmt, ...);

// -- encoding helpers --------------------------

static u64 utf8_encode_char(u32 codepoint, char *buffer) {
    if (codepoint <= 0x7F) {
        buffer[0] = (char)codepoint;
        return 1;
    } else if (codepoint <= 0x7FF) {
        buffer[0] = 0xC0 | (codepoint >> 6);
        buffer[1] = 0x80 | (codepoint & 0x3F);
        return 2;
    } else if (codepoint <= 0xFFFF) {
        buffer[0] = 0xE0 | (codepoint >> 12);
        buffer[1] = 0x80 | ((codepoint >> 6) & 0x3F);
        buffer[2] = 0x80 | (codepoint & 0x3F);
        return 3;
    } else {
        buffer[0] = 0xF0 | (codepoint >> 18);
        buffer[1] = 0x80 | ((codepoint >> 12) & 0x3F);
        buffer[2] = 0x80 | ((codepoint >> 6) & 0x3F);
        buffer[3] = 0x80 | (codepoint & 0x3F);
        return 4;
    }
}

static u64 utf16_encode_char(u32 codepoint, char16 *buffer) {
    if (codepoint <= 0xFFFF) {
        buffer[0] = (char16)codepoint;
        return 1;
    } else {
        // encode surrogate pair
        codepoint -= 0x10000;
        buffer[0] = 0xD800 | ((codepoint >> 10) & 0x3FF);
        buffer[1] = 0xDC00 | (codepoint & 0x3FF);
        return 2;
    }
}

typedef struct {
    u32 codepoint;   // Unicode scalar value
    u8  size;        // number of bytes in this codepoint
    u64 index;       // byte offset into the String
} UTF8Iterator;

// Decode next UTF-8 codepoint starting at s->value[*i].
// Advances *i by the codepoint length in bytes.
// Returns 0xFFFD (replacement char) on invalid UTF-8.
static inline UTF8Iterator utf8_next(const String *s, u64 *i) {
    UTF8Iterator it = { .codepoint = 0, .index = *i, .size = 0 };

    if (*i >= s->length) return it;  // end

    const byte *b = (const byte *)s->value + *i;
    byte b0 = b[0];

    if (b0 < 0x80) {
        // 1-byte ASCII
        it.codepoint = b0;
        it.size = 1;
    } else if ((b0 >> 5) == 0x6 && *i + 1 < s->length) {
        // 2-byte
        byte b1 = b[1];
        if ((b1 & 0xC0) == 0x80) {
            it.codepoint = ((b0 & 0x1F) << 6) | (b1 & 0x3F);
            it.size = 2;
        } else it.codepoint = 0xFFFD, it.size = 1;
    } else if ((b0 >> 4) == 0xE && *i + 2 < s->length) {
        // 3-byte
        byte b1 = b[1], b2 = b[2];
        if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80) {
            it.codepoint = ((b0 & 0x0F) << 12) | ((b1 & 0x3F) << 6) | (b2 & 0x3F);
            it.size = 3;
        } else it.codepoint = 0xFFFD, it.size = 1;
    } else if ((b0 >> 3) == 0x1E && *i + 3 < s->length) {
        // 4-byte
        byte b1 = b[1], b2 = b[2], b3 = b[3];
        if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80 && (b3 & 0xC0) == 0x80) {
            it.codepoint = ((b0 & 0x07) << 18) | ((b1 & 0x3F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F);
            it.size = 4;
        } else it.codepoint = 0xFFFD, it.size = 1;
    } else {
        it.codepoint = 0xFFFD;
        it.size = 1;
    }

    *i += it.size;
    return it;
}

// Decode previous UTF-8 codepoint starting at s->value[*i].
// Moves *i backwards to the start of the previous codepoint.
// Returns 0xFFFD on invalid UTF-8, 0 if at start.
static inline UTF8Iterator utf8_prev(const String *s, u64 *i) {
    UTF8Iterator it = { .codepoint = 0xFFFD, .index = *i, .size = 1 };

    if (*i == 0) {
        it.codepoint = 0;
        it.size = 0;
        return it;
    }

    u64 pos = *i - 1;
    const byte *bytes = (const byte *) s->value;

    // Move backwards until finding a non-continuation byte
    u8 continuation_bytes = 0;
    while (pos > 0 && (bytes[pos] & 0xC0) == 0x80) {
        pos--;
        continuation_bytes++;
        if (continuation_bytes > 3) {
			break; // invalid UTF-8 max 4 bytes
    	}
	}

    byte b0 = bytes[pos];

    // Determine length from leading byte
    if ((b0 & 0x80) == 0) {       // 1-byte ASCII
        it.size = 1;
        it.codepoint = b0;
    }
	// 2-byte sequence
	else if ((b0 & 0xE0) == 0xC0 && continuation_bytes == 1) {
        it.size = 2;
        if (pos + 1 < s->length) {
            byte b1 = bytes[pos + 1];
            if ((b1 & 0xC0) == 0x80)
                it.codepoint = ((b0 & 0x1F) << 6) | (b1 & 0x3F);
        }
    }
	// 3-byte sequence
	else if ((b0 & 0xF0) == 0xE0 && continuation_bytes == 2) {
        it.size = 3;
        if (pos + 2 < s->length) {
            byte b1 = bytes[pos + 1];
            byte b2 = bytes[pos + 2];
            if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80)
                it.codepoint = ((b0 & 0x0F) << 12) | ((b1 & 0x3F) << 6) | (b2 & 0x3F);
        }
    }
	// 4-byte sequence
	else if ((b0 & 0xF8) == 0xF0 && continuation_bytes == 3) {
        it.size = 4;
        if (pos + 3 < s->length) {
            byte b1 = bytes[pos + 1];
            byte b2 = bytes[pos + 2];
            byte b3 = bytes[pos + 3];
            if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80 && (b3 & 0xC0) == 0x80)
                it.codepoint = ((b0 & 0x07) << 18) | ((b1 & 0x3F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F);
        }
    }

    it.index = pos;
    *i = pos;
    return it;
}

typedef struct {
    u32 codepoint;
    u64 index; // element offset in the wchar array
} WCharIterator;

static inline WCharIterator wchar_next(const wchar_t *wstr, u64 length, u64 *i) {
    WCharIterator it = { .codepoint = 0xFFFD, .index = *i };
    if (*i >= length) {
        it.codepoint = 0; // end
        return it;
    }

    u32 cp = (u32)wstr[*i]; // read codepoint
#ifdef _WIN32
    // Handle surrogate pairs for UTF-16
    if (cp >= 0xD800 && cp <= 0xDBFF && *i + 1 < length) {
        u32 low = wstr[*i + 1];
        if (low >= 0xDC00 && low <= 0xDFFF) {
            cp = 0x10000 + (((cp - 0xD800) << 10) | (low - 0xDC00));
            it.index = *i;
            *i += 2;
            it.codepoint = cp;
            return it;
        }
    }
#endif
    it.codepoint = cp;
    it.index = *i;
    (*i)++;
    return it;
}

// Move backward
static inline WCharIterator wchar_prev(const wchar_t *wstr, u64 *i) {
    WCharIterator it = { .codepoint = 0xFFFD, .index = *i };
    if (*i == 0) {
        it.codepoint = 0; // start reached
        return it;
    }

    (*i)--; // move back one element
    u32 cp = (u32)wstr[*i];

#ifdef _WIN32
    // Check if this is a low surrogate (0xDC00–0xDFFF)
    if (cp >= 0xDC00 && cp <= 0xDFFF && *i > 0) {
        u32 high = wstr[*i - 1];
        if (high >= 0xD800 && high <= 0xDBFF) {
            cp = 0x10000 + (((high - 0xD800) << 10) | (cp - 0xDC00));
            it.index = *i - 1;
            *i -= 1;
            it.codepoint = cp;
            return it;
        }
    }
#endif

    it.codepoint = cp;
    it.index = *i;
    return it;
}

//=============================================================================
// MATH DECLARATIONS
//=============================================================================

#define xtd_max(value1, value2) (value1 >= value2) ? value1 : value2
#define xtd_min(value1, value2) (value1 <= value2) ? value1 : value2

#define xtd_is_between(value, lower, upper) (((lower) <= (value)) && (upper) >= (value))
#define xtd_is_within_margin(value, target, margin) (\
		(xtd_is_between((value), ((target) - (margin)), ((target) + (margin)))) && \
		(xtd_is_between((value), ((target) - (margin)), ((target) + (margin)))))

#define xtd_is_power_of_two(x) ((x != 0) && ((x & (x - 1)) == 0))

#define xtd_sq(v) ((v) * (v))
#define xtd_sqrt(v) (sqrt(v)) // TODO: remove math.h and make custom sqrt

#define xtd_round(v) // TODO
					 //
#define xtd_cos32(v) // TODO
#define xtd_cos64(v) // TODO
#define xtd_sin32(v) // TODO
#define xtd_sin64(v) // TODO

// -- Vec2 --------------------------------------------------------------------

#define Vec2(type) { \
    struct { type x;     type y;      }; \
    struct { type width; type height; }; \
    type elements[2];                    \
}
typedef union Vec2(u8)  Vec2u8;
typedef union Vec2(u16) Vec2u16;
typedef union Vec2(u32) Vec2u32;
typedef union Vec2(u64) Vec2u64;
typedef union Vec2(i8)  Vec2i8;
typedef union Vec2(i16) Vec2i16;
typedef union Vec2(i32) Vec2i32;
typedef union Vec2(i64) Vec2i64;
typedef union Vec2(f32) Vec2f32;
typedef union Vec2(f64) Vec2f64;

#define vec2_mul(a, b) { ((a).x * (b).x),  ((a).y * (b).y) }
#define vec2_cast_mul(type, a, b) { (type) ((a).x * (b).x),  (type) ((a).y * (b).y) }

#define vec2_add(a, b) { ((a).x + (b).x), ((a).y + (b).y) }
#define vec2_cast_add(type, a, b) { (type) ((a).x + (b).x), (type) ((a).y + (b).y) }

#define vec2_sub(a, b) { ((a).x - (b).x), ((a).y - (b).y) }
#define vec2_cast_sub(type, a, b) { (type) ((a).x - (b).x), (type) ((a).y - (b).y) }

#define vec2_min(a, b) { (xtd_min((a).x, (b).x)), (xtd_min((a).y, (b).y)) }
#define vec2_cast_min(type, a, b) { (type) (xtd_min((a).x, (b).x)), (type) (xtd_min((a).y, (b).y)) }

#define vec2_max(a, b) { (xtd_max((a).x, (b).x)), (xtd_max((a).y, (b).y)) }
#define vec2_cast_max(type, a, b) { (type) (xtd_max((a).x, (b).x)), (type) (xtd_max((a).y, (b).y))}

#define vec2_dot(a, b) (((a).x * (b).x) + ((a).y * (b).y))

#define vec2_len_sq(v) (xtd_sq((v).x) + xtd_sq((v).y))
#define vec2_len(v) (xtd_sqrt(vec2_len_sq(v)))

#define vec2_dist_sq(a, b) (xtd_sq((a).x - (b).x) + xtd_sq((a).y - (b).y))
#define vec2_dist(a, b) (xtd_sqrt(vec2_dist_sq((a), (b))))

// Vec2Norm
// Vec2Perp
// Vec2IsPerp
// Vec2IsEq
// Vec2Clamp01
// Vec2Lerp
// Vec2MoveTo

// -- Vec3 --------------------------------------------------------------------

#define Vec3(type) { \
    struct { type x;     type y;      type z;     }; \
    struct { type width; type height; type depth; }; \
    type elements[3];                                \
}
typedef union Vec3(u8)  Vec3u8;
typedef union Vec3(u16) Vec3u16;
typedef union Vec3(u32) Vec3u32;
typedef union Vec3(u64) Vec3u64;
typedef union Vec3(i8)  Vec3i8;
typedef union Vec3(i16) Vec3i16;
typedef union Vec3(i32) Vec3i32;
typedef union Vec3(i64) Vec3i64;
typedef union Vec3(f32) Vec3f32;
typedef union Vec3(f64) Vec3f64;

#define vec3_mul(a, b) { ((a).x * (b).x),  ((a).y * (b).y), ((a).z * (b).z) }
#define vec3_cast_mul(type, a, b) { (type) ((a).x * (b).x),  (type) ((a).y * (b).y), (type) ((a).z * (b).z) }

#define vec3_add(a, b) { ((a).x + (b).x), ((a).y + (b).y), ((a).z + (b).z) }
#define vec3_cast_add(type, a, b) { (type) ((a).x + (b).x), (type) ((a).y + (b).y), (type) ((a).z + (b).z) }

#define vec3_sub(a, b) { ((a).x - (b).x), ((a).y - (b).y), ((a).z - (b).z) }
#define vec3_cast_sub(type, a, b) { (type) ((a).x - (b).x), (type) ((a).y - (b).y), (type) ((a).z - (b).z) }

#define vec3_min(a, b) { (xtd_min((a).x, (b).x)), (xtd_min((a).y, (b).y)), (xtd_min((a).z, (b).z)) }
#define vec3_cast_min(type, a, b) { (type) (xtd_min((a).x, (b).x)), (type) (xtd_min((a).y, (b).y)), (type) (xtd_min((a).z, (b).z)) }

#define vec3_max(a, b) { (xtd_max((a).x, (b).x)), (xtd_max((a).y, (b).y)), (xtd_max((a).z, (b).z)) }
#define vec3_cast_max(type, a, b) { (type) (xtd_max((a).x, (b).x)), (type) (xtd_max((a).y, (b).y)), (type) (xtd_max((a).z, (b).z)) }

#define vec3_dot(a, b) (((a).x * (b).x) + ((a).y * (b).y) + ((a).z * (b).z))

#define vec3_len_sq(v) (xtd_sq((v).x) + xtd_sq((v).y) + xtd_sq((v).z))
#define vec3_len(v) (xtd_sqrt(vec3_len_sq(v)))

#define vec3_dist_sq(a, b) (xtd_sq((a).x - (b).x) + xtd_sq((a).y - (b).y) + xtd_sq((a).z - (b).z))
#define vec3_dist(a, b) (xtd_sqrt(vec3_dist_sq((a), (b))))

// Vec3Norm
// Vec3Perp
// Vec3IsPerp
// Vec3IsEq
// Vec3Clamp01
// Vec3Lerp
// Vec3MoveTo

// -- Matrix ------------------------------------------------------------------
// TODO

//=============================================================================
// BINARY DECLARATIONS
//=============================================================================

#define Bit(x) (1 << (x))
#define BBit(x) ((u64)1 << (x))
#define HasBit(n, pos) ((n) & (1 << (pos)))

#define FlagSet(n, f) ((n) |= (f))
#define FlagClear(n, f) ((n &= ~(f)))
#define FlagToggle(n, f) ((n) ^= (f))
#define FlagExists(n, f) (((n) & (f)) == (f))
#define FlagEquals(n, f) ((n) == (f))
#define FlagIntersects(n, f) (((n) & (f)) > 0)

//=============================================================================
// MULTITHREADING DECLARATIONS
//=============================================================================

// -- Atomics -----------------------------------------------------------------
#if defined(_WIN32) || defined(_WIN64)
    typedef LONG atomic_i32;
    typedef LONGLONG atomic_i64;

    #define atomic_barrier_full() MemoryBarrier()
    #define atomic_barrier_acquire() _ReadBarrier()
    #define atomic_barrier_release() _WriteBarrier()

    #define atomic_load_32(ptr) InterlockedCompareExchange((volatile atomic_i32 *)(ptr), 0, 0)
    #define atomic_store_32(ptr, val) InterlockedExchange((volatile atomic_i32 *)(ptr), (atomic_i32)(val))

    #define atomic_load_64(ptr) InterlockedCompareExchange64((volatile atomic_i64 *)(ptr), 0, 0)
    #define atomic_store_64(ptr, val) InterlockedExchange64((volatile atomic_i64 *)(ptr), (atomic_i64)(val))

    #define atomic_load_ptr(ptr) InterlockedCompareExchangePointer((void* volatile *)(ptr), NULL, NULL)
    #define atomic_store_ptr(ptr, val) InterlockedExchangePointer((void* volatile *)(ptr), (void *)(val))

    #define atomic_comp_ex_32(ptr, expected, desired) \
        InterlockedCompareExchange((volatile atomic_i32 *)(ptr), (atomic_i32)(desired), (atomic_i32)(expected))
    #define atomic_comp_ex_64(ptr, expected, desired) \
        InterlockedCompareExchange64((volatile atomic_i64 *)(ptr), (atomic_i64)(desired), (atomic_i64)(expected))
    #define atomic_comp_ex_ptr(ptr, expected, desired) \
        InterlockedCompareExchangePointer((void * volatile *)(ptr), (void *)(desired), (void *)(expected))

    #define atomic_ex_32(ptr, val) InterlockedExchange((volatile atomic_i32 *)(ptr), (atomic_i32)(val))
    #define atomic_ex_64(ptr, val) InterlockedExchange64((volatile atomic_i64 *)(ptr), (atomic_i64)(val))
    #define atomic_ex_ptr(ptr, val) InterlockedExchangePointer((void * volatile *)(ptr), (void *)(val))

    #define atomic_fetch_add_32(ptr, val) InterlockedExchangeAdd((volatile atomic_i32 *)(ptr), (atomic_i32)(val))
    #define atomic_fetch_add_64(ptr, val) InterlockedExchangeAdd64((volatile atomic_i64 *)(ptr), (atomic_i64)(val))

    #define atomic_fetch_sub_32(ptr, val) InterlockedExchangeAdd((volatile atomic_i32 *)(ptr), -(atomic_i32)(val))
    #define atomic_fetch_sub_64(ptr, val) InterlockedExchangeAdd64((volatile atomic_i64 *)(ptr), -(atomic_i64)(val))

    #define atomic_inc_32(ptr) InterlockedIncrement((volatile atomic_i32 *)(ptr))
    #define atomic_inc_64(ptr) InterlockedIncrement64((volatile atomic_i64 *)(ptr))

    #define atomic_dec_32(ptr) InterlockedDecrement((volatile atomic_i32 *)(ptr))
    #define atomic_dec_64(ptr) InterlockedDecrement64((volatile atomic_i64 *)(ptr))

    #define atomic_fetch_or_32(ptr, mask) InterlockedOr((volatile atomic_i32 *)(ptr), (atomic_i32)(mask))
    #define atomic_fetch_or_64(ptr, mask) InterlockedOr64((volatile atomic_i64 *)(ptr), (atomic_i64)(mask))

    #define atomic_fetch_and_32(ptr, mask) InterlockedAnd((volatile atomic_i32 *)(ptr), (atomic_i32)(mask))
    #define atomic_fetch_and_64(ptr, mask) InterlockedAnd64((volatile atomic_i64 *)(ptr), (atomic_i64)(mask))

    #define atomic_fetch_xor_32(ptr, mask) InterlockedXor((volatile atomic_i32 *)(ptr), (atomic_i32)(mask))
    #define atomic_fetch_xor_64(ptr, mask) InterlockedXor64((volatile atomic_i64 *)(ptr), (atomic_i64)(mask))

//#else TODO
    //#define AtomicBarrierFull() MemoryBarrier()
    //#define AtomicBarrierAcquire()
    //#define AtomicBarrierRelease()

    //#define AtomicLoad(type, ptr)
	//#define AtomicStore(type, ptr, val)
    //#define AtomicCompEx32(ptr, expected, desired)
    //#define AtomicCompEx64(ptr, expected, desired)
    //#define AtomicCompExPtr(ptr, expected, desired)
    //#define AtomicEx32(ptr, val)
    //#define AtomicEx64(ptr, val)
    //#define AtomicExPtr(ptr, val)

    //#define AtomicFetchAdd32(ptr, val)
    //#define AtomicFetchAdd64(ptr, val)
    //#define AtomicFetchSub32(ptr, val)

    //#define AtomicInc32(ptr, val)
    //#define AtomicInc64(ptr, val)

    //#define AtomicDec32(ptr, val)
    //#define AtomicDec64(ptr, val)

    //#define AtomicFetchOr32(ptr, mask)
    //#define AtomicFetchOr64(ptr, mask)

    //#define AtomicFetchAnd32(ptr, mask)
    //#define AtomicFetchAnd64(ptr, mask)

    //#define AtomicFetchXor32(ptr, mask)
    //#define AtomicFetchXor64(ptr, mask)
#endif

// -- Shared Queue ------------------------------------------------------------

typedef struct {
    u8 *buffer; 
    u64 capacity;
    volatile u64 head;
    u64 padding;
    volatile u64 tail;
} SharedQueue;

static inline bool SPSCQueueInit (SharedQueue *q, u64 capacity_bytes) {
    if (!xtd_is_power_of_two(capacity_bytes)) { 
		return false; 
	}

    q->buffer = (u8 *) malloc(capacity_bytes);
    if (!q->buffer) return false;
    
    q->capacity = capacity_bytes;
    q->head = 0;
    q->tail = 0;
    
    return true;
}

static inline void QueueDestroy (SharedQueue *q) {
    free(q->buffer);
    q->buffer = NULL;
    q->capacity = 0;
    q->head = q->tail = 0;
}

// Single Producer Single Consumer
bool SPSCQueuePush (SharedQueue *q, const void *data, u64 size);
bool SPSCQueuePop (SharedQueue *q, void *dest, u64 size);

// Multiple Producer Single Consumer (TODO)
bool MPSCQueuePush (SharedQueue *q, const void *data, u64 size);
bool MPSCQueuePop (SharedQueue *q, void *data, u64 size);

// Single Producer Multiple Consumer (TODO)
bool SPMCQueuePush (SharedQueue *q, const void *data, u64 size);
bool SPMCQueuePop (SharedQueue *q, void *data, u64 size);

// Multiple Producer Multiple Consumer (TODO)
bool MPMCQueuePush (SharedQueue *q, const void *data, u64 size);
bool MPMCQueuePop (SharedQueue *q, void *data, u64 size);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#endif // XTDLIB_H
#ifdef XTDLIB_IMPLEMENTATION
#undef XTDLIB_IMPLEMENTATION
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

//=============================================================================
// TESTING IMPLEMENTATION
//=============================================================================

char *test_result_to_string (TestResult result) { 
	char *result_string;
	switch (result) { 
		case TEST_RESULT_FAIL: 
			result_string = _strdup("TEST_FAILED");
		break;
		case TEST_RESULT_ERROR:
			result_string = _strdup("ERROR");
		break;
		case TEST_RESULT_INCORRECT_VALUE:
			result_string = _strdup("INCORRECT VALUE");
		break;
		case TEST_RESULT_INVALID_STATE:
			result_string = _strdup("INVALID STATE");
		break;
		case TEST_RESULT_OUT_OF_RANGE:
			result_string = _strdup("OUT OF RANGE");
		break;
		case TEST_RESULT_NULL_VALUE:
			result_string = _strdup("NULL VALUE");
		break;
		case TEST_RESULT_RESOURCE_ERROR:
			result_string = _strdup("RESOURCE ERROR");
		break;
		case TEST_NOT_IMPLEMENTED:
			result_string = _strdup("TEST NOT IMPLEMENTED");
		break;
		default:
			result_string = _strdup("UNKNOWN TEST RESULT");
		break;
	}
	return result_string;
}

void test_case_record_error (TestCase *test_case, TestResult error_type, const char *message, const char *details, ...) {
	test_case->num_errors += 1;
	u32 error_index = test_case->num_errors - 1;
	
	// format and save error message	
	char error_message_buffer[120];
	char *stringified_result = test_result_to_string(error_type);
	snprintf(error_message_buffer, 120, "%s - %s", test_result_to_string(error_type), message);
	free(stringified_result);

	test_case->errors = realloc(test_case->errors, sizeof(char *) * test_case->num_errors);
	test_case->errors[error_index] = _strdup(error_message_buffer);

	// format and save error details
	char error_details_buffer[120];
	va_list args;
	va_start(args, details);
	vsnprintf(error_details_buffer, sizeof(error_details_buffer), details, args);
	va_end(args);

	test_case->error_details = realloc(test_case->error_details, sizeof(char *) * test_case->num_errors);
	test_case->error_details[error_index] = _strdup(error_details_buffer);
	
	return;
}

void test_case_run_test (TestCase *test_case) {
	test_case->function();
	test_case->passed = (test_case->num_errors == 0);
	return;
}

void test_group_output_errors (TestCase **test_cases, u32 num_test_cases) {
	
    bool all_passed = true;
	for (u32 case_index = 0; case_index < num_test_cases; ++case_index) {
        if (test_cases[case_index]->passed == false) {
            all_passed = false;
            break;
        }
    }
    if (all_passed) {
        return;
    }

    printf("========================= ERRORS ==============================\n");
	for (u32 case_index = 0; case_index < num_test_cases; ++case_index) {
		TestCase *test_case = test_cases[case_index];	
		u32 num_errors = test_case->num_errors;
		for (u32 error_index = 0; error_index < num_errors; ++error_index) {
			printf("[%s] %s\n", test_case->name, test_case->errors[error_index]);
			if (XTD_TEST_LOGGING_VERBOSITY == TEST_LOGGING_VERBOSITY_HIGH) {
				printf("%s\n", test_case->error_details[error_index]);
			}
		}
	}
	return;
}

void test_group_run_tests (TestCase **test_cases, u32 num_test_cases) {

	// collect modules
	const char *modules[128];
	u32 num_modules = 0;
	
	const char *components[128];
	u32 num_components = 0;

	// gather modules and components
	for (u32 case_index = 0; case_index < num_test_cases; ++case_index) {
		
		const char *module = test_cases[case_index]->tags[0];
		const char *component = test_cases[case_index]->tags[1];

		// TODO replace with map
		// gather modules
		bool module_is_unique = true;
		for (u32 module_index = 0; module_index < num_modules; ++module_index) {
			if (strcmp(modules[module_index], module) == 0) {
				module_is_unique = false;
				break;
			}
		}
		if (module_is_unique) {
			modules[num_modules] = test_cases[case_index]->tags[0];
			num_modules++;
		}

		// gather components
		bool component_is_unique = true;
		for (u32 component_index = 0; component_index < num_components; ++component_index) {
			if (strcmp(components[component_index], component) == 0) {
				component_is_unique = false;
				break;
			}
		}
		if (component_is_unique) {
			components[num_components] = test_cases[case_index]->tags[1];
			num_components++;
		}
	}
	
	// run all tests
	for (u32 case_index = 0; case_index < num_test_cases; ++case_index) {
		test_case_run_test(test_cases[case_index]);
	}

	// medium / low: print pass/fail per module
	if (XTD_TEST_LOGGING_VERBOSITY <= TEST_LOGGING_VERBOSITY_MEDIUM) {
		for (u32 module_index = 0; module_index < num_modules; ++module_index) {
			printf("[%s]:\t\t", modules[module_index]);
			for (u32 case_index = 0; case_index < num_test_cases; ++case_index) {
				if (strcmp(test_cases[case_index]->tags[0], modules[module_index]) != 0)
					continue;	
				if (test_cases[case_index]->passed)
					printf(".");
				else
					printf("F");
			}
			printf("\n");
		}
	}	
	// high: print pass/fail per component per module
	else if (XTD_TEST_LOGGING_VERBOSITY == TEST_LOGGING_VERBOSITY_HIGH) {
		for (u32 module_index = 0; module_index < num_modules; ++module_index) {
			printf("[%s]:\n", modules[module_index]);
			for (u32 component_index = 0; component_index < num_components; ++component_index) {
				bool component_in_module = false;
				for (u32 case_index = 0; case_index < num_test_cases; ++case_index) {
					if (strcmp(test_cases[case_index]->tags[0], modules[module_index]) == 0 &&
						strcmp(test_cases[case_index]->tags[1], components[component_index]) == 0) {
						component_in_module = true;
					}
				}
				if (!component_in_module) {
					continue;
				}

				printf("  -- %s:\t", components[component_index]);
                if (strlen(components[component_index]) < 8) {
                	printf("\t");
                }

				for (u32 case_index = 0; case_index < num_test_cases; ++case_index) {
					if (strcmp(test_cases[case_index]->tags[0], modules[module_index]) != 0)
						continue;
					if (strcmp(test_cases[case_index]->tags[1], components[component_index]) != 0)
						continue;
					if (test_cases[case_index]->passed)
						printf(".");
					else
						printf("F");
				}
				printf("\n");
			}
		}
	}

	if (XTD_TEST_LOGGING_VERBOSITY >= TEST_LOGGING_VERBOSITY_MEDIUM) {
		test_group_output_errors(test_cases, num_test_cases);
	}

	return;
}

void xtd_run_all_tests (void) {
	test_group_run_tests(xtd_global_test_collector, xtd_global_test_collector_num_tests);
}

//=============================================================================
// MEMORY IMPLEMENTATION
//=============================================================================

// -- Arena -------------------------------------------------------------------

b32 _arena_init (Arena *arena) {
    
    xtd_assert_message(arena != NULL, "[_arena_init]: Cannot initialize NULL arena\n");
    if (!arena) return false;
    
	arena->base = memory_reserve(ARENA_DEFAULT_RESERVE_SIZE);
    
    xtd_assert_message(arena->base != NULL, "[_arena_init]: Arena base is NULL after reserving memory\n");
    if (!arena->base) return false;
    
    arena->reserved  = ARENA_DEFAULT_RESERVE_SIZE;
    arena->committed = 0;
    arena->used      = 0;
    return true;
}

b32 arena_commit_memory (Arena *arena, u64 size) {

    xtd_assert_message(arena != NULL, "[arena_commit_memory]: Arena is NULL\n");
    xtd_assert_message(arena->base != NULL, "[arena_commit_memory]: Arena base is NULL\n");
    if (!arena || !arena->base) return false;

    u64 commit_size = memory_align_forward(size, (u64) ARENA_COMMIT_CHUNK_SIZE);
    void *commit_ptr = (u8 *)arena->base + arena->committed;
    void *result = memory_commit_reserved(commit_ptr, commit_size);
    
    if (result)
    {
        arena->committed += commit_size;
        return true;
    }

    xtd_assert_message(false, "[arena_commit_memory]: Failed to commit memory for arena\n");
    return false;
}

void *_arena_push (Arena *arena, u64 size, u64 alignment, b32 clear_to_zero) {

    if (arena->base == NULL) {
        _arena_init(arena);
        xtd_assert_message(arena->base != NULL, "[_arena_push]: Arena base is NULL after initialization\n.");
    }

    u64 current_ptr = (u64) arena->base + arena->used;
    u64 aligned_ptr = memory_align_forward(current_ptr, alignment);
    u64 padding = aligned_ptr - current_ptr;
    u64 total_size = padding + size;

    if (arena->used + total_size > arena->committed) {
        b32 new_committed = arena_commit_memory(arena, total_size);
        if (!new_committed) {
            return NULL;
        }
    }

    void *result = (void *)aligned_ptr;
    if (clear_to_zero) {
        memory_zero(result, size);
    }

    arena->used = (aligned_ptr + size) - (u64)arena->base;

    return result;
}

//=============================================================================
// STORAGE IMPLEMENTATION
//=============================================================================

// -- Array -------------------------------------------------------------------

void *array_grow (Arena *arena, ArrayHeader *header, void *array, u64 item_size, u64 count, b32 clear_to_zero) {
    u64 prev_count = header->count;
    u64 prev_capacity = header->capacity;
    u64 target_count = prev_count + count;

    if (target_count <= prev_capacity) {
        return array;
    }

    u64 new_capacity = prev_capacity ? prev_capacity * 2 : 16;
    while (new_capacity < target_count) {
        new_capacity *= 2;
    }

    u64 total_old_bytes = prev_capacity * item_size;
    u64 total_new_bytes = new_capacity * item_size;

    void *new_array = _arena_push(arena, total_new_bytes, alignof(u8), false);
    if (!new_array) return NULL;

    if (array && prev_count > 0) {
        u8 *src = (u8 *)array;
        u8 *dst = (u8 *)new_array;
        for (u64 i = 0; i < total_old_bytes; i++) {
            dst[i] = src[i];
        }
    }

    if (clear_to_zero) {
        u64 new_part_offset = prev_capacity * item_size;
        ZeroMemory((u8 *)new_array + new_part_offset, total_new_bytes - new_part_offset);
    }

    header->capacity = new_capacity;
    return new_array;
}

void array_shift_right (ArrayHeader *header, void *array, u64 item_size, u64 from_index, u64 n) {
    u64 count = header->count;
    if (!array || from_index > count) return;  // > instead of >= so inserting at end works

    u8 *base = (u8 *)array;

    u64 items_to_move = count - from_index;
    if (items_to_move > 0) {
        memmove(base + (from_index + n) * item_size,
                base + from_index * item_size,
                items_to_move * item_size);
    }
}

void array_shift_left (ArrayHeader *header, void *array, u64 item_size, u64 from_index, u64 n) {
    u64 count = header->count;
    if (!array || from_index >= count) return;

    u8 *base = (u8 *)array;

    u64 items_to_move = count - (from_index + n);
    if (items_to_move > 0) {
        memmove(base + from_index * item_size,
                base + (from_index + n) * item_size,
                items_to_move * item_size);
    }
}

/*
// -- Map ---------------------------------------------------------------------

// -- USAGE ------------

//struct KVStrEmployee { String key; Employee *value; };
//struct MapStrEmployee { _MapHeader_; KVStrEmployee *v; };

//MapStrEmployee map = {0};

//Employee *bob = ...;
//MapsPut(arena, map, bob->name, bob);

//Employee *bob = MapsGet(map, Employee *, S("Bob"));

//MapDelete(scratch.arena, map, S("Bob"));
//----------------------



//-- USAGE ------------

//Index index = MapsIndex(map, S("Bob"));
//if (index.exists) {
//    i32 at = index.value;
//    Employee *bob = map.v[at].value;
//}

//for (i32 i = 0; i < map.count; ++i) {
//    Employee *empoyee = map.v[i].value;
//}

//ArenaRelease(&arena);
//----------------------


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

//=============================================================================
// STRING IMPLEMENTATION
//=============================================================================

String string_slice (const String *s, u64 start, u64 end) {
	start = xtd_min(s->length, start);
	end = xtd_min(end, s->length);
	end = xtd_max(end, start);
	return (String) {s->value + start, end - start};
}

String *string_copy (const String *s) {
    String *copied_string = memory_allocate(memory_size_of(String));
	if (!copied_string) return NULL;
	
	copied_string->length = s->length;
	copied_string->value = memory_allocate(memory_size_of(copied_string->length));	
    if (!copied_string->value) return NULL;

	memory_copy(copied_string->value, s->value, s->length);
	
	return copied_string;
}

String *string_copy_into (const String *string_to_copy, String *string_to_fill) {
	if (!string_to_copy || !string_to_copy->value) { return string_to_fill; }

	const u64 copy_length = xtd_min(string_to_copy->length, string_to_fill->length);
	memory_copy(string_to_fill->value, string_to_copy->value, copy_length);
	
	string_to_fill->length = copy_length;
	
	return string_to_fill;
}

String *string_copy_slice (const String *string_to_copy, u64 start, u64 end) {
	String slice_to_copy = string_slice(string_to_copy, start, end);
	return string_copy(&slice_to_copy);
}

String *string_copy_slice_into (const String *string_to_copy, u64 start, u64 end, String *string_to_fill) {
	String slice_to_copy = string_slice(string_to_copy, start, end);
	return string_copy_into(&slice_to_copy, string_to_fill);
}

bool string_is_equal (const String *a, const String *b) {	
    return (a->length == b->length) && (memory_compare(a->value, b->value, a->length) == 0);
}

u64 string_find_next_char (const String *s, char c) {
	for (u64 index = 0; index < s->length; ++index) {
		if (s->value[index] == c) {
			return index;
		}
	}
	return STRING_INVALID_INDEX;
}

u64 string_find_last_char (const String *s, char c) {
	for (u64 index = s->length; index-- > 0; ) {
		if (s->value[index] == c) {
			return index;
		}
	}
	return STRING_INVALID_INDEX;
}

u64 string_find_next_substring(const String *s, const String *substr) {
    if (substr->length == 0) { return 0; }
    if (s->length < substr->length) { return STRING_INVALID_INDEX; }

    for (u64 index = 0; index <= s->length - substr->length; ++index) {
        if (memory_compare(s->value + index, substr->value, substr->length) == 0) {
            return index; 
        }
    }

    return STRING_INVALID_INDEX;
}

u64 string_find_last_substring(const String *s, const String *substr) {
    if (substr->length == 0) { return s->length; }
    if (s->length < substr->length) { return STRING_INVALID_INDEX; }
 
    for (u64 index = s->length - substr->length + 1; index-- > 0; ) {
        if (memory_compare(s->value + index, substr->value, substr->length) == 0) {
            return index; 
        }
    }

    return STRING_INVALID_INDEX;
}

// -- conversion --------------------------------------------------------------

// -- cstr --------------------------------------

String string_from_cstr(const char *cstr, u64 cstr_size) {
    if (!cstr) {
		return (String) { NULL, 0 };
	}
    return (String) { (char *) cstr, cstr_size };
}

String *string_from_cstr_alloc(const char *cstr, u64 cstr_size) {
    if (!cstr) { return NULL; }

    String *string = memory_allocate(memory_size_of(String));
    if (!string) { return NULL; }

    string->value = memory_allocate(cstr_size);
    if (!string->value) {
        memory_free(string);
        return NULL;
    }

    memory_copy(string->value, (void *) cstr, cstr_size);
    string->length = cstr_size;
    return string;
}

void string_from_cstr_into(const char *cstr, u64 cstr_size, String *string) {
    if (!cstr || !string || !string->value) { return; }

    u64 copy_len = xtd_min(string->length, cstr_size);
    memory_copy(string->value, (void *) cstr, copy_len);
    string->length = copy_len;
}


char *string_to_cstr_alloc(const String *string) {
    if (!string) { return NULL; }

    char *cstr = memory_allocate(string->length + 1);
    if (!cstr) { return NULL; }

    memory_copy(cstr, string->value, string->length);
    cstr[string->length] = '\0';
    return cstr;
}

void string_to_cstr_into(const String *string, char *cstr, u64 cstr_size) {
    if (!string || !cstr || cstr_size == 0) { return; }

    u64 copy_len = xtd_min(string->length, cstr_size - 1);
    memory_copy(cstr, string->value, copy_len);
    cstr[copy_len] = '\0';
}

// -- wstr --------------------------------------

String string_from_wstr(const wchar *wstr, u64 wstr_size) {
    if (!wstr) {
		return (String) { NULL, 0 };
	}

    // Worst case: 4 bytes per codepoint
    u64 max_bytes = wstr_size * 4;
    char *buffer = (char *) memory_allocate(max_bytes);
    if (!buffer) {
		return (String) { NULL, 0 };
	}

    u64 out_len = 0;
    u64 i = 0;
    WCharIterator it;
    while ((it = wchar_next(wstr, wstr_size, &i)).codepoint != 0) {
        u32 cp = it.codepoint;

        if (cp <= 0x7F) {
            buffer[out_len++] = (char)cp;
        } else if (cp <= 0x7FF) {
            buffer[out_len++] = 0xC0 | ((cp >> 6) & 0x1F);
            buffer[out_len++] = 0x80 | (cp & 0x3F);
        } else if (cp <= 0xFFFF) {
            buffer[out_len++] = 0xE0 | ((cp >> 12) & 0x0F);
            buffer[out_len++] = 0x80 | ((cp >> 6) & 0x3F);
            buffer[out_len++] = 0x80 | (cp & 0x3F);
        } else {
            buffer[out_len++] = 0xF0 | ((cp >> 18) & 0x07);
            buffer[out_len++] = 0x80 | ((cp >> 12) & 0x3F);
            buffer[out_len++] = 0x80 | ((cp >> 6) & 0x3F);
            buffer[out_len++] = 0x80 | (cp & 0x3F);
        }
    }

    return (String) { buffer, out_len };
}

String *string_from_wstr_alloc(const wchar *wstr, u64 wstr_size) {
    if (!wstr) { return NULL; }

    // Allocate the String struct itself
    String *s = (String *) memory_allocate(sizeof(String));
    if (!s) { return NULL; }

    // Worst-case buffer size: 4 bytes per wchar
    u64 max_bytes = wstr_size * 4;
    s->value = (char *) memory_allocate(max_bytes);
    if (!s->value) {
        free(s);
        return NULL;
    }

    u64 out_len = 0;
    u64 i = 0;
    WCharIterator it;
    while ((it = wchar_next(wstr, wstr_size, &i)).codepoint != 0) {
        u32 cp = it.codepoint;

        if (cp <= 0x7F) {
            s->value[out_len++] = (char)cp;
        } else if (cp <= 0x7FF) {
            s->value[out_len++] = 0xC0 | ((cp >> 6) & 0x1F);
            s->value[out_len++] = 0x80 | (cp & 0x3F);
        } else if (cp <= 0xFFFF) {
            s->value[out_len++] = 0xE0 | ((cp >> 12) & 0x0F);
            s->value[out_len++] = 0x80 | ((cp >> 6) & 0x3F);
            s->value[out_len++] = 0x80 | (cp & 0x3F);
        } else {
            s->value[out_len++] = 0xF0 | ((cp >> 18) & 0x07);
            s->value[out_len++] = 0x80 | ((cp >> 12) & 0x3F);
            s->value[out_len++] = 0x80 | ((cp >> 6) & 0x3F);
            s->value[out_len++] = 0x80 | (cp & 0x3F);
        }
    }

    s->length = out_len;
    return s;
}

void string_from_wstr_into(const wchar *wstr, u64 wstr_size, String *string) {
    if (!string || !string->value || string->length == 0) return;
    if (!wstr) {
        string->length = 0;
        return;
    }

    u64 copy_len = (wstr_size < string->length) ? wstr_size : string->length;

    for (u64 i = 0; i < copy_len; ++i) {
        string->value[i] = (char)wstr[i];  // or handle UTF-8 encoding if needed
    }

    string->length = copy_len;  // update to actual number copied
}

wchar *string_to_wstr_alloc(const String *string) {
    if (!string || !string->value || string->length == 0) { return NULL; }

    // Worst case: 1 wchar per codepoint (or surrogate pair handled on Windows)
    u64 max_wchars = string->length;
    wchar *buffer = (wchar *) memory_allocate(max_wchars * memory_size_of(wchar));
    if (!buffer) { return NULL; }

    u64 i = 0;
    u64 out_index = 0;
    UTF8Iterator it;
    while ((it = utf8_next(string, &i)).codepoint != 0) {
        u32 cp = it.codepoint;

#ifdef _WIN32
        // UTF-16 surrogate pair handling
        if (cp > 0xFFFF) {
            cp -= 0x10000;
            buffer[out_index++] = 0xD800 | ((cp >> 10) & 0x3FF); // high surrogate
            buffer[out_index++] = 0xDC00 | (cp & 0x3FF);         // low surrogate
        } else {
            buffer[out_index++] = (wchar)cp;
        }
#else
        buffer[out_index++] = (wchar)cp; // Linux/macOS wchar_t is 32-bit
#endif
    }

    return buffer;
}

void string_to_wstr_into(const String *string, wchar *wstr, u64 wstr_size) {
    if (!string || !string->value || !wstr || wstr_size == 0) { return; }

    u64 i = 0;
    u64 out_index = 0;
    UTF8Iterator it;

    while ((it = utf8_next(string, &i)).codepoint != 0) {
        if (out_index >= wstr_size) break; // prevent buffer overflow

        u32 cp = it.codepoint;

#ifdef _WIN32
        // UTF-16 surrogate pair handling
        if (cp > 0xFFFF) {
            if (out_index + 1 >= wstr_size) break; // not enough space for surrogate pair
            cp -= 0x10000;
            wstr[out_index++] = 0xD800 | ((cp >> 10) & 0x3FF); // high surrogate
            wstr[out_index++] = 0xDC00 | (cp & 0x3FF);         // low surrogate
        } else {
            wstr[out_index++] = (wchar)cp;
        }
#else
        wstr[out_index++] = (wchar)cp; // Linux/macOS wchar_t is 32-bit
#endif
    }
}

// -- utf16 -------------------------------------

String string_from_utf16 (const char16 *c16str, u64 c16str_size) {
    String s = {0};
    if (!c16str || c16str_size == 0) { return s; }

    // worst-case UTF-8 size: 3 bytes per UTF-16 code unit, plus 1 for safety
    u64 max_bytes = c16str_size * 3;
    char *buffer = malloc(max_bytes);
    if (!buffer) return s;

    u64 pos = 0;
    for (u64 i = 0; i < c16str_size; ++i) {
        u32 codepoint;
        char16 cu = c16str[i];

        if (cu >= 0xD800 && cu <= 0xDBFF && i + 1 < c16str_size) {
            // high surrogate
            char16 cu2 = c16str[i+1];
            if (cu2 >= 0xDC00 && cu2 <= 0xDFFF) {
                codepoint = ((cu - 0xD800) << 10) + (cu2 - 0xDC00) + 0x10000;
                i++;
            } else {
                codepoint = 0xFFFD; // invalid surrogate pair
            }
        } else if (cu >= 0xDC00 && cu <= 0xDFFF) {
            codepoint = 0xFFFD; // unexpected low surrogate
        } else {
            codepoint = cu;
        }

        pos += utf8_encode_char(codepoint, buffer + pos);
    }

    s.value = buffer;
    s.length = pos;
    return s;
}

String *string_from_utf16_alloc(const char16 *c16str, u64 c16str_size) {
    if (!c16str) { return NULL; }

    String *s = malloc(sizeof(String));
    if (!s) { return NULL; }

    if (c16str_size == 0) {
        s->length = 0;
        s->value = NULL;
        return s;
    }

    // Convert UTF-16 into a newly allocated buffer directly
    u64 max_bytes = c16str_size * 3;
    s->value = malloc(max_bytes);
    if (!s->value) {
        free(s);
        return NULL;
    }

    u64 pos = 0;
    for (u64 i = 0; i < c16str_size; ++i) {
        u32 codepoint;
        char16 cu = c16str[i];

        if (cu >= 0xD800 && cu <= 0xDBFF && i + 1 < c16str_size) {
            char16 cu2 = c16str[i+1];
            if (cu2 >= 0xDC00 && cu2 <= 0xDFFF) {
                codepoint = ((cu - 0xD800) << 10) + (cu2 - 0xDC00) + 0x10000;
                i++;
            } else {
                codepoint = 0xFFFD;
            }
        } else if (cu >= 0xDC00 && cu <= 0xDFFF) {
            codepoint = 0xFFFD;
        } else {
            codepoint = cu;
        }

        pos += utf8_encode_char(codepoint, s->value + pos);
    }

    s->length = pos;
    return s;
}

void string_from_utf16_into(const char16 *c16str, u64 c16str_size, String *string) {
    if (!string) { return; }

    // Free existing buffer if present
    if (string->value) {
        free(string->value);
        string->value = NULL;
        string->length = 0;
    }

    if (!c16str || c16str_size == 0) {
        string->length = 0;
        string->value = NULL;
        return;
    }

    // Worst-case buffer: 3 bytes per UTF-16 code unit
    u64 max_bytes = c16str_size * 3;
    string->value = malloc(max_bytes);
    if (!string->value) {
        string->length = 0;
        return;
    }

    u64 pos = 0;
    for (u64 i = 0; i < c16str_size; ++i) {
        u32 codepoint;
        char16 cu = c16str[i];

        if (cu >= 0xD800 && cu <= 0xDBFF && i + 1 < c16str_size) {
            char16 cu2 = c16str[i + 1];
            if (cu2 >= 0xDC00 && cu2 <= 0xDFFF) {
                codepoint = ((cu - 0xD800) << 10) + (cu2 - 0xDC00) + 0x10000;
                i++;
            } else {
                codepoint = 0xFFFD;
            }
        } else if (cu >= 0xDC00 && cu <= 0xDFFF) {
            codepoint = 0xFFFD;
        } else {
            codepoint = cu;
        }

        pos += utf8_encode_char(codepoint, string->value + pos);
    }

    string->length = pos;
}

char16 *string_to_utf16_alloc(const String *string) {
    if (!string || string->length == 0) return NULL;

    // Allocate enough space for all codepoints (each may take 2 UTF-16 units)
    u64 max_units = string->length * 2;
    char16 *buffer = malloc(max_units * sizeof(char16));
    if (!buffer) return NULL;

    u64 out_pos = 0;
    u64 i = 0;
    while (i < string->length) {
        UTF8Iterator it = utf8_next(string, &i);

        if (it.codepoint == 0) break;      // end of string
        if (it.codepoint == 0xFFFD) {      // invalid UTF-8
            buffer[out_pos++] = 0xFFFD;
            continue;
        }

        out_pos += utf16_encode_char(it.codepoint, buffer + out_pos);
    }

    return buffer;
}

void string_to_utf16_into(const String *string, char16 *c16str, u64 c16str_size) {
    if (!string || !c16str || c16str_size == 0) return;

    u64 out_pos = 0;
    u64 i = 0;
    while (i < string->length && out_pos < c16str_size) {
        UTF8Iterator it = utf8_next(string, &i);
        if (it.codepoint <= 0xFFFF) {
            c16str[out_pos++] = (char16)it.codepoint;
        } else if (out_pos + 1 < c16str_size) {
            // surrogate pair
            u32 cp = it.codepoint - 0x10000;
            c16str[out_pos++] = 0xD800 | (cp >> 10);
            c16str[out_pos++] = 0xDC00 | (cp & 0x3FF);
        } else {
            break; // not enough space
        }
    }
}

// -- utf32 -------------------------------------

String string_from_utf32 (const char32 *c32str, u64 c32str_length);
String *string_from_utf32_alloc (const char32 *c32str, u64 c32str_length);
void string_from_utf32_into (const char32 *c32str, u64 c32str_length, String *string);

char32 *string_to_utf32_alloc (const String *string);
void string_to_utf32_into (const String *string, char32 *c32str, u64 c32str_size);

// -- format ------------------------------------

String string_from_format (const char* fmt, ...);
String *string_from_format_alloc (const char *fmt, ...);
void string_from_format_into (String *string, const char *fmt, ...);

//=============================================================================
// MULTITHREADING IMPLEMENTATION
//=============================================================================
/*
// -- Thread ------------------------------------------------------------------

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

// -- Mutex -------------------------------------------------------------------

typedef CRITICAL_SECTION Mutex;
// takes Mutex *
#define MutexInit(mutex) InitializeCriticalSection(mutex)
#define MutexDestroy(mutex) DeleteCriticalSection(mutex)
#define MutexLock(mutex) EnterCriticalSection(mutex)
#define MutexUnlock(mutex) LeaveCriticalSection(mutex)

*/

// -- Single Producer Single Consumer -----------------------------------------

bool SPSCQueuePush (SharedQueue *q, const void *data, u64 size) {
    u64 head = q->head;
    u64 tail = q->tail;
    u64 capacity = q->capacity;

    if (tail - head >= capacity) {
        return false;
    }

    u64 position = tail & (capacity - 1);
    u64 first_chunk = xtd_min(capacity - position, size);

    memcpy(q->buffer + position, data, first_chunk);
    memcpy(q->buffer, (u8 *)data + first_chunk, size - first_chunk);

    atomic_barrier_release();
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

    atomic_barrier_acquire();
    u64 position = head & (capacity - 1);
    u64 first_chunk = xtd_min(capacity - position, size);

    memcpy(dest, q->buffer + position, first_chunk);
    memcpy((u8 *) dest + first_chunk, q->buffer, size - first_chunk);

    q->head = head + size;
    return true;
}

// -- Multiple Producer Single Consumer (TODO) --------------------------------
bool MPSCQueuePush (SharedQueue *q, const void *data, u64 size) {
	xtd_ignore_unused(q);
	xtd_ignore_unused(data);
	xtd_ignore_unused(size);
    return false;
}

bool MPSCQueuePop (SharedQueue *q, void *data, u64 size) {
    xtd_ignore_unused(q);
	xtd_ignore_unused(data);
	xtd_ignore_unused(size);
	return false;
}

// -- Single Producer Multiple Consumer (TODO) --------------------------------
bool SPMCQueuePush (SharedQueue *q, const void *data, u64 size) {
    xtd_ignore_unused(q);
	xtd_ignore_unused(data);
	xtd_ignore_unused(size); 
	return false;
}

bool SPMCQueuePop (SharedQueue *q, void *data, u64 size) {
    xtd_ignore_unused(q);
	xtd_ignore_unused(data);
	xtd_ignore_unused(size);
	return false;
}

// -- Multiple Producer Multiple Consumer (TODO) ------------------------------
bool MPMCQueuePush (SharedQueue *q, const void *data, u64 size) {
	xtd_ignore_unused(q);
	xtd_ignore_unused(data);
	xtd_ignore_unused(size); 
	return false;
}

bool MPMCQueuePop (SharedQueue *q, void *data, u64 size) {
    xtd_ignore_unused(q);
	xtd_ignore_unused(data);
	xtd_ignore_unused(size); 
	return false;
}

#endif // XTDLIB_IMPLEMENTATION
///////////////////////////////////////////////////////////////////////////////
