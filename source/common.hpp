#ifndef clox_common_h
#define clox_common_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// #define DEBUG_TRACE_EXECUTION
// #define DEBUG_PRINT_CODE

// #define DEBUG_STRESS_GC
// #define DEBUG_LOG_GC

// #define NAN_BOXING

// #define ENABLE_MP

#define ROUND_ROBIN
// #define GENERATIONAL_GC

// #define PARALLEL_MARKING
#define COMPACT_LISP_2

constexpr int UINT8_COUNT = (UINT8_MAX + 1);
constexpr int PARALLEL_COUNT = 8;

#endif
