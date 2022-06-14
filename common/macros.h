#ifndef COMMON_MACROS_H
#define COMMON_MACROS_H

// STD headers
#include <assert.h>
#include <limits>
#include <stdint.h>

/**
 * Macros.
 */
#define LIKELY(x)                   __builtin_expect(!!(x), 1)
#define UNLIKELY(x)                 __builtin_expect(!!(x), 0)
#define SUPPRESS_UNUSED_WARNING(a)  ((void) a)

// Assert macros (define DISABLE_ASSERT to disable assertion).
#ifdef DISABLE_ASSERT
#define SP_ASSERT(exp) do {} while (0)
#else
#define SP_ASSERT(exp) assert(exp)
#endif

// Macro to generate default methods
#define DEFAULT_CTOR_AND_DTOR(TypeName)     \
  TypeName() = default;                     \
  ~TypeName() = default

// Macro to disallow copy/assignment
#define DISALLOW_COPY_AND_ASSIGN(TypeName)  \
  TypeName(const TypeName&) = delete;       \
  void operator=(const TypeName&) = delete

/**
 * Constant parameters.
 */
constexpr uint8_t kBitsPerByte = 8;
constexpr double kInvalidJobSize = -1;
constexpr uint64_t kBitsPerGb = 1000000000;
constexpr uint64_t kMicrosecsPerSec = 1000000;
constexpr uint64_t kNanosecsPerSec = 1000000000;
constexpr double kDblPosInfty = std::numeric_limits<double>::infinity();
constexpr double kDblNegInfty = -std::numeric_limits<double>::infinity();

#endif
