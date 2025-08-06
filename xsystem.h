#ifndef XSYSTEM_H
#define XSYSTEM_H

#include <stdio.h>
#include <windows.h>

#include "xstyle.h"

#define Assert(condition) \
    ((condition) ? (void) 0 : AssertFail(#condition, NULL, __FILE__, __LINE__))

#define AssertMessage(condition, message) \
    ((condition) ? (void) 0 : AssertFail(#condition, message, __FILE__, __LINE__))

static inline void AssertFail (const char *condition, const char *message, const char *file, u32 line) {
    fprintf(stderr, "Assertion failed: (%s), file %s, line %d\n", condition, file, line);
    if (message) { fprintf(stderr, "\t -- [Assert Info]: %s", message); }
    ExitProcess(1);
}

#endif // XSYSTEM_H