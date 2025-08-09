#ifndef XSTRING_H
#define XSTRING_H

#include "xstyle.h"

typedef struct {
    char *value;
    u64 length;
} String;

typedef struct StringBuilder {
    String buffer;
    u64 capacity;
} StringBuilder;

#define StrBuild(arena, builder, x) _Generic((x), \
    WString:  StrBuildWStr,     \
    wchar:    StrBuildWChar,    \
    wchar *:  StrBuildCWStr,    \
    String:   StrBuildStr,      \
    char:     StrBuildChar,     \
    i32:      StrBuildI32,      \
    u32:      StrBuildU32,      \
    u64,      StrBuildU64,      \
    f32:      StrBuildF32       \
    default:  StrBuildCStr)(arena, builder, x)

#define StrBuildF(arena, builder, format, ...) StrBuildFormat(arena, builder, format, ## __VA_ARGS__)

#define S(string) { (string), strlen(string) };
#define StrFrom(string, start) // copy string excluding first (start) characters
#define StrFromTo(string, start, end) // copy string from (start) index to (end) index
#define StrCopy(arena, string); // copy string to arena
#define StrFormat(arena, string, ...) // sprintf

#define UTF8ToWChar(arena, string, builder)
#define WStrBuilderTerm(arena, builder)

#define StrIndexChar(path, char, flags)
// SSF_SearchBackwards
// SSF_IndexPlusOne
// SSF_CountOnFail

#endif // XSTRING_H