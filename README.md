
# Xtdlib
A modified C11 standard library for cross-platform development with minimal dependencies on the standard C library.
## Design Philosophy

### Memory Arenas Over Malloc/Free
The traditional `malloc`/`free` dynamic memory allocation pattern suffers from several issues:
-  **Performance Overhead**: All allocations go through an OS allocator, which has bookkeeping, fragmentation handling, and locking.
-  **Complex Program Architecture**: Individual object lifetimes must be manually tracked and managed, which is error-prone and often unnecessarily complex.

Memory arenas solve these problems by:
-  **Bulk Allocation**: Reserve large memory blocks upfront, cheaply commit as needed
-  **Automatic Cleanup**: Release entire arenas at once, eliminating individual frees
-  **Cache Efficiency**: Linear allocation patterns improve memory locality

### Range-Based Strings and String Builders
Traditional null-terminated strings have fundamental limitations:
-  **Length Calculation**: `strlen()` requires O(n) traversal for every length query
-  **Buffer Overruns**: No inherent bounds checking leads to errors and security vulnerabilities

-  **O(1) Length Access**: Store length alongside the string data
-  **Bounds Safety**: All operations can validate against known string boundaries
-  **Efficient Manipulation**: String builders allow incremental construction without repeated copying
-  **Flexible Storage**: Strings can reference any memory region, not just null-terminated buffers
```c
// Traditional approach - error prone and inefficient
char  buffer[256];
strcpy(buffer, "Hello");
strcat(buffer, " ");
strcat(buffer, "World"); // Must scan entire string each time
// XTDLib approach - safe and efficient
StringBuilder builder = {0};
StrBuild(&arena, &builder, "Hello");
StrBuild(&arena, &builder, " ");
StrBuild(&arena, &builder, "World");
String result = builder.buffer; // O(1) access to length
```
## Core Features
### Memory Management
-  **Arena Allocators**: Pool-based memory allocation with automatic cleanup
-  **Cross-platform Memory Operations**: Unified API for memory reservation, commitment, and release
-  **Alignment Support**: Built-in memory alignment utilities
### Data Structures
- **Stack, Queue, Doubly Linked List**: Linked-list based containers optimized for arena usel
-  **Dynamic Arrays**: Dynamic array with exposed array header (capacity / used)
-  **Lock-free Structures**: Single/multiple producer-consumer queue implementations for lock-free thread safe data sharing.
- **Iterable Map**: Hash Map implementation with array-like iteration.
### Mathematics
-  **Utility Functions**: Min/max, power-of-two checks, distance calculations
-  **Linear Algebra**: Vector and Matrix objects and operations
- **Digital Signal Processing**: DSP building blocks
### Multithreading
-  **Unified Thread API**: Cross-platform thread api without pthreads
-  **Atomic Primitives**: Cross-platform atomic operations
-  **Lock-free Data Structures**: Wait-free single-producer/single-consumer queues
### Testing Framework
-  **Integrated Testing**: Built-in test registration and execution
-  **Hierarchical Organization**: Module and component-based test grouping
-  **Flexible Reporting**: Multiple verbosity levels for test output

## Platform Support
Currently supports:
- Windows (Win32/Win64)
- Partial POSIX support (under development)\
The library uses platform-specific implementations where necessary (memory management, atomics) while providing a unified interface.
## Usage
XTDLib is designed as a single-header library. Include the header and define the implementation:
```c
#define XTDLIB_IMPLEMENTATION
#include  "xtdlib.h"

int  main() {
	Arena arena = {0};
	// Use arena-based allocation
	int *numbers = arena_push_array(&arena, 10, int);
	// Clean up automatically
	arena_release(&arena);
	return  0;
}
```

## Testing
The library includes an integrated testing framework with automatic test discovery. Unit testing functions created with the TEST macro are automatically added to a global registry of unit tests, which can be run with `xtd_run_all_tests()`.
### Example Unit Test
```c
#define XTD_TEST_LOGGING_VERBOSITY TEST_LOGGING_VERBOSITY_HIGH

TEST(Example, "module", "component") {
	// Test implementation
	if (some_error_condition) {
		test_case_record_error(&ExampleTest, TEST_RESULT_FAIL, 
			"Basic Description", 
			"Details with optional printf-style formatting (version %d.%d.%d)", 
			1, 0, 0);
	}

	return 0;
}

int main (void) {
	xtd_run_all_tests(); // run example test, record pass/fail, print errors
}
```
### Included Xtdlib Unit Tests
Xtdlib includes `tests.c`, a file containing unit tests for all functionality of the library that can be used to verify installation and portability.
```bash
> ./xtdlib_tests
[Memory]:  
-- Arena:  ...........  
[Storage]:  
-- Stack:  .........  
-- Queue:  .........  
-- DLL:  .......  
-- Array:  ..........  
-- Map:  .  
[String]:  
-- String:  .  
-- Builder:  .  
[Math]:  
-- Utils:  ......  
-- Vec2:  ...............  
-- Vec3:  ...............  
-- Matrix:  .  
-- DSP:  .  
[Binary]:  
-- Bits:  .  
-- Flags:  .  
[Multithreading]:  
-- Atomics:  ...........................  
-- Mutex:  .  
-- Thread:  .  
-- SharedQueue:  .
```
