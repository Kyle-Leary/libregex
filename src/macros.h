#pragma once

#include "ansi.h"
#include <stdio.h>
#include <stdlib.h>

#define IS_INSIDE(x, min, max) ((x >= min) && (x < max))
#define CLAMP(x, min, max) ((x < min) ? min : (x >= max) ? max : x)
#define ABS(x) ((x < 0) ? -x : x)

#define MIN(a, b) ((a < b) ? a : b)
#define MAX(a, b) ((a > b) ? a : b)

#define TIMES(code, num_times)                                                 \
  {                                                                            \
    int i_times_block;                                                         \
    while (i_times_block < num_times) {                                        \
      {code} i_times_block++;                                                  \
    }                                                                          \
  }

#define INTERNAL_ERROR_MSG(msg, ...)                                           \
  {                                                                            \
    fprintf(stderr,                                                            \
            ANSI_RED "ERROR: " msg                                             \
                     "\n\t[at file: (%s, %d); in: %s]" ANSI_RESET "\n",        \
            ##__VA_ARGS__, __FILE__, __LINE__, __PRETTY_FUNCTION__);           \
  }

#define INTERNAL_ERROR_MSG_NO_ARGS(msg)                                        \
  {                                                                            \
    fprintf(stderr,                                                            \
            ANSI_RED "ERROR: " msg                                             \
                     "\n\t[at file: (%s, %d); in: %s]" ANSI_RESET "\n",        \
            __FILE__, __LINE__, __PRETTY_FUNCTION__);                          \
  }

#ifdef DEBUG
#define ERROR(msg, ...)                                                        \
  {                                                                            \
    INTERNAL_ERROR_MSG(msg, __VA_ARGS__);                                      \
    fprintf(stderr, "Crashing...\n");                                          \
    exit(1);                                                                   \
  }
#define ERROR_NO_ARGS(msg)                                                     \
  {                                                                            \
    INTERNAL_ERROR_MSG_NO_ARGS(msg);                                           \
    fprintf(stderr, "Crashing...\n");                                          \
    exit(1);                                                                   \
  }
#else
#define ERROR(msg, ...)                                                        \
  { INTERNAL_ERROR_MSG(msg, __VA_ARGS__); }
#define ERROR_NO_ARGS(msg)                                                     \
  { INTERNAL_ERROR_MSG_NO_ARGS(msg); }
#endif // DEBUG

#define ERROR_FROM_BUF(message_buffer)                                         \
  { ERROR("%s", message_buffer); }

#define NULL_CHECK(value)                                                      \
  {                                                                            \
    if (value == NULL) {                                                       \
      ERROR_NO_ARGS("NULL check of " #value " failed.");                       \
    }                                                                          \
  }

#define NULL_CHECK_MSG(value, msg)                                             \
  {                                                                            \
    if (value == NULL) {                                                       \
      ERROR_NO_ARGS("NULL check of " #value " failed: " msg ".");              \
    }                                                                          \
  }

#define NULL_CHECK_STR_MSG(value, msg)                                         \
  {                                                                            \
    if (value == NULL) {                                                       \
      ERROR("NULL check of " #value " failed: %s.", msg);                      \
    }                                                                          \
  }

#define RUNTIME_ASSERT(expr)                                                   \
  {                                                                            \
    if (!(expr)) {                                                             \
      ERROR_NO_ARGS("assertion of " #expr " failed.");                         \
    }                                                                          \
  }

#define RUNTIME_ASSERT_MSG(expr, msg)                                          \
  {                                                                            \
    if (!(expr)) {                                                             \
      ERROR_NO_ARGS("assertion of " #expr " failed: " msg ".");                \
    }                                                                          \
  }

#define RUNTIME_ASSERT_STR_MSG(expr, msg)                                      \
  {                                                                            \
    if (!(expr)) {                                                             \
      ERROR("assertion of " #expr " failed: %s.", msg);                        \
    }                                                                          \
  }

#define NOT_IMPLEMENTED                                                        \
  fprintf(stderr, "ERROR: %s not implemented.\n", __PRETTY_FUNCTION__)
