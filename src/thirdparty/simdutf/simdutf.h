/* auto-generated on 2026-09-21 15:19:28.994895. Do not edit! */
/* begin file include/simdutf.h */
#ifndef SIMDUTF_H
#define SIMDUTF_H
#include <cstring>

/* begin file include/simdutf/compiler_check.h */
#ifndef SIMDUTF_COMPILER_CHECK_H
#define SIMDUTF_COMPILER_CHECK_H

#ifndef __cplusplus
  #error simdutf requires a C++ compiler
#endif

#ifndef SIMDUTF_CPLUSPLUS
  #if defined(_MSVC_LANG) && !defined(__clang__)
    #define SIMDUTF_CPLUSPLUS (_MSC_VER == 1900 ? 201103L : _MSVC_LANG)
  #else
    #define SIMDUTF_CPLUSPLUS __cplusplus
  #endif
#endif

// C++ 26
#if !defined(SIMDUTF_CPLUSPLUS26) && (SIMDUTF_CPLUSPLUS >= 202602L)
  #define SIMDUTF_CPLUSPLUS26 1
#endif

// C++ 23
#if !defined(SIMDUTF_CPLUSPLUS23) && (SIMDUTF_CPLUSPLUS >= 202302L)
  #define SIMDUTF_CPLUSPLUS23 1
#endif

// C++ 20
#if !defined(SIMDUTF_CPLUSPLUS20) && (SIMDUTF_CPLUSPLUS >= 202002L)
  #define SIMDUTF_CPLUSPLUS20 1
#endif

// C++ 17
#if !defined(SIMDUTF_CPLUSPLUS17) && (SIMDUTF_CPLUSPLUS >= 201703L)
  #define SIMDUTF_CPLUSPLUS17 1
#endif

// C++ 14
#if !defined(SIMDUTF_CPLUSPLUS14) && (SIMDUTF_CPLUSPLUS >= 201402L)
  #define SIMDUTF_CPLUSPLUS14 1
#endif

// C++ 11
#if !defined(SIMDUTF_CPLUSPLUS11) && (SIMDUTF_CPLUSPLUS >= 201103L)
  #define SIMDUTF_CPLUSPLUS11 1
#endif

#ifndef SIMDUTF_CPLUSPLUS17
  #error simdutf requires a compiler compliant with the C++17 standard
#endif

#endif // SIMDUTF_COMPILER_CHECK_H
/* end file include/simdutf/compiler_check.h */
/* begin file include/simdutf/common_defs.h */
#ifndef SIMDUTF_COMMON_DEFS_H
#define SIMDUTF_COMMON_DEFS_H

/* begin file include/simdutf/portability.h */
#ifndef SIMDUTF_PORTABILITY_H
#define SIMDUTF_PORTABILITY_H


#include <cfloat>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#ifndef _WIN32
  // strcasecmp, strncasecmp
  #include <strings.h>
#endif

#if defined(__apple_build_version__)
  #if __apple_build_version__ < 14000000
    #define SIMDUTF_SPAN_DISABLED                                              \
      1 // apple-clang/13 doesn't support std::convertible_to
  #endif
#endif

#if SIMDUTF_CPLUSPLUS20
  #include <version>
  #if __cpp_concepts >= 201907L && __cpp_lib_span >= 202002L &&                \
      !defined(SIMDUTF_SPAN_DISABLED)
    #define SIMDUTF_SPAN 1
  #endif // __cpp_concepts >= 201907L && __cpp_lib_span >= 202002L
  #if __cpp_lib_atomic_ref >= 201806L
    #define SIMDUTF_ATOMIC_REF 1
  #endif // __cpp_lib_atomic_ref
  #if __has_cpp_attribute(maybe_unused) >= 201603L
    #define SIMDUTF_MAYBE_UNUSED_AVAILABLE 1
  #endif // __has_cpp_attribute(maybe_unused) >= 201603L
#endif

/**
 * We want to check that it is actually a little endian system at
 * compile-time.
 */

#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__)
  #define SIMDUTF_IS_BIG_ENDIAN (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#elif defined(_WIN32)
  #define SIMDUTF_IS_BIG_ENDIAN 0
#else
  #if defined(__APPLE__) ||                                                    \
      defined(__FreeBSD__) // defined __BYTE_ORDER__ && defined
                           // __ORDER_BIG_ENDIAN__
    #include <machine/endian.h>
  #elif defined(sun) ||                                                        \
      defined(__sun) // defined(__APPLE__) || defined(__FreeBSD__)
    #include <sys/byteorder.h>
  #else // defined(__APPLE__) || defined(__FreeBSD__)

    #ifdef __has_include
      #if __has_include(<endian.h>)
        #include <endian.h>
      #endif //__has_include(<endian.h>)
    #endif   //__has_include

  #endif // defined(__APPLE__) || defined(__FreeBSD__)

  #ifndef !defined(__BYTE_ORDER__) || !defined(__ORDER_LITTLE_ENDIAN__)
    #define SIMDUTF_IS_BIG_ENDIAN 0
  #endif

  #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    #define SIMDUTF_IS_BIG_ENDIAN 0
  #else // __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    #define SIMDUTF_IS_BIG_ENDIAN 1
  #endif // __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__

#endif // defined __BYTE_ORDER__ && defined __ORDER_BIG_ENDIAN__

/**
 * At this point in time, SIMDUTF_IS_BIG_ENDIAN is defined.
 */

#ifdef _MSC_VER
  #define SIMDUTF_VISUAL_STUDIO 1
  /**
   * We want to differentiate carefully between
   * clang under visual studio and regular visual
   * studio.
   *
   * Under clang for Windows, we enable:
   *  * target pragmas so that part and only part of the
   *     code gets compiled for advanced instructions.
   *
   */
  #ifdef __clang__
    // clang under visual studio
    #define SIMDUTF_CLANG_VISUAL_STUDIO 1
  #else
    // just regular visual studio (best guess)
    #define SIMDUTF_REGULAR_VISUAL_STUDIO 1
  #endif // __clang__
#endif   // _MSC_VER

#ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
  // https://en.wikipedia.org/wiki/C_alternative_tokens
  // This header should have no effect, except maybe
  // under Visual Studio.
  #include <iso646.h>
#endif

#if (defined(__x86_64__) || defined(_M_AMD64)) && !defined(_M_ARM64EC)
  #define SIMDUTF_IS_X86_64 1
#elif defined(__aarch64__) || defined(_M_ARM64) || defined(_M_ARM64EC)
  #define SIMDUTF_IS_ARM64 1
#elif defined(__PPC64__) || defined(_M_PPC64)
  #if defined(__VEC__) && defined(__ALTIVEC__) && defined(__POWER8_VECTOR__)
    #define SIMDUTF_IS_PPC64 1
  #endif
#elif defined(__s390__)
// s390 IBM system. Big endian.
#elif (defined(__riscv) || defined(__riscv__)) && __riscv_xlen == 64
  // RISC-V 64-bit
  #define SIMDUTF_IS_RISCV64 1

  // #if __riscv_v_intrinsic >= 1000000
  //   #define SIMDUTF_HAS_RVV_INTRINSICS 1
  //   #define SIMDUTF_HAS_RVV_TARGET_REGION 1
  // #elif ...
  //  Check for special compiler versions that implement pre v1.0 intrinsics
  #if __riscv_v_intrinsic >= 11000
    #define SIMDUTF_HAS_RVV_INTRINSICS 1
  #endif

  #define SIMDUTF_HAS_ZVBB_INTRINSICS                                          \
    0 // there is currently no way to detect this

  #if SIMDUTF_HAS_RVV_INTRINSICS && __riscv_vector &&                          \
      __riscv_v_min_vlen >= 128 && __riscv_v_elen >= 64
    // RISC-V V extension
    #define SIMDUTF_IS_RVV 1
    #if SIMDUTF_HAS_ZVBB_INTRINSICS && __riscv_zvbb >= 1000000
      // RISC-V Vector Basic Bit-manipulation
      #define SIMDUTF_IS_ZVBB 1
    #endif
  #endif

#elif defined(__loongarch_lp64)
  #if defined(__loongarch_sx) && defined(__loongarch_asx)
    #define SIMDUTF_IS_LSX 1
    #define SIMDUTF_IS_LASX 1 // We can always run both
  #elif defined(__loongarch_sx)
    #define SIMDUTF_IS_LSX 1
    // Adjust for runtime dispatching support.
    #if defined(__GNUC__) && !defined(__clang__) &&                            \
        !defined(__INTEL_COMPILER) && !defined(__NVCOMPILER)
      #if __GNUC__ > 15 || (__GNUC__ == 15 && __GNUC_MINOR__ >= 0)
        // We are ok, we will support runtime dispatch for LASX.
      #else
        // We disable runtime dispatch for LASX, which means that we will not be
        // able to use LASX even if it is supported by the hardware. Loongson
        // users should update to GCC 15 or better.
        #define SIMDUTF_IMPLEMENTATION_LASX 0
      #endif
    #else
      // We are not using GCC, so we assume that we can support runtime dispatch
      // for LASX. https://godbolt.org/z/jcMnrjYhs
      #define SIMDUTF_IMPLEMENTATION_LASX 0
    #endif
  #endif
#else
  // The simdutf library is designed
  // for 64-bit processors and it seems that you are not
  // compiling for a known 64-bit platform. Please
  // use a 64-bit target such as x64 or 64-bit ARM for best performance.
  #define SIMDUTF_IS_32BITS 1

  // We do not support 32-bit platforms, but it can be
  // handy to identify them.
  #if defined(_M_IX86) || defined(__i386__)
    #define SIMDUTF_IS_X86_32BITS 1
  #elif defined(__arm__) || defined(_M_ARM)
    #define SIMDUTF_IS_ARM_32BITS 1
  #elif defined(__PPC__) || defined(_M_PPC)
    #define SIMDUTF_IS_PPC_32BITS 1
  #endif

#endif // defined(__x86_64__) || defined(_M_AMD64)

#ifdef SIMDUTF_IS_32BITS
  #ifndef SIMDUTF_NO_PORTABILITY_WARNING
  // In the future, we may want to warn users of 32-bit systems that
  // the simdutf does not support accelerated kernels for such systems.
  #endif // SIMDUTF_NO_PORTABILITY_WARNING
#endif   // SIMDUTF_IS_32BITS

// this is almost standard?
#define SIMDUTF_STRINGIFY_IMPLEMENTATION_(a) #a
#define SIMDUTF_STRINGIFY(a) SIMDUTF_STRINGIFY_IMPLEMENTATION_(a)

// Our fast kernels require 64-bit systems.
//
// On 32-bit x86, we lack 64-bit popcnt, lzcnt, blsr instructions.
// Furthermore, the number of SIMD registers is reduced.
//
// On 32-bit ARM, we would have smaller registers.
//
// The simdutf users should still have the fallback kernel. It is
// slower, but it should run everywhere.

//
// Enable valid runtime implementations, and select
// SIMDUTF_BUILTIN_IMPLEMENTATION
//

// We are going to use runtime dispatch.
#if defined(SIMDUTF_IS_X86_64) || defined(SIMDUTF_IS_LSX)
  #ifdef __clang__
    // clang does not have GCC push pop
    // warning: clang attribute push can't be used within a namespace in clang
    // up til 8.0 so SIMDUTF_TARGET_REGION and SIMDUTF_UNTARGET_REGION must be
    // *outside* of a namespace.
    #define SIMDUTF_TARGET_REGION(T)                                           \
      _Pragma(SIMDUTF_STRINGIFY(clang attribute push(                          \
          __attribute__((target(T))), apply_to = function)))
    #define SIMDUTF_UNTARGET_REGION _Pragma("clang attribute pop")
  #elif defined(__GNUC__)
    // GCC is easier
    #define SIMDUTF_TARGET_REGION(T)                                           \
      _Pragma("GCC push_options") _Pragma(SIMDUTF_STRINGIFY(GCC target(T)))
    #define SIMDUTF_UNTARGET_REGION _Pragma("GCC pop_options")
  #endif // clang then gcc

#endif // defined(SIMDUTF_IS_X86_64) || defined(SIMDUTF_IS_LSX)

// Default target region macros don't do anything.
#ifndef SIMDUTF_TARGET_REGION
  #define SIMDUTF_TARGET_REGION(T)
  #define SIMDUTF_UNTARGET_REGION
#endif

// Is threading enabled?
#if defined(_REENTRANT) || defined(_MT)
  #ifndef SIMDUTF_THREADS_ENABLED
    #define SIMDUTF_THREADS_ENABLED
  #endif
#endif

// workaround for large stack sizes under -O0.
// https://github.com/simdutf/simdutf/issues/691
#ifdef __APPLE__
  #ifndef __OPTIMIZE__
    // Apple systems have small stack sizes in secondary threads.
    // Lack of compiler optimization may generate high stack usage.
    // Users may want to disable threads for safety, but only when
    // in debug mode which we detect by the fact that the __OPTIMIZE__
    // macro is not defined.
    #undef SIMDUTF_THREADS_ENABLED
  #endif
#endif

#ifdef SIMDUTF_VISUAL_STUDIO
  // This is one case where we do not distinguish between
  // regular visual studio and clang under visual studio.
  // clang under Windows has _stricmp (like visual studio) but not strcasecmp
  // (as clang normally has)
  #define simdutf_strcasecmp _stricmp
  #define simdutf_strncasecmp _strnicmp
#else
  // The strcasecmp, strncasecmp, and strcasestr functions do not work with
  // multibyte strings (e.g. UTF-8). So they are only useful for ASCII in our
  // context.
  // https://www.gnu.org/software/libunistring/manual/libunistring.html#char-_002a-strings
  #define simdutf_strcasecmp strcasecmp
  #define simdutf_strncasecmp strncasecmp
#endif

#if defined(__GNUC__) && !defined(__clang__)
  #if __GNUC__ >= 11
    #define SIMDUTF_GCC11ORMORE 1
  #endif //  __GNUC__ >= 11
  #if __GNUC__ == 10
    #define SIMDUTF_GCC10 1
  #endif //  __GNUC__ == 10
  #if __GNUC__ < 10
    #define SIMDUTF_GCC9OROLDER 1
  #endif //  __GNUC__ == 10
#endif   // defined(__GNUC__) && !defined(__clang__)

#endif // SIMDUTF_PORTABILITY_H
/* end file include/simdutf/portability.h */
/* begin file include/simdutf/avx512.h */
#ifndef SIMDUTF_AVX512_H_
#define SIMDUTF_AVX512_H_

/*
    It's possible to override AVX512 settings with cmake DCMAKE_CXX_FLAGS.

    All preprocessor directives has form `SIMDUTF_HAS_AVX512{feature}`,
    where a feature is a code name for extensions.

    Please see the listing below to find which are supported.
*/

#ifndef SIMDUTF_HAS_AVX512F
  #if defined(__AVX512F__) && __AVX512F__ == 1
    #define SIMDUTF_HAS_AVX512F 1
  #endif
#endif

#ifndef SIMDUTF_HAS_AVX512DQ
  #if defined(__AVX512DQ__) && __AVX512DQ__ == 1
    #define SIMDUTF_HAS_AVX512DQ 1
  #endif
#endif

#ifndef SIMDUTF_HAS_AVX512IFMA
  #if defined(__AVX512IFMA__) && __AVX512IFMA__ == 1
    #define SIMDUTF_HAS_AVX512IFMA 1
  #endif
#endif

#ifndef SIMDUTF_HAS_AVX512CD
  #if defined(__AVX512CD__) && __AVX512CD__ == 1
    #define SIMDUTF_HAS_AVX512CD 1
  #endif
#endif

#ifndef SIMDUTF_HAS_AVX512BW
  #if defined(__AVX512BW__) && __AVX512BW__ == 1
    #define SIMDUTF_HAS_AVX512BW 1
  #endif
#endif

#ifndef SIMDUTF_HAS_AVX512VL
  #if defined(__AVX512VL__) && __AVX512VL__ == 1
    #define SIMDUTF_HAS_AVX512VL 1
  #endif
#endif

#ifndef SIMDUTF_HAS_AVX512VBMI
  #if defined(__AVX512VBMI__) && __AVX512VBMI__ == 1
    #define SIMDUTF_HAS_AVX512VBMI 1
  #endif
#endif

#ifndef SIMDUTF_HAS_AVX512VBMI2
  #if defined(__AVX512VBMI2__) && __AVX512VBMI2__ == 1
    #define SIMDUTF_HAS_AVX512VBMI2 1
  #endif
#endif

#ifndef SIMDUTF_HAS_AVX512VNNI
  #if defined(__AVX512VNNI__) && __AVX512VNNI__ == 1
    #define SIMDUTF_HAS_AVX512VNNI 1
  #endif
#endif

#ifndef SIMDUTF_HAS_AVX512BITALG
  #if defined(__AVX512BITALG__) && __AVX512BITALG__ == 1
    #define SIMDUTF_HAS_AVX512BITALG 1
  #endif
#endif

#ifndef SIMDUTF_HAS_AVX512VPOPCNTDQ
  #if defined(__AVX512VPOPCNTDQ__) && __AVX512VPOPCNTDQ__ == 1
    #define SIMDUTF_HAS_AVX512VPOPCNTDQ 1
  #endif
#endif

#endif // SIMDUTF_AVX512_H_
/* end file include/simdutf/avx512.h */

// Sometimes logging is useful, but we want it disabled by default
// and free of any logging code in release builds.
#ifdef SIMDUTF_LOGGING
  #include <cstdlib>
  #include <iostream>
  #define simdutf_log(msg)                                                     \
    std::cout << "[" << __FUNCTION__ << "]: " << msg << std::endl              \
              << "\t" << __FILE__ << ":" << __LINE__ << std::endl;
  #define simdutf_log_assert(cond, msg)                                        \
    do {                                                                       \
      if (!(cond)) {                                                           \
        std::cerr << "[" << __FUNCTION__ << "]: " << msg << std::endl          \
                  << "\t" << __FILE__ << ":" << __LINE__ << std::endl;         \
        std::abort();                                                          \
      }                                                                        \
    } while (0)
#else
  #define simdutf_log(msg)
  #define simdutf_log_assert(cond, msg)
#endif

#if SIMDUTF_CPLUSPLUS17
  #define simdutf_unused [[maybe_unused]]
#endif // SIMDUTF_CPLUSPLUS17

#if defined(SIMDUTF_REGULAR_VISUAL_STUDIO)
  #define SIMDUTF_DEPRECATED __declspec(deprecated)

  #define simdutf_really_inline __forceinline // really inline in release mode
  #define simdutf_always_inline __forceinline // always inline, no matter what
  #define simdutf_never_inline __declspec(noinline)

  #ifndef simdutf_unused
    #define simdutf_unused
  #endif // simdutf_unused
  #define simdutf_warn_unused

  #ifndef simdutf_likely
    #define simdutf_likely(x) x
  #endif
  #ifndef simdutf_unlikely
    #define simdutf_unlikely(x) x
  #endif

  #define SIMDUTF_PUSH_DISABLE_WARNINGS __pragma(warning(push))
  #define SIMDUTF_PUSH_DISABLE_ALL_WARNINGS __pragma(warning(push, 0))
  #define SIMDUTF_DISABLE_VS_WARNING(WARNING_NUMBER)                           \
    __pragma(warning(disable : WARNING_NUMBER))
  // Get rid of Intellisense-only warnings (Code Analysis)
  // Though __has_include is C++17, it is supported in Visual Studio 2017 or
  // better (_MSC_VER>=1910).
  #ifdef __has_include
    #if __has_include(<CppCoreCheck\Warnings.h>)
      #include <CppCoreCheck\Warnings.h>
      #define SIMDUTF_DISABLE_UNDESIRED_WARNINGS                               \
        SIMDUTF_DISABLE_VS_WARNING(ALL_CPPCORECHECK_WARNINGS)
    #endif
  #endif

  #ifndef SIMDUTF_DISABLE_UNDESIRED_WARNINGS
    #define SIMDUTF_DISABLE_UNDESIRED_WARNINGS
  #endif

  #define SIMDUTF_DISABLE_DEPRECATED_WARNING SIMDUTF_DISABLE_VS_WARNING(4996)
  #define SIMDUTF_DISABLE_STRICT_OVERFLOW_WARNING
  #define SIMDUTF_POP_DISABLE_WARNINGS __pragma(warning(pop))
  #define SIMDUTF_DISABLE_UNUSED_WARNING
#else // SIMDUTF_REGULAR_VISUAL_STUDIO
  #if defined(__OPTIMIZE__) || defined(NDEBUG)
    #define simdutf_really_inline inline __attribute__((always_inline))
  #else
    #define simdutf_really_inline inline
  #endif
  #define simdutf_always_inline                                                \
    inline __attribute__((always_inline)) // always inline, no matter what
  #define SIMDUTF_DEPRECATED __attribute__((deprecated))
  #define simdutf_never_inline inline __attribute__((noinline))
  #ifndef simdutf_unused
    #define simdutf_unused __attribute__((unused))
  #endif // simdutf_unused
  #define simdutf_warn_unused __attribute__((warn_unused_result))

  #ifndef simdutf_likely
    #define simdutf_likely(x) __builtin_expect(!!(x), 1)
  #endif
  #ifndef simdutf_unlikely
    #define simdutf_unlikely(x) __builtin_expect(!!(x), 0)
  #endif
  // clang-format off
  #define SIMDUTF_PUSH_DISABLE_WARNINGS _Pragma("GCC diagnostic push")
  // gcc doesn't seem to disable all warnings with all and extra, add warnings
  // here as necessary
  #define SIMDUTF_PUSH_DISABLE_ALL_WARNINGS                                    \
    SIMDUTF_PUSH_DISABLE_WARNINGS                                              \
    SIMDUTF_DISABLE_GCC_WARNING(-Weffc++)                                      \
    SIMDUTF_DISABLE_GCC_WARNING(-Wall)                                         \
    SIMDUTF_DISABLE_GCC_WARNING(-Wconversion)                                  \
    SIMDUTF_DISABLE_GCC_WARNING(-Wextra)                                       \
    SIMDUTF_DISABLE_GCC_WARNING(-Wattributes)                                  \
    SIMDUTF_DISABLE_GCC_WARNING(-Wimplicit-fallthrough)                        \
    SIMDUTF_DISABLE_GCC_WARNING(-Wnon-virtual-dtor)                            \
    SIMDUTF_DISABLE_GCC_WARNING(-Wreturn-type)                                 \
    SIMDUTF_DISABLE_GCC_WARNING(-Wshadow)                                      \
    SIMDUTF_DISABLE_GCC_WARNING(-Wunused-parameter)                            \
    SIMDUTF_DISABLE_GCC_WARNING(-Wunused-variable)
  #define SIMDUTF_PRAGMA(P) _Pragma(#P)
  #define SIMDUTF_DISABLE_GCC_WARNING(WARNING)                                 \
    SIMDUTF_PRAGMA(GCC diagnostic ignored #WARNING)
  #if defined(SIMDUTF_CLANG_VISUAL_STUDIO)
    #define SIMDUTF_DISABLE_UNDESIRED_WARNINGS                                 \
      SIMDUTF_DISABLE_GCC_WARNING(-Wmicrosoft-include)
  #else
    #define SIMDUTF_DISABLE_UNDESIRED_WARNINGS
  #endif
  #define SIMDUTF_DISABLE_DEPRECATED_WARNING                                   \
    SIMDUTF_DISABLE_GCC_WARNING(-Wdeprecated-declarations)
  #define SIMDUTF_DISABLE_STRICT_OVERFLOW_WARNING                              \
    SIMDUTF_DISABLE_GCC_WARNING(-Wstrict-overflow)
  #define SIMDUTF_POP_DISABLE_WARNINGS _Pragma("GCC diagnostic pop")
  #define SIMDUTF_DISABLE_UNUSED_WARNING                                       \
    SIMDUTF_PUSH_DISABLE_WARNINGS                                              \
    SIMDUTF_DISABLE_GCC_WARNING(-Wunused-function)                             \
    SIMDUTF_DISABLE_GCC_WARNING(-Wunused-const-variable)
  // clang-format on

#endif // MSC_VER

// Will evaluate to constexpr in C++23 or later. This makes it possible to mark
// functions constexpr if the "if consteval" feature is available to use.
#if SIMDUTF_CPLUSPLUS23
  #define simdutf_constexpr23 constexpr
#else
  #define simdutf_constexpr23
#endif

#ifndef SIMDUTF_DLLIMPORTEXPORT
  #if defined(SIMDUTF_VISUAL_STUDIO) // Visual Studio
                                     /**
                                      * Windows users need to do some extra work when building
                                      * or using a dynamic library (DLL). When building, we need
                                      * to set SIMDUTF_DLLIMPORTEXPORT to __declspec(dllexport).
                                      * When *using* the DLL, the user needs to set
                                      * SIMDUTF_DLLIMPORTEXPORT __declspec(dllimport).
                                      *
                                      * Static libraries not need require such work.
                                      *
                                      * It does not matter here whether you are using
                                      * the regular visual studio or clang under visual
                                      * studio, you still need to handle these issues.
                                      *
                                      * Non-Windows systems do not have this complexity.
                                      */
    #if SIMDUTF_BUILDING_WINDOWS_DYNAMIC_LIBRARY

      // We set SIMDUTF_BUILDING_WINDOWS_DYNAMIC_LIBRARY when we build a DLL
      // under Windows. It should never happen that both
      // SIMDUTF_BUILDING_WINDOWS_DYNAMIC_LIBRARY and
      // SIMDUTF_USING_WINDOWS_DYNAMIC_LIBRARY are set.
      #define SIMDUTF_DLLIMPORTEXPORT __declspec(dllexport)
    #elif SIMDUTF_USING_WINDOWS_DYNAMIC_LIBRARY
      // Windows user who call a dynamic library should set
      // SIMDUTF_USING_WINDOWS_DYNAMIC_LIBRARY to 1.

      #define SIMDUTF_DLLIMPORTEXPORT __declspec(dllimport)
    #else
      // We assume by default static linkage
      #define SIMDUTF_DLLIMPORTEXPORT
    #endif
  #else // defined(SIMDUTF_VISUAL_STUDIO)
    // Non-Windows systems do not have this complexity.
    #define SIMDUTF_DLLIMPORTEXPORT
  #endif // defined(SIMDUTF_VISUAL_STUDIO)
#endif

#if SIMDUTF_MAYBE_UNUSED_AVAILABLE
  #define simdutf_maybe_unused [[maybe_unused]]
#else
  #define simdutf_maybe_unused
#endif

#endif // SIMDUTF_COMMON_DEFS_H
/* end file include/simdutf/common_defs.h */
/* begin file include/simdutf/encoding_types.h */
#ifndef SIMDUTF_ENCODING_TYPES_H
#define SIMDUTF_ENCODING_TYPES_H
#include <string_view>

#if !defined(SIMDUTF_NO_STD_TEXT_ENCODING) &&                                  \
    defined(__cpp_lib_text_encoding) && __cpp_lib_text_encoding >= 202306L
  #define SIMDUTF_HAS_STD_TEXT_ENCODING 1
  #include <text_encoding>
#endif

namespace simdutf {

enum encoding_type {
  UTF8 = 1,      // BOM 0xef 0xbb 0xbf
  UTF16_LE = 2,  // BOM 0xff 0xfe
  UTF16_BE = 4,  // BOM 0xfe 0xff
  UTF32_LE = 8,  // BOM 0xff 0xfe 0x00 0x00
  UTF32_BE = 16, // BOM 0x00 0x00 0xfe 0xff
  Latin1 = 32,

  unspecified = 0
};

#ifndef SIMDUTF_IS_BIG_ENDIAN
  #error "SIMDUTF_IS_BIG_ENDIAN needs to be defined."
#endif

enum endianness {
  LITTLE = 0,
  BIG = 1,
  NATIVE =
#if SIMDUTF_IS_BIG_ENDIAN
      BIG
#else
      LITTLE
#endif
};

simdutf_warn_unused simdutf_really_inline constexpr bool
match_system(endianness e) {
  return e == endianness::NATIVE;
}

simdutf_warn_unused std::string_view to_string(encoding_type bom);

// Note that BOM for UTF8 is discouraged.
namespace BOM {

/**
 * Checks for a BOM. If not, returns unspecified
 * @param byte          the string to process
 * @param length        the length of the string in code units
 * @return the corresponding encoding
 */

simdutf_warn_unused encoding_type check_bom(const uint8_t *byte, size_t length);
simdutf_warn_unused encoding_type check_bom(const char *byte, size_t length);
/**
 * Returns the size, in bytes, of the BOM for a given encoding type.
 * Note that UTF8 BOM are discouraged.
 * @param bom         the encoding type
 * @return the size in bytes of the corresponding BOM
 */
simdutf_warn_unused size_t bom_byte_size(encoding_type bom);

} // namespace BOM

#ifdef SIMDUTF_HAS_STD_TEXT_ENCODING
/**
 * Convert a simdutf encoding type to a std::text_encoding.
 *
 * @param enc  the simdutf encoding type
 * @return     the corresponding std::text_encoding, or
 *             std::text_encoding::id::unknown for unspecified/unsupported
 */
simdutf_warn_unused constexpr std::text_encoding
to_std_encoding(encoding_type enc) noexcept {
  switch (enc) {
  case UTF8:
    return std::text_encoding(std::text_encoding::id::UTF8);
  case UTF16_LE:
    return std::text_encoding(std::text_encoding::id::UTF16LE);
  case UTF16_BE:
    return std::text_encoding(std::text_encoding::id::UTF16BE);
  case UTF32_LE:
    return std::text_encoding(std::text_encoding::id::UTF32LE);
  case UTF32_BE:
    return std::text_encoding(std::text_encoding::id::UTF32BE);
  case Latin1:
    return std::text_encoding(std::text_encoding::id::ISOLatin1);
  case unspecified:
  default:
    return std::text_encoding(std::text_encoding::id::unknown);
  }
}

/**
 * Convert a std::text_encoding to a simdutf encoding type.
 *
 * @param enc  the std::text_encoding
 * @return     the corresponding simdutf encoding type, or
 *             encoding_type::unspecified if the encoding is not supported
 */
simdutf_warn_unused constexpr encoding_type
from_std_encoding(const std::text_encoding &enc) noexcept {
  switch (enc.mib()) {
  case std::text_encoding::id::UTF8:
    return UTF8;
  case std::text_encoding::id::UTF16LE:
    return UTF16_LE;
  case std::text_encoding::id::UTF16BE:
    return UTF16_BE;
  case std::text_encoding::id::UTF32LE:
    return UTF32_LE;
  case std::text_encoding::id::UTF32BE:
    return UTF32_BE;
  case std::text_encoding::id::ISOLatin1:
    return Latin1;
  default:
    return unspecified;
  }
}

/**
 * Get the native-endian UTF-16 encoding type for this system.
 *
 * @return UTF16_LE on little-endian systems, UTF16_BE on big-endian systems
 */
simdutf_warn_unused constexpr encoding_type native_utf16_encoding() noexcept {
  #if SIMDUTF_IS_BIG_ENDIAN
  return UTF16_BE;
  #else
  return UTF16_LE;
  #endif
}

/**
 * Get the native-endian UTF-32 encoding type for this system.
 *
 * @return UTF32_LE on little-endian systems, UTF32_BE on big-endian systems
 */
simdutf_warn_unused constexpr encoding_type native_utf32_encoding() noexcept {
  #if SIMDUTF_IS_BIG_ENDIAN
  return UTF32_BE;
  #else
  return UTF32_LE;
  #endif
}

/**
 * Convert a std::text_encoding to a simdutf encoding type,
 * using native endianness for UTF-16/UTF-32 without explicit endianness.
 *
 * When the input is std::text_encoding::id::UTF16 or UTF32 (without LE/BE
 * suffix), this returns the native-endian simdutf variant.
 *
 * @param enc  the std::text_encoding
 * @return     the corresponding simdutf encoding type, or
 *             encoding_type::unspecified if the encoding is not supported
 */
simdutf_warn_unused constexpr encoding_type
from_std_encoding_native(const std::text_encoding &enc) noexcept {
  switch (enc.mib()) {
  case std::text_encoding::id::UTF8:
    return UTF8;
  case std::text_encoding::id::UTF16:
    return native_utf16_encoding();
  case std::text_encoding::id::UTF16LE:
    return UTF16_LE;
  case std::text_encoding::id::UTF16BE:
    return UTF16_BE;
  case std::text_encoding::id::UTF32:
    return native_utf32_encoding();
  case std::text_encoding::id::UTF32LE:
    return UTF32_LE;
  case std::text_encoding::id::UTF32BE:
    return UTF32_BE;
  case std::text_encoding::id::ISOLatin1:
    return Latin1;
  default:
    return unspecified;
  }
}
#endif // SIMDUTF_HAS_STD_TEXT_ENCODING

} // namespace simdutf
#endif
/* end file include/simdutf/encoding_types.h */
/* begin file include/simdutf/error.h */
#ifndef SIMDUTF_ERROR_H
#define SIMDUTF_ERROR_H
#include <string_view>

namespace simdutf {

enum error_code {
  SUCCESS = 0,
  HEADER_BITS, // Any byte must have fewer than 5 header bits.
  TOO_SHORT,   // The leading byte must be followed by N-1 continuation bytes,
               // where N is the UTF-8 character length This is also the error
               // when the input is truncated.
  TOO_LONG,    // We either have too many consecutive continuation bytes or the
               // string starts with a continuation byte.
  OVERLONG, // The decoded character must be above U+7F for two-byte characters,
            // U+7FF for three-byte characters, and U+FFFF for four-byte
            // characters.
  TOO_LARGE, // The decoded character must be less than or equal to
             // U+10FFFF,less than or equal than U+7F for ASCII OR less than
             // equal than U+FF for Latin1
  SURROGATE, // The decoded character must be not be in U+D800...DFFF (UTF-8 or
             // UTF-32)
             // OR
             // a high surrogate must be followed by a low surrogate
             // and a low surrogate must be preceded by a high surrogate
             // (UTF-16)
             // OR
             // there must be no surrogate at all and one is
             // found (Latin1 functions)
             // OR
             // *specifically* for the function
             // utf8_length_from_utf16_with_replacement, a surrogate (whether
             // in error or not) has been found (I.e., whether we are in the
             // Basic Multilingual Plane or not).
  INVALID_BASE64_CHARACTER, // Found a character that cannot be part of a valid
                            // base64 string. This may include a misplaced
                            // padding character ('=').
  BASE64_INPUT_REMAINDER,   // The base64 input terminates with a single
                            // character, excluding padding (=). It is also used
                            // in strict mode when padding is not adequate.
  BASE64_EXTRA_BITS,        // The base64 input terminates with non-zero
                            // padding bits.
  OUTPUT_BUFFER_TOO_SMALL,  // The provided buffer is too small.
  OTHER                     // Not related to validation/transcoding.
};

inline std::string_view error_to_string(error_code code) noexcept {
  switch (code) {
  case SUCCESS:
    return "SUCCESS";
  case HEADER_BITS:
    return "HEADER_BITS";
  case TOO_SHORT:
    return "TOO_SHORT";
  case TOO_LONG:
    return "TOO_LONG";
  case OVERLONG:
    return "OVERLONG";
  case TOO_LARGE:
    return "TOO_LARGE";
  case SURROGATE:
    return "SURROGATE";
  case INVALID_BASE64_CHARACTER:
    return "INVALID_BASE64_CHARACTER";
  case BASE64_INPUT_REMAINDER:
    return "BASE64_INPUT_REMAINDER";
  case BASE64_EXTRA_BITS:
    return "BASE64_EXTRA_BITS";
  case OUTPUT_BUFFER_TOO_SMALL:
    return "OUTPUT_BUFFER_TOO_SMALL";
  default:
    return "OTHER";
  }
}

struct result {
  error_code error;
  size_t count; // In case of error, indicates the position of the error. In
                // case of success, indicates the number of code units
                // validated/written.

  simdutf_really_inline simdutf_constexpr23 result() noexcept
      : error{error_code::SUCCESS}, count{0} {}

  simdutf_really_inline simdutf_constexpr23 result(error_code err,
                                                   size_t pos) noexcept
      : error{err}, count{pos} {}

  simdutf_really_inline simdutf_constexpr23 bool is_ok() const noexcept {
    return error == error_code::SUCCESS;
  }

  simdutf_really_inline simdutf_constexpr23 bool is_err() const noexcept {
    return error != error_code::SUCCESS;
  }
};

struct full_result {
  error_code error;
  size_t input_count;
  size_t output_count;
  bool padding_error = false; // true if the error is due to padding, only
                              // meaningful when error is not SUCCESS

  simdutf_really_inline simdutf_constexpr23 full_result() noexcept
      : error{error_code::SUCCESS}, input_count{0}, output_count{0} {}

  simdutf_really_inline simdutf_constexpr23 full_result(error_code err,
                                                        size_t pos_in,
                                                        size_t pos_out) noexcept
      : error{err}, input_count{pos_in}, output_count{pos_out} {}
  simdutf_really_inline simdutf_constexpr23 full_result(
      error_code err, size_t pos_in, size_t pos_out, bool padding_err) noexcept
      : error{err}, input_count{pos_in}, output_count{pos_out},
        padding_error{padding_err} {}

  simdutf_really_inline simdutf_constexpr23 operator result() const noexcept {
    if (error == error_code::SUCCESS) {
      return result{error, output_count};
    } else {
      return result{error, input_count};
    }
  }
};

} // namespace simdutf
#endif
/* end file include/simdutf/error.h */

SIMDUTF_PUSH_DISABLE_WARNINGS
SIMDUTF_DISABLE_UNDESIRED_WARNINGS

// Public API
/* begin file include/simdutf/simdutf_version.h */
// /include/simdutf/simdutf_version.h automatically generated by release.py,
// do not change by hand
#ifndef SIMDUTF_SIMDUTF_VERSION_H
#define SIMDUTF_SIMDUTF_VERSION_H

/** The version of simdutf being used (major.minor.revision) */
#define SIMDUTF_VERSION "9.2.0"

namespace simdutf {
enum {
  /**
   * The major version (MAJOR.minor.revision) of simdutf being used.
   */
  SIMDUTF_VERSION_MAJOR = 9,
  /**
   * The minor version (major.MINOR.revision) of simdutf being used.
   */
  SIMDUTF_VERSION_MINOR = 2,
  /**
   * The revision (major.minor.REVISION) of simdutf being used.
   */
  SIMDUTF_VERSION_REVISION = 0
};
} // namespace simdutf

#endif // SIMDUTF_SIMDUTF_VERSION_H
/* end file include/simdutf/simdutf_version.h */
/* begin file include/simdutf/implementation.h */
#ifndef SIMDUTF_IMPLEMENTATION_H
#define SIMDUTF_IMPLEMENTATION_H
#if !defined(SIMDUTF_NO_THREADS)
  #include <atomic>
#endif
#ifdef SIMDUTF_INTERNAL_TESTS
  #include <vector>
#endif
/* begin file include/simdutf/internal/isadetection.h */
/* From
https://github.com/endorno/pytorch/blob/master/torch/lib/TH/generic/simd/simd.h
Highly modified.

Copyright (c) 2016-     Facebook, Inc            (Adam Paszke)
Copyright (c) 2014-     Facebook, Inc            (Soumith Chintala)
Copyright (c) 2011-2014 Idiap Research Institute (Ronan Collobert)
Copyright (c) 2012-2014 Deepmind Technologies    (Koray Kavukcuoglu)
Copyright (c) 2011-2012 NEC Laboratories America (Koray Kavukcuoglu)
Copyright (c) 2011-2013 NYU                      (Clement Farabet)
Copyright (c) 2006-2010 NEC Laboratories America (Ronan Collobert, Leon Bottou,
Iain Melvin, Jason Weston) Copyright (c) 2006      Idiap Research Institute
(Samy Bengio) Copyright (c) 2001-2004 Idiap Research Institute (Ronan Collobert,
Samy Bengio, Johnny Mariethoz)

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

3. Neither the names of Facebook, Deepmind Technologies, NYU, NEC Laboratories
America and IDIAP Research Institute nor the names of its contributors may be
   used to endorse or promote products derived from this software without
   specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef SIMDutf_INTERNAL_ISADETECTION_H
#define SIMDutf_INTERNAL_ISADETECTION_H

#include <cstdint>
#include <cstdlib>
#if defined(_MSC_VER)
  #include <intrin.h>
#elif (defined(HAVE_GCC_GET_CPUID) && defined(USE_GCC_GET_CPUID)) ||           \
    defined(__FILC__)
  #include <cpuid.h>
#endif

#ifdef __FILC__
  #include <stdfil.h>
#endif


// RISC-V ISA detection utilities
#if SIMDUTF_IS_RISCV64 && defined(__linux__)
  #include <unistd.h> // for syscall
// We define these ourselves, for backwards compatibility
struct simdutf_riscv_hwprobe {
  int64_t key;
  uint64_t value;
};
  #define simdutf_riscv_hwprobe(...) syscall(258, __VA_ARGS__)
  #define SIMDUTF_RISCV_HWPROBE_KEY_IMA_EXT_0 4
  #define SIMDUTF_RISCV_HWPROBE_IMA_V (1 << 2)
  #define SIMDUTF_RISCV_HWPROBE_EXT_ZVBB (1 << 17)
#endif // SIMDUTF_IS_RISCV64 && defined(__linux__)

#if defined(__loongarch__) && defined(__linux__)
  #include <sys/auxv.h>
// bits/hwcap.h
// #define HWCAP_LOONGARCH_LSX             (1 << 4)
// #define HWCAP_LOONGARCH_LASX            (1 << 5)
#endif

namespace simdutf {
namespace internal {

enum instruction_set {
  DEFAULT = 0x0,
  NEON = 0x1,
  AVX2 = 0x4,
  SSE42 = 0x8,
  PCLMULQDQ = 0x10,
  BMI1 = 0x20,
  BMI2 = 0x40,
  ALTIVEC = 0x80,
  AVX512F = 0x100,
  AVX512DQ = 0x200,
  AVX512IFMA = 0x400,
  AVX512PF = 0x800,
  AVX512ER = 0x1000,
  AVX512CD = 0x2000,
  AVX512BW = 0x4000,
  AVX512VL = 0x8000,
  AVX512VBMI2 = 0x10000,
  AVX512VPOPCNTDQ = 0x2000,
  RVV = 0x4000,
  ZVBB = 0x8000,
  LSX = 0x40000,
  LASX = 0x80000,
};

#if defined(__PPC64__)

static inline uint32_t detect_supported_architectures() {
  return instruction_set::ALTIVEC;
}

#elif SIMDUTF_IS_RISCV64

static inline uint32_t detect_supported_architectures() {
  uint32_t host_isa = instruction_set::DEFAULT;
  #if SIMDUTF_IS_RVV
  host_isa |= instruction_set::RVV;
  #endif
  #if SIMDUTF_IS_ZVBB
  host_isa |= instruction_set::ZVBB;
  #endif
  #if defined(__linux__)
  simdutf_riscv_hwprobe probes[] = {{SIMDUTF_RISCV_HWPROBE_KEY_IMA_EXT_0, 0}};
  long ret = simdutf_riscv_hwprobe(&probes, sizeof probes / sizeof *probes, 0,
                                   nullptr, 0);
  if (ret == 0) {
    uint64_t extensions = probes[0].value;
    if (extensions & SIMDUTF_RISCV_HWPROBE_IMA_V)
      host_isa |= instruction_set::RVV;
    if (extensions & SIMDUTF_RISCV_HWPROBE_EXT_ZVBB)
      host_isa |= instruction_set::ZVBB;
  }
  #endif
  #if defined(RUN_IN_SPIKE_SIMULATOR)
  // Proxy Kernel does not implement yet hwprobe syscall
  host_isa |= instruction_set::RVV;
  #endif
  return host_isa;
}

#elif defined(__aarch64__) || defined(_M_ARM64) || defined(_M_ARM64EC)

static inline uint32_t detect_supported_architectures() {
  return instruction_set::NEON;
}

#elif defined(__x86_64__) || defined(_M_AMD64) // x64

namespace {
namespace cpuid_bit {
// Can be found on Intel ISA Reference for CPUID

// EAX = 0x01
constexpr uint32_t pclmulqdq = uint32_t(1)
                               << 1; ///< @private bit  1 of ECX for EAX=0x1
constexpr uint32_t sse42 = uint32_t(1)
                           << 20; ///< @private bit 20 of ECX for EAX=0x1
constexpr uint32_t osxsave =
    (uint32_t(1) << 26) |
    (uint32_t(1) << 27); ///< @private bits 26+27 of ECX for EAX=0x1

// EAX = 0x7f (Structured Extended Feature Flags), ECX = 0x00 (Sub-leaf)
// See: "Table 3-8. Information Returned by CPUID Instruction"
namespace ebx {
constexpr uint32_t bmi1 = uint32_t(1) << 3;
constexpr uint32_t avx2 = uint32_t(1) << 5;
constexpr uint32_t bmi2 = uint32_t(1) << 8;
constexpr uint32_t avx512f = uint32_t(1) << 16;
constexpr uint32_t avx512dq = uint32_t(1) << 17;
constexpr uint32_t avx512ifma = uint32_t(1) << 21;
constexpr uint32_t avx512cd = uint32_t(1) << 28;
constexpr uint32_t avx512bw = uint32_t(1) << 30;
constexpr uint32_t avx512vl = uint32_t(1) << 31;
} // namespace ebx

namespace ecx {
constexpr uint32_t avx512vbmi = uint32_t(1) << 1;
constexpr uint32_t avx512vbmi2 = uint32_t(1) << 6;
constexpr uint32_t avx512vnni = uint32_t(1) << 11;
constexpr uint32_t avx512bitalg = uint32_t(1) << 12;
constexpr uint32_t avx512vpopcnt = uint32_t(1) << 14;
} // namespace ecx
namespace edx {
constexpr uint32_t avx512vp2intersect = uint32_t(1) << 8;
}
namespace xcr0_bit {
constexpr uint64_t avx256_saved = uint64_t(1) << 2; ///< @private bit 2 = AVX
constexpr uint64_t avx512_saved =
    uint64_t(7) << 5; ///< @private bits 5,6,7 = opmask, ZMM_hi256, hi16_ZMM
} // namespace xcr0_bit
} // namespace cpuid_bit
} // namespace

static inline void cpuid(uint32_t *eax, uint32_t *ebx, uint32_t *ecx,
                         uint32_t *edx) {
  #if defined(_MSC_VER)
  int cpu_info[4];
  __cpuidex(cpu_info, *eax, *ecx);
  *eax = cpu_info[0];
  *ebx = cpu_info[1];
  *ecx = cpu_info[2];
  *edx = cpu_info[3];
  #elif (defined(HAVE_GCC_GET_CPUID) && defined(USE_GCC_GET_CPUID)) ||         \
      defined(__FILC__)
  uint32_t level = *eax;
  __get_cpuid(level, eax, ebx, ecx, edx);
  #else
  uint32_t a = *eax, b, c = *ecx, d;
  asm volatile("cpuid\n\t" : "+a"(a), "=b"(b), "+c"(c), "=d"(d));
  *eax = a;
  *ebx = b;
  *ecx = c;
  *edx = d;
  #endif
}

static inline uint64_t xgetbv() {
  #if defined(_MSC_VER)
  return _xgetbv(0);
  #elif defined(__FILC__)
  return zxgetbv();
  #else
  uint32_t xcr0_lo, xcr0_hi;
  asm volatile("xgetbv\n\t" : "=a"(xcr0_lo), "=d"(xcr0_hi) : "c"(0));
  return xcr0_lo | ((uint64_t)xcr0_hi << 32);
  #endif
}

static inline uint32_t detect_supported_architectures() {
  uint32_t eax;
  uint32_t ebx = 0;
  uint32_t ecx = 0;
  uint32_t edx = 0;
  uint32_t host_isa = 0x0;

  // EBX for EAX=0x1
  eax = 0x1;
  cpuid(&eax, &ebx, &ecx, &edx);

  if (ecx & cpuid_bit::sse42) {
    host_isa |= instruction_set::SSE42;
  }

  if (ecx & cpuid_bit::pclmulqdq) {
    host_isa |= instruction_set::PCLMULQDQ;
  }

  if ((ecx & cpuid_bit::osxsave) != cpuid_bit::osxsave) {
    return host_isa;
  }

  // xgetbv for checking if the OS saves registers
  uint64_t xcr0 = xgetbv();

  if ((xcr0 & cpuid_bit::xcr0_bit::avx256_saved) == 0) {
    return host_isa;
  }
  // ECX for EAX=0x7
  eax = 0x7;
  ecx = 0x0; // Sub-leaf = 0
  cpuid(&eax, &ebx, &ecx, &edx);
  if (ebx & cpuid_bit::ebx::avx2) {
    host_isa |= instruction_set::AVX2;
  }
  if (ebx & cpuid_bit::ebx::bmi1) {
    host_isa |= instruction_set::BMI1;
  }
  if (ebx & cpuid_bit::ebx::bmi2) {
    host_isa |= instruction_set::BMI2;
  }
  if (!((xcr0 & cpuid_bit::xcr0_bit::avx512_saved) ==
        cpuid_bit::xcr0_bit::avx512_saved)) {
    return host_isa;
  }
  if (ebx & cpuid_bit::ebx::avx512f) {
    host_isa |= instruction_set::AVX512F;
  }
  if (ebx & cpuid_bit::ebx::avx512bw) {
    host_isa |= instruction_set::AVX512BW;
  }
  if (ebx & cpuid_bit::ebx::avx512cd) {
    host_isa |= instruction_set::AVX512CD;
  }
  if (ebx & cpuid_bit::ebx::avx512dq) {
    host_isa |= instruction_set::AVX512DQ;
  }
  if (ebx & cpuid_bit::ebx::avx512vl) {
    host_isa |= instruction_set::AVX512VL;
  }
  if (ecx & cpuid_bit::ecx::avx512vbmi2) {
    host_isa |= instruction_set::AVX512VBMI2;
  }
  if (ecx & cpuid_bit::ecx::avx512vpopcnt) {
    host_isa |= instruction_set::AVX512VPOPCNTDQ;
  }
  return host_isa;
}
#elif defined(__loongarch__)

static inline uint32_t detect_supported_architectures() {
  uint32_t host_isa = instruction_set::DEFAULT;
  #if defined(__linux__)
  uint64_t hwcap = 0;
  hwcap = getauxval(AT_HWCAP);
  if (hwcap & HWCAP_LOONGARCH_LSX) {
    host_isa |= instruction_set::LSX;
  }
  if (hwcap & HWCAP_LOONGARCH_LASX) {
    host_isa |= instruction_set::LASX;
  }
  #endif
  return host_isa;
}
#else // fallback

// includes 32-bit ARM.
static inline uint32_t detect_supported_architectures() {
  return instruction_set::DEFAULT;
}

#endif // end SIMD extension detection code

} // namespace internal
} // namespace simdutf

#endif // SIMDutf_INTERNAL_ISADETECTION_H
/* end file include/simdutf/internal/isadetection.h */

#include <string_view>
#if SIMDUTF_SPAN
  #include <concepts>
  #include <type_traits>
  #include <span>
  #include <tuple>
  #include <utility> // for std::unreachable
#endif
// The following defines are conditionally enabled/disabled during amalgamation.
// By default all features are enabled, regular code shouldn't check them. Only
// when user code really relies of a selected subset, it's good to verify these
// flags, like:
//
//      #if !SIMDUTF_FEATURE_UTF16
//      #   error("Please amalgamate simdutf with UTF-16 support")
//      #endif
//
#define SIMDUTF_FEATURE_DETECT_ENCODING 0
#define SIMDUTF_FEATURE_ASCII 0
#define SIMDUTF_FEATURE_LATIN1 0
#define SIMDUTF_FEATURE_UTF8 1
#define SIMDUTF_FEATURE_UTF16 0
#define SIMDUTF_FEATURE_UTF32 1
#define SIMDUTF_FEATURE_BASE64 0

/// helpers placed in namespace detail are not a part of the public API
namespace simdutf {
namespace detail {
namespace {
// this is to avoid including <algorithm> just for min
constexpr std::size_t min(std::size_t a, std::size_t b) {
  return a < b ? a : b;
}
template <typename T, typename U>
constexpr std::size_t min(const T &a, const U &b) = delete;
} // namespace
} // namespace detail
} // namespace simdutf

#if SIMDUTF_CPLUSPLUS23
/* begin file include/simdutf/constexpr_ptr.h */
#ifndef SIMDUTF_CONSTEXPR_PTR_H
#define SIMDUTF_CONSTEXPR_PTR_H

#include <cstddef>

namespace simdutf {
namespace detail {
/**
 * The constexpr_ptr class is a workaround for reinterpret_cast not being
 * allowed during constant evaluation.
 */
template <typename to, typename from>
  requires(sizeof(to) == sizeof(from))
struct constexpr_ptr {
  const from *p;

  constexpr explicit constexpr_ptr(const from *ptr) noexcept : p(ptr) {}

  constexpr to operator*() const noexcept { return static_cast<to>(*p); }

  constexpr constexpr_ptr &operator++() noexcept {
    ++p;
    return *this;
  }

  constexpr constexpr_ptr operator++(int) noexcept {
    auto old = *this;
    ++p;
    return old;
  }

  constexpr constexpr_ptr &operator--() noexcept {
    --p;
    return *this;
  }

  constexpr constexpr_ptr operator--(int) noexcept {
    auto old = *this;
    --p;
    return old;
  }

  constexpr constexpr_ptr &operator+=(std::ptrdiff_t n) noexcept {
    p += n;
    return *this;
  }

  constexpr constexpr_ptr &operator-=(std::ptrdiff_t n) noexcept {
    p -= n;
    return *this;
  }

  constexpr constexpr_ptr operator+(std::ptrdiff_t n) const noexcept {
    return constexpr_ptr{p + n};
  }

  constexpr constexpr_ptr operator-(std::ptrdiff_t n) const noexcept {
    return constexpr_ptr{p - n};
  }

  constexpr std::ptrdiff_t operator-(const constexpr_ptr &o) const noexcept {
    return p - o.p;
  }

  constexpr to operator[](std::ptrdiff_t n) const noexcept {
    return static_cast<to>(*(p + n));
  }

  // to prevent compilation errors for memcpy, even if it is never
  // called during constant evaluation
  constexpr operator const void *() const noexcept { return p; }
};

template <typename to, typename from>
constexpr constexpr_ptr<to, from> constexpr_cast_ptr(from *p) noexcept {
  return constexpr_ptr<to, from>{p};
}

/**
 * helper type for constexpr_writeptr, so it is possible to
 * do "*ptr = val;"
 */
template <typename SrcType, typename TargetType>
struct constexpr_write_ptr_proxy {

  constexpr explicit constexpr_write_ptr_proxy(TargetType *raw) : p(raw) {}

  constexpr constexpr_write_ptr_proxy &operator=(SrcType v) {
    *p = static_cast<TargetType>(v);
    return *this;
  }

  TargetType *p;
};

/**
 * helper for working around reinterpret_cast not being allowed during constexpr
 * evaluation. will try to act as a SrcType* but actually write to the pointer
 * given in the constructor, which is of another type TargetType
 */
template <typename SrcType, typename TargetType> struct constexpr_write_ptr {
  constexpr explicit constexpr_write_ptr(TargetType *raw) : p(raw) {}

  constexpr constexpr_write_ptr_proxy<SrcType, TargetType> operator*() const {
    return constexpr_write_ptr_proxy<SrcType, TargetType>{p};
  }

  constexpr constexpr_write_ptr_proxy<SrcType, TargetType>
  operator[](std::ptrdiff_t n) const {
    return constexpr_write_ptr_proxy<SrcType, TargetType>{p + n};
  }

  constexpr constexpr_write_ptr &operator++() {
    ++p;
    return *this;
  }

  constexpr constexpr_write_ptr operator++(int) {
    constexpr_write_ptr old = *this;
    ++p;
    return old;
  }

  constexpr std::ptrdiff_t operator-(const constexpr_write_ptr &other) const {
    return p - other.p;
  }

  TargetType *p;
};

template <typename SrcType, typename TargetType>
constexpr auto constexpr_cast_writeptr(TargetType *raw) {
  return constexpr_write_ptr<SrcType, TargetType>{raw};
}

} // namespace detail
} // namespace simdutf
#endif
/* end file include/simdutf/constexpr_ptr.h */
#endif

#if SIMDUTF_SPAN
/// helpers placed in namespace detail are not a part of the public API
namespace simdutf {
namespace detail {
/**
 * matches a byte, in the many ways C++ allows. note that these
 * are all distinct types.
 */
template <typename T>
concept byte_like = std::is_same_v<T, std::byte> ||     //
                    std::is_same_v<T, char> ||          //
                    std::is_same_v<T, signed char> ||   //
                    std::is_same_v<T, unsigned char> || //
                    std::is_same_v<T, char8_t>;

template <typename T>
concept is_byte_like = byte_like<std::remove_cvref_t<T>>;

template <typename T>
concept is_pointer = std::is_pointer_v<T>;

/**
 * matches anything that behaves like std::span and points to character-like
 * data such as: std::byte, char, unsigned char, signed char, std::int8_t,
 * std::uint8_t
 */
template <typename T>
concept input_span_of_byte_like = requires(const T &t) {
  { t.size() } noexcept -> std::convertible_to<std::size_t>;
  { t.data() } noexcept -> is_pointer;
  { *t.data() } noexcept -> is_byte_like;
};

template <typename T>
concept is_mutable = !std::is_const_v<std::remove_reference_t<T>>;

/**
 * like span_of_byte_like, but for an output span (intended to be written to)
 */
template <typename T>
concept output_span_of_byte_like = requires(T &t) {
  { t.size() } noexcept -> std::convertible_to<std::size_t>;
  { t.data() } noexcept -> is_pointer;
  { *t.data() } noexcept -> is_byte_like;
  { *t.data() } noexcept -> is_mutable;
};

/**
 * a pointer like object, when indexed, results in a byte like result.
 * valid examples: char*, const char*, std::array<char,10>
 * invalid examples: int*, std::array<int,10>
 */
template <class InputPtr>
concept indexes_into_byte_like = requires(InputPtr p) {
  { std::decay_t<decltype(p[0])>{} } -> simdutf::detail::byte_like;
};
template <class InputPtr>
concept indexes_into_utf16 = requires(InputPtr p) {
  { std::decay_t<decltype(p[0])>{} } -> std::same_as<char16_t>;
};
template <class InputPtr>
concept indexes_into_utf32 = requires(InputPtr p) {
  { std::decay_t<decltype(p[0])>{} } -> std::same_as<char32_t>;
};

template <class InputPtr>
concept index_assignable_from_char = requires(InputPtr p, char s) {
  { p[0] = s };
};

/**
 * a pointer like object that results in a uint32_t when indexed.
 * valid examples: uint32_t*
 */
template <class InputPtr>
concept indexes_into_uint32 = requires(InputPtr p) {
  { std::decay_t<decltype(p[0])>{} } -> std::same_as<std::uint32_t>;
};
} // namespace detail
} // namespace simdutf
#endif // SIMDUTF_SPAN

// these includes are needed for constexpr support. they are
// not part of the public api.
/* begin file include/simdutf/scalar/swap_bytes.h */
#ifndef SIMDUTF_SWAP_BYTES_H
#define SIMDUTF_SWAP_BYTES_H

namespace simdutf {
namespace scalar {

constexpr inline simdutf_warn_unused uint16_t
u16_swap_bytes(const uint16_t word) {
  return uint16_t((word >> 8) | (word << 8));
}

constexpr inline simdutf_warn_unused uint32_t
u32_swap_bytes(const uint32_t word) {
  return ((word >> 24) & 0xff) |      // move byte 3 to byte 0
         ((word << 8) & 0xff0000) |   // move byte 1 to byte 2
         ((word >> 8) & 0xff00) |     // move byte 2 to byte 1
         ((word << 24) & 0xff000000); // byte 0 to byte 3
}

namespace utf32 {
template <endianness big_endian> constexpr uint32_t swap_if_needed(uint32_t c) {
  return !match_system(big_endian) ? scalar::u32_swap_bytes(c) : c;
}
} // namespace utf32

namespace utf16 {
template <endianness big_endian> constexpr uint16_t swap_if_needed(uint16_t c) {
  return !match_system(big_endian) ? scalar::u16_swap_bytes(c) : c;
}
} // namespace utf16

} // namespace scalar
} // namespace simdutf

#endif
/* end file include/simdutf/scalar/swap_bytes.h */
/* begin file include/simdutf/scalar/ascii.h */
#ifndef SIMDUTF_ASCII_H
#define SIMDUTF_ASCII_H

#include <cstring>

namespace simdutf {
namespace scalar {
namespace {
namespace ascii {

template <class InputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires simdutf::detail::indexes_into_byte_like<InputPtr>
#endif
simdutf_warn_unused simdutf_constexpr23 bool validate(InputPtr data,
                                                      size_t len) noexcept {
  uint64_t pos = 0;

#if SIMDUTF_CPLUSPLUS23
  // avoid memcpy during constant evaluation
  if !consteval
#endif
  // process in blocks of 16 bytes when possible
  {
    for (; pos + 16 <= len; pos += 16) {
      uint64_t v1;
      std::memcpy(&v1, data + pos, sizeof(uint64_t));
      uint64_t v2;
      std::memcpy(&v2, data + pos + sizeof(uint64_t), sizeof(uint64_t));
      uint64_t v{v1 | v2};
      if ((v & 0x8080808080808080) != 0) {
        return false;
      }
    }
  }

  // process the tail byte-by-byte
  for (; pos < len; pos++) {
    if (static_cast<std::uint8_t>(data[pos]) >= 0b10000000) {
      return false;
    }
  }
  return true;
}
template <class InputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires simdutf::detail::indexes_into_byte_like<InputPtr>
#endif
simdutf_warn_unused simdutf_constexpr23 result
validate_with_errors(InputPtr data, size_t len) noexcept {
  size_t pos = 0;
#if SIMDUTF_CPLUSPLUS23
  // avoid memcpy during constant evaluation
  if !consteval
#endif
  {
    // process in blocks of 16 bytes when possible
    for (; pos + 16 <= len; pos += 16) {
      uint64_t v1;
      std::memcpy(&v1, data + pos, sizeof(uint64_t));
      uint64_t v2;
      std::memcpy(&v2, data + pos + sizeof(uint64_t), sizeof(uint64_t));
      uint64_t v{v1 | v2};
      if ((v & 0x8080808080808080) != 0) {
        for (; pos < len; pos++) {
          if (static_cast<std::uint8_t>(data[pos]) >= 0b10000000) {
            return result(error_code::TOO_LARGE, pos);
          }
        }
      }
    }
  }

  // process the tail byte-by-byte
  for (; pos < len; pos++) {
    if (static_cast<std::uint8_t>(data[pos]) >= 0b10000000) {
      return result(error_code::TOO_LARGE, pos);
    }
  }
  return result(error_code::SUCCESS, pos);
}

} // namespace ascii
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif
/* end file include/simdutf/scalar/ascii.h */
/* begin file include/simdutf/scalar/atomic_util.h */
#ifndef SIMDUTF_ATOMIC_UTIL_H
#define SIMDUTF_ATOMIC_UTIL_H
#if SIMDUTF_ATOMIC_REF
  #include <atomic>
  #include <cstring>
namespace simdutf {
namespace scalar {

// This function is a memcpy that uses atomic operations to read from the
// source.
inline void memcpy_atomic_read(char *dst, const char *src, size_t len) {
  static_assert(std::atomic_ref<char>::required_alignment == sizeof(char),
                "std::atomic_ref requires the same alignment as char_type");
  // We expect all 64-bit systems to be able to read 64-bit words from an
  // aligned memory region atomically. You might be able to do better on
  // specific systems, e.g., x64 systems can read 128-bit words atomically.
  constexpr size_t alignment = sizeof(uint64_t);

  // Lambda for atomic byte-by-byte copy
  auto bbb_memcpy_atomic_read = [](char *bytedst, const char *bytesrc,
                                   size_t bytelen) noexcept {
    char *mutable_src = const_cast<char *>(bytesrc);
    for (size_t j = 0; j < bytelen; ++j) {
      bytedst[j] =
          std::atomic_ref<char>(mutable_src[j]).load(std::memory_order_relaxed);
    }
  };

  // Handle unaligned start
  size_t offset = reinterpret_cast<std::uintptr_t>(src) % alignment;
  if (offset) {
    size_t to_align = detail::min(len, alignment - offset);
    bbb_memcpy_atomic_read(dst, src, to_align);
    src += to_align;
    dst += to_align;
    len -= to_align;
  }

  // Process aligned 64-bit chunks
  while (len >= alignment) {
    auto *src_aligned = reinterpret_cast<uint64_t *>(const_cast<char *>(src));
    const auto dst_value =
        std::atomic_ref<uint64_t>(*src_aligned).load(std::memory_order_relaxed);
    std::memcpy(dst, &dst_value, sizeof(uint64_t));
    src += alignment;
    dst += alignment;
    len -= alignment;
  }

  // Handle remaining bytes
  if (len) {
    bbb_memcpy_atomic_read(dst, src, len);
  }
}

// This function is a memcpy that uses atomic operations to write to the
// destination.
inline void memcpy_atomic_write(char *dst, const char *src, size_t len) {
  static_assert(std::atomic_ref<char>::required_alignment == sizeof(char),
                "std::atomic_ref requires the same alignment as char");
  // We expect all 64-bit systems to be able to write 64-bit words to an aligned
  // memory region atomically.
  // You might be able to do better on specific systems, e.g., x64 systems can
  // write 128-bit words atomically.
  constexpr size_t alignment = sizeof(uint64_t);

  // Lambda for atomic byte-by-byte write
  auto bbb_memcpy_atomic_write = [](char *bytedst, const char *bytesrc,
                                    size_t bytelen) noexcept {
    for (size_t j = 0; j < bytelen; ++j) {
      std::atomic_ref<char>(bytedst[j])
          .store(bytesrc[j], std::memory_order_relaxed);
    }
  };

  // Handle unaligned start
  size_t offset = reinterpret_cast<std::uintptr_t>(dst) % alignment;
  if (offset) {
    size_t to_align = detail::min(len, alignment - offset);
    bbb_memcpy_atomic_write(dst, src, to_align);
    dst += to_align;
    src += to_align;
    len -= to_align;
  }

  // Process aligned 64-bit chunks
  while (len >= alignment) {
    auto *dst_aligned = reinterpret_cast<uint64_t *>(dst);
    uint64_t src_val;
    std::memcpy(&src_val, src, sizeof(uint64_t)); // Non-atomic read from src
    std::atomic_ref<uint64_t>(*dst_aligned)
        .store(src_val, std::memory_order_relaxed);
    dst += alignment;
    src += alignment;
    len -= alignment;
  }

  // Handle remaining bytes
  if (len) {
    bbb_memcpy_atomic_write(dst, src, len);
  }
}
} // namespace scalar
} // namespace simdutf
#endif // SIMDUTF_ATOMIC_REF
#endif // SIMDUTF_ATOMIC_UTIL_H
/* end file include/simdutf/scalar/atomic_util.h */
/* begin file include/simdutf/scalar/latin1.h */
#ifndef SIMDUTF_LATIN1_H
#define SIMDUTF_LATIN1_H

namespace simdutf {
namespace scalar {
namespace {
namespace latin1 {

simdutf_really_inline size_t utf8_length_from_latin1(const char *buf,
                                                     size_t len) {
  const uint8_t *c = reinterpret_cast<const uint8_t *>(buf);
  size_t answer = 0;
  for (size_t i = 0; i < len; i++) {
    if ((c[i] >> 7)) {
      answer++;
    }
  }
  return answer + len;
}

} // namespace latin1
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif
/* end file include/simdutf/scalar/latin1.h */
/* begin file include/simdutf/scalar/latin1_to_utf16/latin1_to_utf16.h */
#ifndef SIMDUTF_LATIN1_TO_UTF16_H
#define SIMDUTF_LATIN1_TO_UTF16_H

namespace simdutf {
namespace scalar {
namespace {
namespace latin1_to_utf16 {

template <endianness big_endian, typename InputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires simdutf::detail::indexes_into_byte_like<InputPtr>
#endif
simdutf_constexpr23 size_t convert(InputPtr data, size_t len,
                                   char16_t *utf16_output) {
  size_t pos = 0;
  char16_t *start{utf16_output};

  while (pos < len) {
    uint16_t word =
        uint8_t(data[pos]); // extend Latin-1 char to 16-bit Unicode code point
    *utf16_output++ =
        char16_t(match_system(big_endian) ? word : u16_swap_bytes(word));
    pos++;
  }

  return utf16_output - start;
}

template <endianness big_endian>
inline result convert_with_errors(const char *buf, size_t len,
                                  char16_t *utf16_output) {
  const uint8_t *data = reinterpret_cast<const uint8_t *>(buf);
  size_t pos = 0;
  char16_t *start{utf16_output};

  while (pos < len) {
    uint16_t word =
        uint16_t(data[pos]); // extend Latin-1 char to 16-bit Unicode code point
    *utf16_output++ =
        char16_t(match_system(big_endian) ? word : u16_swap_bytes(word));
    pos++;
  }

  return result(error_code::SUCCESS, utf16_output - start);
}

} // namespace latin1_to_utf16
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif
/* end file include/simdutf/scalar/latin1_to_utf16/latin1_to_utf16.h */
/* begin file include/simdutf/scalar/latin1_to_utf32/latin1_to_utf32.h */
#ifndef SIMDUTF_LATIN1_TO_UTF32_H
#define SIMDUTF_LATIN1_TO_UTF32_H

namespace simdutf {
namespace scalar {
namespace {
namespace latin1_to_utf32 {

template <typename InputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires simdutf::detail::indexes_into_byte_like<InputPtr>
#endif
simdutf_constexpr23 size_t convert(InputPtr data, size_t len,
                                   char32_t *utf32_output) {
  char32_t *start{utf32_output};
  for (size_t i = 0; i < len; i++) {
    *utf32_output++ = uint8_t(data[i]);
  }
  return utf32_output - start;
}

} // namespace latin1_to_utf32
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif
/* end file include/simdutf/scalar/latin1_to_utf32/latin1_to_utf32.h */
/* begin file include/simdutf/scalar/latin1_to_utf8/latin1_to_utf8.h */
#ifndef SIMDUTF_LATIN1_TO_UTF8_H
#define SIMDUTF_LATIN1_TO_UTF8_H

#include <cstring>

namespace simdutf {
namespace scalar {
namespace {
namespace latin1_to_utf8 {

template <typename InputPtr, typename OutputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires(simdutf::detail::indexes_into_byte_like<InputPtr> &&
           simdutf::detail::index_assignable_from_char<OutputPtr>)
#endif
simdutf_constexpr23 size_t convert(InputPtr data, size_t len,
                                   OutputPtr utf8_output) {
  // const unsigned char *data = reinterpret_cast<const unsigned char *>(buf);
  size_t pos = 0;
  size_t utf8_pos = 0;

  while (pos < len) {
#if SIMDUTF_CPLUSPLUS23
    if !consteval
#endif
    {
      // try to convert the next block of 16 ASCII bytes
      if (pos + 16 <= len) { // if it is safe to read 16 more bytes, check that
                             // they are ascii
        uint64_t v1;
        ::memcpy(&v1, data + pos, sizeof(uint64_t));
        uint64_t v2;
        ::memcpy(&v2, data + pos + sizeof(uint64_t), sizeof(uint64_t));
        uint64_t v{v1 |
                   v2}; // We are only interested in these bits: 1000 1000 1000
                        // 1000, so it makes sense to concatenate everything
        if ((v & 0x8080808080808080) ==
            0) { // if NONE of these are set, e.g. all of them are zero, then
                 // everything is ASCII
          size_t final_pos = pos + 16;
          while (pos < final_pos) {
            utf8_output[utf8_pos++] = char(data[pos]);
            pos++;
          }
          continue;
        }
      } // if (pos + 16 <= len)
    } // !consteval scope

    unsigned char byte = data[pos];
    if ((byte & 0x80) == 0) { // if ASCII
      // will generate one UTF-8 bytes
      utf8_output[utf8_pos++] = char(byte);
      pos++;
    } else {
      // will generate two UTF-8 bytes
      utf8_output[utf8_pos++] = char((byte >> 6) | 0b11000000);
      utf8_output[utf8_pos++] = char((byte & 0b111111) | 0b10000000);
      pos++;
    }
  } // while
  return utf8_pos;
}

simdutf_really_inline size_t convert(const char *buf, size_t len,
                                     char *utf8_output) {
  return convert(reinterpret_cast<const unsigned char *>(buf), len,
                 utf8_output);
}

inline size_t convert_safe(const char *buf, size_t len, char *utf8_output,
                           size_t utf8_len) {
  const unsigned char *data = reinterpret_cast<const unsigned char *>(buf);
  size_t pos = 0;
  size_t skip_pos = 0;
  size_t utf8_pos = 0;
  while (pos < len && utf8_pos < utf8_len) {
    // try to convert the next block of 16 ASCII bytes
    if (pos >= skip_pos && pos + 16 <= len &&
        utf8_pos + 16 <= utf8_len) { // if it is safe to read 16 more bytes,
                                     // check that they are ascii
      uint64_t v1;
      ::memcpy(&v1, data + pos, sizeof(uint64_t));
      uint64_t v2;
      ::memcpy(&v2, data + pos + sizeof(uint64_t), sizeof(uint64_t));
      uint64_t v{v1 |
                 v2}; // We are only interested in these bits: 1000 1000 1000
                      // 1000, so it makes sense to concatenate everything
      if ((v & 0x8080808080808080) ==
          0) { // if NONE of these are set, e.g. all of them are zero, then
               // everything is ASCII
        ::memcpy(utf8_output + utf8_pos, buf + pos, 16);
        utf8_pos += 16;
        pos += 16;
      } else {
        // At least one of the next 16 bytes are not ASCII, we will process them
        // one by one
        skip_pos = pos + 16;
      }
    } else {
      const auto byte = data[pos];
      if ((byte & 0x80) == 0) { // if ASCII
        // will generate one UTF-8 bytes
        utf8_output[utf8_pos++] = char(byte);
        pos++;
      } else if (utf8_pos + 2 <= utf8_len) {
        // will generate two UTF-8 bytes
        utf8_output[utf8_pos++] = char((byte >> 6) | 0b11000000);
        utf8_output[utf8_pos++] = char((byte & 0b111111) | 0b10000000);
        pos++;
      } else {
        break;
      }
    }
  }
  return utf8_pos;
}

inline full_result convert_safe_with_details(const char *buf, size_t len,
                                             char *utf8_output,
                                             size_t utf8_len) {
  const size_t output_count = convert_safe(buf, len, utf8_output, utf8_len);
  // Recover the consumed input count from the completed output. The runtime
  // safe converter uses this helper only for its short scalar tail.
  size_t input_count = 0;
  size_t counted_output = 0;
  while (input_count < len) {
    const size_t width =
        uint8_t(buf[input_count]) < uint8_t(0x80) ? size_t(1) : size_t(2);
    if (counted_output + width > output_count) {
      break;
    }
    input_count++;
    counted_output += width;
  }
  return full_result(input_count == len ? error_code::SUCCESS
                                        : error_code::OUTPUT_BUFFER_TOO_SMALL,
                     input_count, output_count);
}

template <typename InputPtr, typename OutputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires(simdutf::detail::indexes_into_byte_like<InputPtr> &&
           simdutf::detail::index_assignable_from_char<OutputPtr>)
#endif
simdutf_constexpr23 size_t convert_safe_constexpr(InputPtr data, size_t len,
                                                  OutputPtr utf8_output,
                                                  size_t utf8_len) {
  size_t pos = 0;
  size_t utf8_pos = 0;
  while (pos < len && utf8_pos < utf8_len) {
    const unsigned char byte = data[pos];
    if ((byte & 0x80) == 0) { // if ASCII
      // will generate one UTF-8 bytes
      utf8_output[utf8_pos++] = char(byte);
      pos++;
    } else if (utf8_pos + 2 <= utf8_len) {
      // will generate two UTF-8 bytes
      utf8_output[utf8_pos++] = char((byte >> 6) | 0b11000000);
      utf8_output[utf8_pos++] = char((byte & 0b111111) | 0b10000000);
      pos++;
    } else {
      break;
    }
  }
  return utf8_pos;
}

template <typename InputPtr, typename OutputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires(simdutf::detail::indexes_into_byte_like<InputPtr> &&
           simdutf::detail::index_assignable_from_char<OutputPtr>)
#endif
simdutf_constexpr23 full_result convert_safe_with_details_constexpr(
    InputPtr data, size_t len, OutputPtr utf8_output, size_t utf8_len) {
  const size_t output_count =
      convert_safe_constexpr(data, len, utf8_output, utf8_len);
  size_t input_count = 0;
  size_t counted_output = 0;
  while (input_count < len) {
    const size_t width =
        uint8_t(data[input_count]) < uint8_t(0x80) ? size_t(1) : size_t(2);
    if (counted_output + width > output_count) {
      break;
    }
    input_count++;
    counted_output += width;
  }
  return full_result(input_count == len ? error_code::SUCCESS
                                        : error_code::OUTPUT_BUFFER_TOO_SMALL,
                     input_count, output_count);
}

template <typename InputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires simdutf::detail::indexes_into_byte_like<InputPtr>
#endif
simdutf_constexpr23 simdutf_warn_unused size_t
utf8_length_from_latin1(InputPtr input, size_t length) noexcept {
  size_t answer = length;
  size_t i = 0;

#if SIMDUTF_CPLUSPLUS23
  if !consteval
#endif
  {
    auto pop = [](uint64_t v) {
      return (size_t)(((v >> 7) & UINT64_C(0x0101010101010101)) *
                          UINT64_C(0x0101010101010101) >>
                      56);
    };
    for (; i + 32 <= length; i += 32) {
      uint64_t v;
      memcpy(&v, input + i, 8);
      answer += pop(v);
      memcpy(&v, input + i + 8, sizeof(v));
      answer += pop(v);
      memcpy(&v, input + i + 16, sizeof(v));
      answer += pop(v);
      memcpy(&v, input + i + 24, sizeof(v));
      answer += pop(v);
    }
    for (; i + 8 <= length; i += 8) {
      uint64_t v;
      memcpy(&v, input + i, sizeof(v));
      answer += pop(v);
    }
  } // !consteval scope
  for (; i + 1 <= length; i += 1) {
    answer += static_cast<uint8_t>(input[i]) >> 7;
  }
  return answer;
}

} // namespace latin1_to_utf8
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif
/* end file include/simdutf/scalar/latin1_to_utf8/latin1_to_utf8.h */
/* begin file include/simdutf/scalar/utf16.h */
#ifndef SIMDUTF_UTF16_H
#define SIMDUTF_UTF16_H

namespace simdutf {
namespace scalar {
namespace utf16 {

template <endianness big_endian>
simdutf_warn_unused simdutf_constexpr23 bool
validate_as_ascii(const char16_t *data, size_t len) noexcept {
  for (size_t pos = 0; pos < len; pos++) {
    char16_t word = scalar::utf16::swap_if_needed<big_endian>(data[pos]);
    if (word >= 0x80) {
      return false;
    }
  }
  return true;
}

template <endianness big_endian>
inline simdutf_warn_unused simdutf_constexpr23 bool
validate(const char16_t *data, size_t len) noexcept {
  uint64_t pos = 0;
  while (pos < len) {
    char16_t word = scalar::utf16::swap_if_needed<big_endian>(data[pos]);
    if ((word & 0xF800) == 0xD800) {
      if (pos + 1 >= len) {
        return false;
      }
      char16_t diff = char16_t(word - 0xD800);
      if (diff > 0x3FF) {
        return false;
      }
      char16_t next_word = !match_system(big_endian)
                               ? u16_swap_bytes(data[pos + 1])
                               : data[pos + 1];
      char16_t diff2 = char16_t(next_word - 0xDC00);
      if (diff2 > 0x3FF) {
        return false;
      }
      pos += 2;
    } else {
      pos++;
    }
  }
  return true;
}

template <endianness big_endian>
inline simdutf_warn_unused simdutf_constexpr23 result
validate_with_errors(const char16_t *data, size_t len) noexcept {
  size_t pos = 0;
  while (pos < len) {
    char16_t word = scalar::utf16::swap_if_needed<big_endian>(data[pos]);
    if ((word & 0xF800) == 0xD800) {
      if (pos + 1 >= len) {
        return result(error_code::SURROGATE, pos);
      }
      char16_t diff = char16_t(word - 0xD800);
      if (diff > 0x3FF) {
        return result(error_code::SURROGATE, pos);
      }
      char16_t next_word = !match_system(big_endian)
                               ? u16_swap_bytes(data[pos + 1])
                               : data[pos + 1];
      char16_t diff2 = uint16_t(next_word - 0xDC00);
      if (diff2 > 0x3FF) {
        return result(error_code::SURROGATE, pos);
      }
      pos += 2;
    } else {
      pos++;
    }
  }
  return result(error_code::SUCCESS, pos);
}

template <endianness big_endian>
simdutf_constexpr23 size_t count_code_points(const char16_t *p, size_t len) {
  // We are not BOM aware.
  size_t counter{0};
  for (size_t i = 0; i < len; i++) {
    char16_t word = scalar::utf16::swap_if_needed<big_endian>(p[i]);
    counter += ((word & 0xFC00) != 0xDC00);
  }
  return counter;
}

template <endianness big_endian>
simdutf_constexpr23 size_t utf8_length_from_utf16(const char16_t *p,
                                                  size_t len) {
  // We are not BOM aware.
  size_t counter{0};
  for (size_t i = 0; i < len; i++) {
    char16_t word = scalar::utf16::swap_if_needed<big_endian>(p[i]);
    counter++; // ASCII
    counter += static_cast<size_t>(
        word >
        0x7F); // non-ASCII is at least 2 bytes, surrogates are 2*2 == 4 bytes
    counter += static_cast<size_t>((word > 0x7FF && word <= 0xD7FF) ||
                                   (word >= 0xE000)); // three-byte
  }
  return counter;
}

template <endianness big_endian>
simdutf_constexpr23 size_t utf32_length_from_utf16(const char16_t *p,
                                                   size_t len) {
  // We are not BOM aware.
  size_t counter{0};
  for (size_t i = 0; i < len; i++) {
    char16_t word = scalar::utf16::swap_if_needed<big_endian>(p[i]);
    counter += ((word & 0xFC00) != 0xDC00);
  }
  return counter;
}

simdutf_really_inline simdutf_constexpr23 void
change_endianness_utf16(const char16_t *input, size_t size, char16_t *output) {
  for (size_t i = 0; i < size; i++) {
    *output++ = char16_t(input[i] >> 8 | input[i] << 8);
  }
}

template <endianness big_endian>
simdutf_warn_unused simdutf_constexpr23 size_t
trim_partial_utf16(const char16_t *input, size_t length) {
  if (length == 0) {
    return 0;
  }
  uint16_t last_word = uint16_t(input[length - 1]);
  last_word = scalar::utf16::swap_if_needed<big_endian>(last_word);
  length -= ((last_word & 0xFC00) == 0xD800);
  return length;
}

template <endianness big_endian> constexpr bool is_high_surrogate(char16_t c) {
  c = scalar::utf16::swap_if_needed<big_endian>(c);
  return (0xd800 <= c && c <= 0xdbff);
}

template <endianness big_endian> constexpr bool is_low_surrogate(char16_t c) {
  c = scalar::utf16::swap_if_needed<big_endian>(c);
  return (0xdc00 <= c && c <= 0xdfff);
}

simdutf_unused simdutf_really_inline constexpr bool high_surrogate(char16_t c) {
  return (0xd800 <= c && c <= 0xdbff);
}

template <endianness big_endian>
simdutf_constexpr23 result
utf8_length_from_utf16_with_replacement(const char16_t *p, size_t len) {
  bool any_surrogates = false;
  // We are not BOM aware.
  size_t counter{0};
  for (size_t i = 0; i < len; i++) {
    if (is_high_surrogate<big_endian>(p[i])) {
      any_surrogates = true;
      // surrogate pair
      if (i + 1 < len && is_low_surrogate<big_endian>(p[i + 1])) {
        counter += 4;
        i++; // skip low surrogate
      } else {
        counter += 3; // unpaired high surrogate replaced by U+FFFD
      }
      continue;
    } else if (is_low_surrogate<big_endian>(p[i])) {
      any_surrogates = true;
      counter += 3; // unpaired low surrogate replaced by U+FFFD
      continue;
    }
    char16_t word = !match_system(big_endian) ? u16_swap_bytes(p[i]) : p[i];
    counter++; // at least 1 byte
    counter +=
        static_cast<size_t>(word > 0x7F); // non-ASCII is at least 2 bytes
    counter += static_cast<size_t>(word > 0x7FF); // three-byte
  }
  return {any_surrogates ? error_code::SURROGATE : error_code::SUCCESS,
          counter};
}

// variable templates are a C++14 extension
template <endianness big_endian> constexpr char16_t replacement() {
  return !match_system(big_endian) ? scalar::u16_swap_bytes(0xfffd) : 0xfffd;
}

template <endianness big_endian>
simdutf_constexpr23 void to_well_formed_utf16(const char16_t *input, size_t len,
                                              char16_t *output) {
  const char16_t replacement = utf16::replacement<big_endian>();
  bool high_surrogate_prev = false, high_surrogate, low_surrogate;
  size_t i = 0;
  for (; i < len; i++) {
    char16_t c = input[i];
    high_surrogate = is_high_surrogate<big_endian>(c);
    low_surrogate = is_low_surrogate<big_endian>(c);
    if (high_surrogate_prev && !low_surrogate) {
      output[i - 1] = replacement;
    }

    if (!high_surrogate_prev && low_surrogate) {
      output[i] = replacement;
    } else {
      output[i] = input[i];
    }
    high_surrogate_prev = high_surrogate;
  }

  /* string may not end with high surrogate */
  if (high_surrogate_prev) {
    output[i - 1] = replacement;
  }
}

} // namespace utf16
} // namespace scalar
} // namespace simdutf

#endif
/* end file include/simdutf/scalar/utf16.h */
/* begin file include/simdutf/scalar/utf16_to_latin1/utf16_to_latin1.h */
#ifndef SIMDUTF_UTF16_TO_LATIN1_H
#define SIMDUTF_UTF16_TO_LATIN1_H

#include <cstring> // for std::memcpy

namespace simdutf {
namespace scalar {
namespace {
namespace utf16_to_latin1 {

template <endianness big_endian, typename InputPtr, typename OutputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires(simdutf::detail::indexes_into_utf16<InputPtr> &&
           simdutf::detail::index_assignable_from_char<OutputPtr>)
#endif
simdutf_constexpr23 size_t convert(InputPtr data, size_t len,
                                   OutputPtr latin_output) {
  if (len == 0) {
    return 0;
  }
  size_t pos = 0;
  const auto latin_output_start = latin_output;
  uint16_t word = 0;
  uint16_t too_large = 0;

  while (pos < len) {
    word = !match_system(big_endian) ? u16_swap_bytes(data[pos]) : data[pos];
    too_large |= word;
    *latin_output++ = char(word & 0xFF);
    pos++;
  }
  if ((too_large & 0xFF00) != 0) {
    return 0;
  }

  return latin_output - latin_output_start;
}

template <endianness big_endian, typename InputPtr, typename OutputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires(simdutf::detail::indexes_into_utf16<InputPtr> &&
           simdutf::detail::index_assignable_from_char<OutputPtr>)
#endif
simdutf_constexpr23 result convert_with_errors(InputPtr data, size_t len,
                                               OutputPtr latin_output) {
  if (len == 0) {
    return result(error_code::SUCCESS, 0);
  }
  size_t pos = 0;
  auto start = latin_output;
  uint16_t word;

  while (pos < len) {
#if SIMDUTF_CPLUSPLUS23
    if !consteval
#endif
    {
      if (pos + 16 <= len) { // if it is safe to read 32 more bytes, check that
                             // they are Latin1
        uint64_t v1, v2, v3, v4;
        ::memcpy(&v1, data + pos, sizeof(uint64_t));
        ::memcpy(&v2, data + pos + 4, sizeof(uint64_t));
        ::memcpy(&v3, data + pos + 8, sizeof(uint64_t));
        ::memcpy(&v4, data + pos + 12, sizeof(uint64_t));

        if constexpr (!match_system(big_endian)) {
          v1 = (v1 >> 8) | (v1 << (64 - 8));
        }
        if constexpr (!match_system(big_endian)) {
          v2 = (v2 >> 8) | (v2 << (64 - 8));
        }
        if constexpr (!match_system(big_endian)) {
          v3 = (v3 >> 8) | (v3 << (64 - 8));
        }
        if constexpr (!match_system(big_endian)) {
          v4 = (v4 >> 8) | (v4 << (64 - 8));
        }

        if (((v1 | v2 | v3 | v4) & 0xFF00FF00FF00FF00) == 0) {
          size_t final_pos = pos + 16;
          while (pos < final_pos) {
            *latin_output++ = !match_system(big_endian)
                                  ? char(u16_swap_bytes(data[pos]))
                                  : char(data[pos]);
            pos++;
          }
          continue;
        }
      }
    }

    word = !match_system(big_endian) ? u16_swap_bytes(data[pos]) : data[pos];
    if ((word & 0xFF00) == 0) {
      *latin_output++ = char(word & 0xFF);
      pos++;
    } else {
      return result(error_code::TOO_LARGE, pos);
    }
  }
  return result(error_code::SUCCESS, latin_output - start);
}

} // namespace utf16_to_latin1
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif
/* end file include/simdutf/scalar/utf16_to_latin1/utf16_to_latin1.h */
/* begin file include/simdutf/scalar/utf16_to_latin1/valid_utf16_to_latin1.h */
#ifndef SIMDUTF_VALID_UTF16_TO_LATIN1_H
#define SIMDUTF_VALID_UTF16_TO_LATIN1_H

namespace simdutf {
namespace scalar {
namespace {
namespace utf16_to_latin1 {

template <endianness big_endian, class InputIterator, class OutputIterator>
simdutf_constexpr23 inline size_t
convert_valid_impl(InputIterator data, size_t len,
                   OutputIterator latin_output) {
  static_assert(
      std::is_same<typename std::decay<decltype(*data)>::type, uint16_t>::value,
      "must decay to uint16_t");
  size_t pos = 0;
  const auto start = latin_output;
  uint16_t word = 0;

  while (pos < len) {
    word = !match_system(big_endian) ? u16_swap_bytes(data[pos]) : data[pos];
    *latin_output++ = char(word);
    pos++;
  }

  return latin_output - start;
}

template <endianness big_endian>
simdutf_really_inline size_t convert_valid(const char16_t *buf, size_t len,
                                           char *latin_output) {
  return convert_valid_impl<big_endian>(reinterpret_cast<const uint16_t *>(buf),
                                        len, latin_output);
}
} // namespace utf16_to_latin1
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif
/* end file include/simdutf/scalar/utf16_to_latin1/valid_utf16_to_latin1.h */
/* begin file include/simdutf/scalar/utf16_to_utf32/utf16_to_utf32.h */
#ifndef SIMDUTF_UTF16_TO_UTF32_H
#define SIMDUTF_UTF16_TO_UTF32_H

namespace simdutf {
namespace scalar {
namespace {
namespace utf16_to_utf32 {

template <endianness big_endian>
simdutf_constexpr23 size_t convert(const char16_t *data, size_t len,
                                   char32_t *utf32_output) {
  size_t pos = 0;
  char32_t *start{utf32_output};
  while (pos < len) {
    uint16_t word =
        !match_system(big_endian) ? u16_swap_bytes(data[pos]) : data[pos];
    if ((word & 0xF800) != 0xD800) {
      // No surrogate pair, extend 16-bit word to 32-bit word
      *utf32_output++ = char32_t(word);
      pos++;
    } else {
      // must be a surrogate pair
      uint16_t diff = uint16_t(word - 0xD800);
      if (diff > 0x3FF) {
        return 0;
      }
      if (pos + 1 >= len) {
        return 0;
      } // minimal bound checking
      uint16_t next_word = !match_system(big_endian)
                               ? u16_swap_bytes(data[pos + 1])
                               : data[pos + 1];
      uint16_t diff2 = uint16_t(next_word - 0xDC00);
      if (diff2 > 0x3FF) {
        return 0;
      }
      uint32_t value = (diff << 10) + diff2 + 0x10000;
      *utf32_output++ = char32_t(value);
      pos += 2;
    }
  }
  return utf32_output - start;
}

template <endianness big_endian>
simdutf_constexpr23 result convert_with_errors(const char16_t *data, size_t len,
                                               char32_t *utf32_output) {
  size_t pos = 0;
  char32_t *start{utf32_output};
  while (pos < len) {
    uint16_t word =
        !match_system(big_endian) ? u16_swap_bytes(data[pos]) : data[pos];
    if ((word & 0xF800) != 0xD800) {
      // No surrogate pair, extend 16-bit word to 32-bit word
      *utf32_output++ = char32_t(word);
      pos++;
    } else {
      // must be a surrogate pair
      uint16_t diff = uint16_t(word - 0xD800);
      if (diff > 0x3FF) {
        return result(error_code::SURROGATE, pos);
      }
      if (pos + 1 >= len) {
        return result(error_code::SURROGATE, pos);
      } // minimal bound checking
      uint16_t next_word = !match_system(big_endian)
                               ? u16_swap_bytes(data[pos + 1])
                               : data[pos + 1];
      uint16_t diff2 = uint16_t(next_word - 0xDC00);
      if (diff2 > 0x3FF) {
        return result(error_code::SURROGATE, pos);
      }
      uint32_t value = (diff << 10) + diff2 + 0x10000;
      *utf32_output++ = char32_t(value);
      pos += 2;
    }
  }
  return result(error_code::SUCCESS, utf32_output - start);
}

} // namespace utf16_to_utf32
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif
/* end file include/simdutf/scalar/utf16_to_utf32/utf16_to_utf32.h */
/* begin file include/simdutf/scalar/utf16_to_utf32/valid_utf16_to_utf32.h */
#ifndef SIMDUTF_VALID_UTF16_TO_UTF32_H
#define SIMDUTF_VALID_UTF16_TO_UTF32_H

namespace simdutf {
namespace scalar {
namespace {
namespace utf16_to_utf32 {

template <endianness big_endian>
simdutf_constexpr23 size_t convert_valid(const char16_t *data, size_t len,
                                         char32_t *utf32_output) {
  size_t pos = 0;
  char32_t *start{utf32_output};
  while (pos < len) {
    uint16_t word =
        !match_system(big_endian) ? u16_swap_bytes(data[pos]) : data[pos];
    if ((word & 0xF800) != 0xD800) {
      // No surrogate pair, extend 16-bit word to 32-bit word
      *utf32_output++ = char32_t(word);
      pos++;
    } else {
      // must be a surrogate pair
      uint16_t diff = uint16_t(word - 0xD800);
      if (pos + 1 >= len) {
        return 0;
      } // minimal bound checking
      uint16_t next_word = !match_system(big_endian)
                               ? u16_swap_bytes(data[pos + 1])
                               : data[pos + 1];
      uint16_t diff2 = uint16_t(next_word - 0xDC00);
      uint32_t value = (diff << 10) + diff2 + 0x10000;
      *utf32_output++ = char32_t(value);
      pos += 2;
    }
  }
  return utf32_output - start;
}

} // namespace utf16_to_utf32
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif
/* end file include/simdutf/scalar/utf16_to_utf32/valid_utf16_to_utf32.h */
/* begin file include/simdutf/scalar/utf16_to_utf8/utf16_to_utf8.h */
#ifndef SIMDUTF_UTF16_TO_UTF8_H
#define SIMDUTF_UTF16_TO_UTF8_H

#include <cstring>

namespace simdutf {
namespace scalar {
namespace {
namespace utf16_to_utf8 {

template <endianness big_endian, typename InputPtr, typename OutputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires simdutf::detail::indexes_into_utf16<InputPtr>
// FIXME constrain output as well
#endif
simdutf_constexpr23 size_t convert(InputPtr data, size_t len,
                                   OutputPtr utf8_output) {
  size_t pos = 0;
  const auto start = utf8_output;
  while (pos < len) {
#if SIMDUTF_CPLUSPLUS23
    if !consteval
#endif
    {
      // try to convert the next block of 8 bytes
      if (pos + 4 <= len) { // if it is safe to read 8 more bytes, check that
                            // they are ascii
        uint64_t v;
        ::memcpy(&v, data + pos, sizeof(uint64_t));
        if constexpr (!match_system(big_endian)) {
          v = (v >> 8) | (v << (64 - 8));
        }
        if ((v & 0xFF80FF80FF80FF80) == 0) {
          size_t final_pos = pos + 4;
          while (pos < final_pos) {
            *utf8_output++ = !match_system(big_endian)
                                 ? char(u16_swap_bytes(data[pos]))
                                 : char(data[pos]);
            pos++;
          }
          continue;
        }
      }
    }
    uint16_t word =
        !match_system(big_endian) ? u16_swap_bytes(data[pos]) : data[pos];
    if ((word & 0xFF80) == 0) {
      // will generate one UTF-8 bytes
      *utf8_output++ = char(word);
      pos++;
    } else if ((word & 0xF800) == 0) {
      // will generate two UTF-8 bytes
      // we have 0b110XXXXX 0b10XXXXXX
      *utf8_output++ = char((word >> 6) | 0b11000000);
      *utf8_output++ = char((word & 0b111111) | 0b10000000);
      pos++;
    } else if ((word & 0xF800) != 0xD800) {
      // will generate three UTF-8 bytes
      // we have 0b1110XXXX 0b10XXXXXX 0b10XXXXXX
      *utf8_output++ = char((word >> 12) | 0b11100000);
      *utf8_output++ = char(((word >> 6) & 0b111111) | 0b10000000);
      *utf8_output++ = char((word & 0b111111) | 0b10000000);
      pos++;
    } else {
      // must be a surrogate pair
      if (pos + 1 >= len) {
        return 0;
      }
      uint16_t diff = uint16_t(word - 0xD800);
      if (diff > 0x3FF) {
        return 0;
      }
      uint16_t next_word = !match_system(big_endian)
                               ? u16_swap_bytes(data[pos + 1])
                               : data[pos + 1];
      uint16_t diff2 = uint16_t(next_word - 0xDC00);
      if (diff2 > 0x3FF) {
        return 0;
      }
      uint32_t value = (diff << 10) + diff2 + 0x10000;
      // will generate four UTF-8 bytes
      // we have 0b11110XXX 0b10XXXXXX 0b10XXXXXX 0b10XXXXXX
      *utf8_output++ = char((value >> 18) | 0b11110000);
      *utf8_output++ = char(((value >> 12) & 0b111111) | 0b10000000);
      *utf8_output++ = char(((value >> 6) & 0b111111) | 0b10000000);
      *utf8_output++ = char((value & 0b111111) | 0b10000000);
      pos += 2;
    }
  }
  return utf8_output - start;
}

template <endianness big_endian, bool check_output = false, typename InputPtr,
          typename OutputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires(simdutf::detail::indexes_into_utf16<InputPtr> &&
           simdutf::detail::index_assignable_from_char<OutputPtr>)
#endif
simdutf_constexpr23 full_result convert_with_errors(InputPtr data, size_t len,
                                                    OutputPtr utf8_output,
                                                    size_t utf8_len = 0) {
  if (check_output && utf8_len == 0) {
    return full_result(error_code::OUTPUT_BUFFER_TOO_SMALL, 0, 0);
  }

  size_t pos = 0;
  auto start = utf8_output;
  auto end = utf8_output + utf8_len;

  while (pos < len) {
#if SIMDUTF_CPLUSPLUS23
    if !consteval
#endif
    {
      // try to convert the next block of 8 bytes
      if (pos + 4 <= len) { // if it is safe to read 8 more bytes, check that
                            // they are ascii
        uint64_t v;
        ::memcpy(&v, data + pos, sizeof(uint64_t));
        if constexpr (!match_system(big_endian))
          v = (v >> 8) | (v << (64 - 8));
        if ((v & 0xFF80FF80FF80FF80) == 0) {
          size_t final_pos = pos + 4;
          while (pos < final_pos) {
            if (check_output && size_t(end - utf8_output) < 1) {
              return full_result(error_code::OUTPUT_BUFFER_TOO_SMALL, pos,
                                 utf8_output - start);
            }
            *utf8_output++ = !match_system(big_endian)
                                 ? char(u16_swap_bytes(data[pos]))
                                 : char(data[pos]);
            pos++;
          }
          continue;
        }
      }
    }

    uint16_t word =
        !match_system(big_endian) ? u16_swap_bytes(data[pos]) : data[pos];
    if ((word & 0xFF80) == 0) {
      // will generate one UTF-8 bytes
      if (check_output && size_t(end - utf8_output) < 1) {
        return full_result(error_code::OUTPUT_BUFFER_TOO_SMALL, pos,
                           utf8_output - start);
      }
      *utf8_output++ = char(word);
      pos++;
    } else if ((word & 0xF800) == 0) {
      // will generate two UTF-8 bytes
      // we have 0b110XXXXX 0b10XXXXXX
      if (check_output && size_t(end - utf8_output) < 2) {
        return full_result(error_code::OUTPUT_BUFFER_TOO_SMALL, pos,
                           utf8_output - start);
      }
      *utf8_output++ = char((word >> 6) | 0b11000000);
      *utf8_output++ = char((word & 0b111111) | 0b10000000);
      pos++;

    } else if ((word & 0xF800) != 0xD800) {
      // will generate three UTF-8 bytes
      // we have 0b1110XXXX 0b10XXXXXX 0b10XXXXXX
      if (check_output && size_t(end - utf8_output) < 3) {
        return full_result(error_code::OUTPUT_BUFFER_TOO_SMALL, pos,
                           utf8_output - start);
      }
      *utf8_output++ = char((word >> 12) | 0b11100000);
      *utf8_output++ = char(((word >> 6) & 0b111111) | 0b10000000);
      *utf8_output++ = char((word & 0b111111) | 0b10000000);
      pos++;
    } else {

      if (check_output && size_t(end - utf8_output) < 4) {
        return full_result(error_code::OUTPUT_BUFFER_TOO_SMALL, pos,
                           utf8_output - start);
      }
      // must be a surrogate pair
      if (pos + 1 >= len) {
        return full_result(error_code::SURROGATE, pos, utf8_output - start);
      }
      uint16_t diff = uint16_t(word - 0xD800);
      if (diff > 0x3FF) {
        return full_result(error_code::SURROGATE, pos, utf8_output - start);
      }
      uint16_t next_word = !match_system(big_endian)
                               ? u16_swap_bytes(data[pos + 1])
                               : data[pos + 1];
      uint16_t diff2 = uint16_t(next_word - 0xDC00);
      if (diff2 > 0x3FF) {
        return full_result(error_code::SURROGATE, pos, utf8_output - start);
      }
      uint32_t value = (diff << 10) + diff2 + 0x10000;
      // will generate four UTF-8 bytes
      // we have 0b11110XXX 0b10XXXXXX 0b10XXXXXX 0b10XXXXXX
      *utf8_output++ = char((value >> 18) | 0b11110000);
      *utf8_output++ = char(((value >> 12) & 0b111111) | 0b10000000);
      *utf8_output++ = char(((value >> 6) & 0b111111) | 0b10000000);
      *utf8_output++ = char((value & 0b111111) | 0b10000000);
      pos += 2;
    }
  }
  return full_result(error_code::SUCCESS, pos, utf8_output - start);
}

template <endianness big_endian>
inline result simple_convert_with_errors(const char16_t *buf, size_t len,
                                         char *utf8_output) {
  return convert_with_errors<big_endian, false>(buf, len, utf8_output, 0);
}

template <endianness big_endian>
simdutf_constexpr23 size_t convert_with_replacement(const char16_t *data,
                                                    size_t len,
                                                    char *utf8_output) {
  size_t pos = 0;
  char *start = utf8_output;
  while (pos < len) {
#if SIMDUTF_CPLUSPLUS23
    if !consteval
#endif
    {
      // try to convert the next block of 8 bytes
      if (pos + 4 <= len) { // if it is safe to read 8 more bytes, check that
                            // they are ascii
        uint64_t v;
        ::memcpy(&v, data + pos, sizeof(uint64_t));
        if constexpr (!match_system(big_endian)) {
          v = (v >> 8) | (v << (64 - 8));
        }
        if ((v & 0xFF80FF80FF80FF80) == 0) {
          size_t final_pos = pos + 4;
          while (pos < final_pos) {
            *utf8_output++ = !match_system(big_endian)
                                 ? char(u16_swap_bytes(data[pos]))
                                 : char(data[pos]);
            pos++;
          }
          continue;
        }
      }
    }
    uint16_t word =
        !match_system(big_endian) ? u16_swap_bytes(data[pos]) : data[pos];
    if ((word & 0xFF80) == 0) {
      // will generate one UTF-8 bytes
      *utf8_output++ = char(word);
      pos++;
    } else if ((word & 0xF800) == 0) {
      // will generate two UTF-8 bytes
      // we have 0b110XXXXX 0b10XXXXXX
      *utf8_output++ = char((word >> 6) | 0b11000000);
      *utf8_output++ = char((word & 0b111111) | 0b10000000);
      pos++;
    } else if ((word & 0xF800) != 0xD800) {
      // will generate three UTF-8 bytes
      // we have 0b1110XXXX 0b10XXXXXX 0b10XXXXXX
      *utf8_output++ = char((word >> 12) | 0b11100000);
      *utf8_output++ = char(((word >> 6) & 0b111111) | 0b10000000);
      *utf8_output++ = char((word & 0b111111) | 0b10000000);
      pos++;
    } else {
      // surrogate range
      uint16_t diff = uint16_t(word - 0xD800);
      if (diff <= 0x3FF && pos + 1 < len) {
        // high surrogate, check for valid pair
        uint16_t next_word = !match_system(big_endian)
                                 ? u16_swap_bytes(data[pos + 1])
                                 : data[pos + 1];
        uint16_t diff2 = uint16_t(next_word - 0xDC00);
        if (diff2 <= 0x3FF) {
          // valid surrogate pair
          uint32_t value = (diff << 10) + diff2 + 0x10000;
          // will generate four UTF-8 bytes
          *utf8_output++ = char((value >> 18) | 0b11110000);
          *utf8_output++ = char(((value >> 12) & 0b111111) | 0b10000000);
          *utf8_output++ = char(((value >> 6) & 0b111111) | 0b10000000);
          *utf8_output++ = char((value & 0b111111) | 0b10000000);
          pos += 2;
          continue;
        }
      }
      // unpaired surrogate: replace with U+FFFD (0xEF 0xBF 0xBD)
      *utf8_output++ = char(0xef);
      *utf8_output++ = char(0xbf);
      *utf8_output++ = char(0xbd);
      pos++;
    }
  }
  return utf8_output - start;
}

template <endianness big_endian, typename InputPtr, typename OutputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires(simdutf::detail::indexes_into_utf16<InputPtr> &&
           simdutf::detail::index_assignable_from_char<OutputPtr>)
#endif
simdutf_constexpr23 full_result convert_with_replacement_safe(
    InputPtr data, size_t len, OutputPtr utf8_output, size_t utf8_len) {
  if (len == 0) {
    return full_result(error_code::SUCCESS, 0, 0);
  }
  if (utf8_len == 0) {
    return full_result(error_code::OUTPUT_BUFFER_TOO_SMALL, 0, 0);
  }

  size_t input_count = 0;
  size_t output_count = 0;
  while (input_count < len) {
    full_result r = convert_with_errors<big_endian, true>(
        data + input_count, len - input_count, utf8_output + output_count,
        utf8_len - output_count);
    input_count += r.input_count;
    output_count += r.output_count;

    if (r.error == error_code::SUCCESS) {
      return full_result(error_code::SUCCESS, input_count, output_count);
    }

    if (r.error == error_code::OUTPUT_BUFFER_TOO_SMALL) {
      if (utf8_len - output_count < 3) {
        return full_result(r.error, input_count, output_count);
      }

      uint16_t word = !match_system(big_endian)
                          ? u16_swap_bytes(data[input_count])
                          : data[input_count];
      bool unpaired = (word & 0xfc00) == 0xdc00;
      if ((word & 0xfc00) == 0xd800) {
        if (input_count + 1 == len) {
          unpaired = true;
        } else {
          const uint16_t next_word = !match_system(big_endian)
                                         ? u16_swap_bytes(data[input_count + 1])
                                         : data[input_count + 1];
          unpaired = (next_word & 0xfc00) != 0xdc00;
        }
      }
      if (!unpaired) {
        return full_result(r.error, input_count, output_count);
      }
    } else if (r.error != error_code::SURROGATE) {
      return full_result(r.error, input_count, output_count);
    }

    if (utf8_len - output_count < 3) {
      return full_result(error_code::OUTPUT_BUFFER_TOO_SMALL, input_count,
                         output_count);
    }
    utf8_output[output_count++] = char(0xef);
    utf8_output[output_count++] = char(0xbf);
    utf8_output[output_count++] = char(0xbd);
    input_count++;
  }
  return full_result(error_code::SUCCESS, input_count, output_count);
}

} // namespace utf16_to_utf8
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif
/* end file include/simdutf/scalar/utf16_to_utf8/utf16_to_utf8.h */
/* begin file include/simdutf/scalar/utf16_to_utf8/valid_utf16_to_utf8.h */
#ifndef SIMDUTF_VALID_UTF16_TO_UTF8_H
#define SIMDUTF_VALID_UTF16_TO_UTF8_H

#include <cstring>

namespace simdutf {
namespace scalar {
namespace {
namespace utf16_to_utf8 {

template <endianness big_endian, typename InputPtr, typename OutputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires(simdutf::detail::indexes_into_utf16<InputPtr> &&
           simdutf::detail::index_assignable_from_char<OutputPtr>)
#endif
simdutf_constexpr23 size_t convert_valid(InputPtr data, size_t len,
                                         OutputPtr utf8_output) {
  size_t pos = 0;
  auto start = utf8_output;
  while (pos < len) {
#if SIMDUTF_CPLUSPLUS23
    if !consteval
#endif
    {
      // try to convert the next block of 4 ASCII characters
      if (pos + 4 <= len) { // if it is safe to read 8 more bytes, check that
                            // they are ascii
        uint64_t v;
        ::memcpy(&v, data + pos, sizeof(uint64_t));
        if constexpr (!match_system(big_endian)) {
          v = (v >> 8) | (v << (64 - 8));
        }
        if ((v & 0xFF80FF80FF80FF80) == 0) {
          size_t final_pos = pos + 4;
          while (pos < final_pos) {
            *utf8_output++ = !match_system(big_endian)
                                 ? char(u16_swap_bytes(data[pos]))
                                 : char(data[pos]);
            pos++;
          }
          continue;
        }
      }
    }

    uint16_t word =
        !match_system(big_endian) ? u16_swap_bytes(data[pos]) : data[pos];
    if ((word & 0xFF80) == 0) {
      // will generate one UTF-8 bytes
      *utf8_output++ = char(word);
      pos++;
    } else if ((word & 0xF800) == 0) {
      // will generate two UTF-8 bytes
      // we have 0b110XXXXX 0b10XXXXXX
      *utf8_output++ = char((word >> 6) | 0b11000000);
      *utf8_output++ = char((word & 0b111111) | 0b10000000);
      pos++;
    } else if ((word & 0xF800) != 0xD800) {
      // will generate three UTF-8 bytes
      // we have 0b1110XXXX 0b10XXXXXX 0b10XXXXXX
      *utf8_output++ = char((word >> 12) | 0b11100000);
      *utf8_output++ = char(((word >> 6) & 0b111111) | 0b10000000);
      *utf8_output++ = char((word & 0b111111) | 0b10000000);
      pos++;
    } else {
      // must be a surrogate pair
      uint16_t diff = uint16_t(word - 0xD800);
      if (pos + 1 >= len) {
        return 0;
      } // minimal bound checking
      uint16_t next_word = !match_system(big_endian)
                               ? u16_swap_bytes(data[pos + 1])
                               : data[pos + 1];
      uint16_t diff2 = uint16_t(next_word - 0xDC00);
      uint32_t value = (diff << 10) + diff2 + 0x10000;
      // will generate four UTF-8 bytes
      // we have 0b11110XXX 0b10XXXXXX 0b10XXXXXX 0b10XXXXXX
      *utf8_output++ = char((value >> 18) | 0b11110000);
      *utf8_output++ = char(((value >> 12) & 0b111111) | 0b10000000);
      *utf8_output++ = char(((value >> 6) & 0b111111) | 0b10000000);
      *utf8_output++ = char((value & 0b111111) | 0b10000000);
      pos += 2;
    }
  }
  return utf8_output - start;
}

} // namespace utf16_to_utf8
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif
/* end file include/simdutf/scalar/utf16_to_utf8/valid_utf16_to_utf8.h */
/* begin file include/simdutf/scalar/utf32.h */
#ifndef SIMDUTF_UTF32_H
#define SIMDUTF_UTF32_H

namespace simdutf {
namespace scalar {
namespace utf32 {

template <typename InputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires simdutf::detail::indexes_into_uint32<InputPtr>
#endif
simdutf_warn_unused simdutf_constexpr23 bool validate(InputPtr data,
                                                      size_t len) noexcept {
  uint64_t pos = 0;
  for (; pos < len; pos++) {
    uint32_t word = data[pos];
    if (word > 0x10FFFF || (word >= 0xD800 && word <= 0xDFFF)) {
      return false;
    }
  }
  return true;
}

simdutf_warn_unused simdutf_really_inline bool validate(const char32_t *buf,
                                                        size_t len) noexcept {
  return validate(reinterpret_cast<const uint32_t *>(buf), len);
}

template <typename InputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires simdutf::detail::indexes_into_uint32<InputPtr>
#endif
simdutf_warn_unused simdutf_constexpr23 result
validate_with_errors(InputPtr data, size_t len) noexcept {
  size_t pos = 0;
  for (; pos < len; pos++) {
    uint32_t word = data[pos];
    if (word > 0x10FFFF) {
      return result(error_code::TOO_LARGE, pos);
    }
    if (word >= 0xD800 && word <= 0xDFFF) {
      return result(error_code::SURROGATE, pos);
    }
  }
  return result(error_code::SUCCESS, pos);
}

simdutf_warn_unused simdutf_really_inline result
validate_with_errors(const char32_t *buf, size_t len) noexcept {
  return validate_with_errors(reinterpret_cast<const uint32_t *>(buf), len);
}

inline simdutf_constexpr23 size_t utf8_length_from_utf32(const char32_t *p,
                                                         size_t len) {
  // We are not BOM aware.
  size_t counter{0};
  for (size_t i = 0; i < len; i++) {
    // credit: @ttsugriy  for the vectorizable approach
    counter++;                                     // ASCII
    counter += static_cast<size_t>(p[i] > 0x7F);   // two-byte
    counter += static_cast<size_t>(p[i] > 0x7FF);  // three-byte
    counter += static_cast<size_t>(p[i] > 0xFFFF); // four-bytes
  }
  return counter;
}

inline simdutf_warn_unused simdutf_constexpr23 size_t
utf16_length_from_utf32(const char32_t *p, size_t len) {
  // We are not BOM aware.
  size_t counter{0};
  for (size_t i = 0; i < len; i++) {
    counter++;                                     // non-surrogate word
    counter += static_cast<size_t>(p[i] > 0xFFFF); // surrogate pair
  }
  return counter;
}

} // namespace utf32
} // namespace scalar
} // namespace simdutf

#endif
/* end file include/simdutf/scalar/utf32.h */
/* begin file include/simdutf/scalar/utf32_to_latin1/utf32_to_latin1.h */
#ifndef SIMDUTF_UTF32_TO_LATIN1_H
#define SIMDUTF_UTF32_TO_LATIN1_H

#include <cstring>

namespace simdutf {
namespace scalar {
namespace {
namespace utf32_to_latin1 {

inline simdutf_constexpr23 size_t convert(const char32_t *data, size_t len,
                                          char *latin1_output) {
  char *start = latin1_output;
  uint32_t utf32_char;
  size_t pos = 0;
  uint32_t too_large = 0;

  while (pos < len) {
    utf32_char = (uint32_t)data[pos];
    too_large |= utf32_char;
    *latin1_output++ = (char)(utf32_char & 0xFF);
    pos++;
  }
  if ((too_large & 0xFFFFFF00) != 0) {
    return 0;
  }
  return latin1_output - start;
}

inline simdutf_constexpr23 result convert_with_errors(const char32_t *data,
                                                      size_t len,
                                                      char *latin1_output) {
  char *start{latin1_output};
  size_t pos = 0;
  while (pos < len) {
#if SIMDUTF_CPLUSPLUS23
    if !consteval
#endif
    {
      if (pos + 2 <= len) { // if it is safe to read 8 more bytes, check that
                            // they are Latin1
        uint64_t v;
        ::memcpy(&v, data + pos, sizeof(uint64_t));
        if ((v & 0xFFFFFF00FFFFFF00) == 0) {
          *latin1_output++ = char(data[pos]);
          *latin1_output++ = char(data[pos + 1]);
          pos += 2;
          continue;
        }
      }
    }

    uint32_t utf32_char = data[pos];
    if ((utf32_char & 0xFFFFFF00) ==
        0) { // Check if the character can be represented in Latin-1
      *latin1_output++ = (char)(utf32_char & 0xFF);
      pos++;
    } else {
      return result(error_code::TOO_LARGE, pos);
    };
  }
  return result(error_code::SUCCESS, latin1_output - start);
}

} // namespace utf32_to_latin1
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif
/* end file include/simdutf/scalar/utf32_to_latin1/utf32_to_latin1.h */
/* begin file include/simdutf/scalar/utf32_to_latin1/valid_utf32_to_latin1.h */
#ifndef SIMDUTF_VALID_UTF32_TO_LATIN1_H
#define SIMDUTF_VALID_UTF32_TO_LATIN1_H

#include <cstring>

namespace simdutf {
namespace scalar {
namespace {
namespace utf32_to_latin1 {

template <typename ReadPtr, typename WritePtr>
simdutf_constexpr23 size_t convert_valid(ReadPtr data, size_t len,
                                         WritePtr latin1_output) {
  static_assert(
      std::is_same<typename std::decay<decltype(*data)>::type, uint32_t>::value,
      "dereferencing the data pointer must result in a uint32_t");
  auto start = latin1_output;
  uint32_t utf32_char;
  size_t pos = 0;

  while (pos < len) {
    utf32_char = data[pos];

#if SIMDUTF_CPLUSPLUS23
    // avoid using the 8 byte at a time optimization in constant evaluation
    // mode. memcpy can't be used and replacing it with bitwise or gave worse
    // codegen (when not during constant evaluation).
    if !consteval {
#endif
      if (pos + 2 <= len) {
        // if it is safe to read 8 more bytes, check that they are Latin1
        uint64_t v;
        std::memcpy(&v, data + pos, sizeof(uint64_t));
        if ((v & 0xFFFFFF00FFFFFF00) == 0) {
          *latin1_output++ = char(data[pos]);
          *latin1_output++ = char(data[pos + 1]);
          pos += 2;
          continue;
        } else {
          // output can not be represented in latin1
          return 0;
        }
      }
#if SIMDUTF_CPLUSPLUS23
    } // if ! consteval
#endif
    if ((utf32_char & 0xFFFFFF00) == 0) {
      *latin1_output++ = char(utf32_char);
    } else {
      // output can not be represented in latin1
      return 0;
    }
    pos++;
  }
  return latin1_output - start;
}

simdutf_really_inline size_t convert_valid(const char32_t *buf, size_t len,
                                           char *latin1_output) {
  return convert_valid(reinterpret_cast<const uint32_t *>(buf), len,
                       latin1_output);
}

} // namespace utf32_to_latin1
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif
/* end file include/simdutf/scalar/utf32_to_latin1/valid_utf32_to_latin1.h */
/* begin file include/simdutf/scalar/utf32_to_utf16/utf32_to_utf16.h */
#ifndef SIMDUTF_UTF32_TO_UTF16_H
#define SIMDUTF_UTF32_TO_UTF16_H

namespace simdutf {
namespace scalar {
namespace {
namespace utf32_to_utf16 {

template <endianness big_endian>
simdutf_constexpr23 size_t convert(const char32_t *data, size_t len,
                                   char16_t *utf16_output) {
  size_t pos = 0;
  char16_t *start{utf16_output};
  while (pos < len) {
    uint32_t word = data[pos];
    if ((word & 0xFFFF0000) == 0) {
      if (word >= 0xD800 && word <= 0xDFFF) {
        return 0;
      }
      // will not generate a surrogate pair
      *utf16_output++ = !match_system(big_endian)
                            ? char16_t(u16_swap_bytes(uint16_t(word)))
                            : char16_t(word);
    } else {
      // will generate a surrogate pair
      if (word > 0x10FFFF) {
        return 0;
      }
      word -= 0x10000;
      uint16_t high_surrogate = uint16_t(0xD800 + (word >> 10));
      uint16_t low_surrogate = uint16_t(0xDC00 + (word & 0x3FF));
      if constexpr (!match_system(big_endian)) {
        high_surrogate = u16_swap_bytes(high_surrogate);
        low_surrogate = u16_swap_bytes(low_surrogate);
      }
      *utf16_output++ = char16_t(high_surrogate);
      *utf16_output++ = char16_t(low_surrogate);
    }
    pos++;
  }
  return utf16_output - start;
}

template <endianness big_endian>
simdutf_constexpr23 result convert_with_errors(const char32_t *data, size_t len,
                                               char16_t *utf16_output) {
  size_t pos = 0;
  char16_t *start{utf16_output};
  while (pos < len) {
    uint32_t word = data[pos];
    if ((word & 0xFFFF0000) == 0) {
      if (word >= 0xD800 && word <= 0xDFFF) {
        return result(error_code::SURROGATE, pos);
      }
      // will not generate a surrogate pair
      *utf16_output++ = !match_system(big_endian)
                            ? char16_t(u16_swap_bytes(uint16_t(word)))
                            : char16_t(word);
    } else {
      // will generate a surrogate pair
      if (word > 0x10FFFF) {
        return result(error_code::TOO_LARGE, pos);
      }
      word -= 0x10000;
      uint16_t high_surrogate = uint16_t(0xD800 + (word >> 10));
      uint16_t low_surrogate = uint16_t(0xDC00 + (word & 0x3FF));
      if constexpr (!match_system(big_endian)) {
        high_surrogate = u16_swap_bytes(high_surrogate);
        low_surrogate = u16_swap_bytes(low_surrogate);
      }
      *utf16_output++ = char16_t(high_surrogate);
      *utf16_output++ = char16_t(low_surrogate);
    }
    pos++;
  }
  return result(error_code::SUCCESS, utf16_output - start);
}

} // namespace utf32_to_utf16
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif
/* end file include/simdutf/scalar/utf32_to_utf16/utf32_to_utf16.h */
/* begin file include/simdutf/scalar/utf32_to_utf16/valid_utf32_to_utf16.h */
#ifndef SIMDUTF_VALID_UTF32_TO_UTF16_H
#define SIMDUTF_VALID_UTF32_TO_UTF16_H

namespace simdutf {
namespace scalar {
namespace {
namespace utf32_to_utf16 {

template <endianness big_endian>
simdutf_constexpr23 size_t convert_valid(const char32_t *data, size_t len,
                                         char16_t *utf16_output) {
  size_t pos = 0;
  char16_t *start{utf16_output};
  while (pos < len) {
    uint32_t word = data[pos];
    if ((word & 0xFFFF0000) == 0) {
      // will not generate a surrogate pair
      *utf16_output++ = !match_system(big_endian)
                            ? char16_t(u16_swap_bytes(uint16_t(word)))
                            : char16_t(word);
      pos++;
    } else {
      // will generate a surrogate pair
      word -= 0x10000;
      uint16_t high_surrogate = uint16_t(0xD800 + (word >> 10));
      uint16_t low_surrogate = uint16_t(0xDC00 + (word & 0x3FF));
      if constexpr (!match_system(big_endian)) {
        high_surrogate = u16_swap_bytes(high_surrogate);
        low_surrogate = u16_swap_bytes(low_surrogate);
      }
      *utf16_output++ = char16_t(high_surrogate);
      *utf16_output++ = char16_t(low_surrogate);
      pos++;
    }
  }
  return utf16_output - start;
}

} // namespace utf32_to_utf16
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif
/* end file include/simdutf/scalar/utf32_to_utf16/valid_utf32_to_utf16.h */
/* begin file include/simdutf/scalar/utf32_to_utf8/utf32_to_utf8.h */
#ifndef SIMDUTF_UTF32_TO_UTF8_H
#define SIMDUTF_UTF32_TO_UTF8_H

#include <cstring>

namespace simdutf {
namespace scalar {
namespace {
namespace utf32_to_utf8 {

template <typename InputPtr, typename OutputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires(simdutf::detail::indexes_into_utf32<InputPtr> &&
           simdutf::detail::index_assignable_from_char<OutputPtr>)
#endif
simdutf_constexpr23 size_t convert(InputPtr data, size_t len,
                                   OutputPtr utf8_output) {
  size_t pos = 0;
  auto start = utf8_output;
  while (pos < len) {
#if SIMDUTF_CPLUSPLUS23
    if !consteval
#endif
    { // try to convert the next block of 2 ASCII characters
      if (pos + 2 <= len) { // if it is safe to read 8 more bytes, check that
                            // they are ascii
        uint64_t v;
        ::memcpy(&v, data + pos, sizeof(uint64_t));
        if ((v & 0xFFFFFF80FFFFFF80) == 0) {
          *utf8_output++ = char(data[pos]);
          *utf8_output++ = char(data[pos + 1]);
          pos += 2;
          continue;
        }
      }
    }

    uint32_t word = data[pos];
    if ((word & 0xFFFFFF80) == 0) {
      // will generate one UTF-8 bytes
      *utf8_output++ = char(word);
      pos++;
    } else if ((word & 0xFFFFF800) == 0) {
      // will generate two UTF-8 bytes
      // we have 0b110XXXXX 0b10XXXXXX
      *utf8_output++ = char((word >> 6) | 0b11000000);
      *utf8_output++ = char((word & 0b111111) | 0b10000000);
      pos++;
    } else if ((word & 0xFFFF0000) == 0) {
      // will generate three UTF-8 bytes
      // we have 0b1110XXXX 0b10XXXXXX 0b10XXXXXX
      if (word >= 0xD800 && word <= 0xDFFF) {
        return 0;
      }
      *utf8_output++ = char((word >> 12) | 0b11100000);
      *utf8_output++ = char(((word >> 6) & 0b111111) | 0b10000000);
      *utf8_output++ = char((word & 0b111111) | 0b10000000);
      pos++;
    } else {
      // will generate four UTF-8 bytes
      // we have 0b11110XXX 0b10XXXXXX 0b10XXXXXX 0b10XXXXXX
      if (word > 0x10FFFF) {
        return 0;
      }
      *utf8_output++ = char((word >> 18) | 0b11110000);
      *utf8_output++ = char(((word >> 12) & 0b111111) | 0b10000000);
      *utf8_output++ = char(((word >> 6) & 0b111111) | 0b10000000);
      *utf8_output++ = char((word & 0b111111) | 0b10000000);
      pos++;
    }
  }
  return utf8_output - start;
}

template <typename InputPtr, typename OutputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires(simdutf::detail::indexes_into_utf32<InputPtr> &&
           simdutf::detail::index_assignable_from_char<OutputPtr>)
#endif
simdutf_constexpr23 result convert_with_errors(InputPtr data, size_t len,
                                               OutputPtr utf8_output) {
  size_t pos = 0;
  auto start = utf8_output;
  while (pos < len) {
#if SIMDUTF_CPLUSPLUS23
    if !consteval
#endif
    { // try to convert the next block of 2 ASCII characters
      if (pos + 2 <= len) { // if it is safe to read 8 more bytes, check that
                            // they are ascii
        uint64_t v;
        ::memcpy(&v, data + pos, sizeof(uint64_t));
        if ((v & 0xFFFFFF80FFFFFF80) == 0) {
          *utf8_output++ = char(data[pos]);
          *utf8_output++ = char(data[pos + 1]);
          pos += 2;
          continue;
        }
      }
    }

    uint32_t word = data[pos];
    if ((word & 0xFFFFFF80) == 0) {
      // will generate one UTF-8 bytes
      *utf8_output++ = char(word);
      pos++;
    } else if ((word & 0xFFFFF800) == 0) {
      // will generate two UTF-8 bytes
      // we have 0b110XXXXX 0b10XXXXXX
      *utf8_output++ = char((word >> 6) | 0b11000000);
      *utf8_output++ = char((word & 0b111111) | 0b10000000);
      pos++;
    } else if ((word & 0xFFFF0000) == 0) {
      // will generate three UTF-8 bytes
      // we have 0b1110XXXX 0b10XXXXXX 0b10XXXXXX
      if (word >= 0xD800 && word <= 0xDFFF) {
        return result(error_code::SURROGATE, pos);
      }
      *utf8_output++ = char((word >> 12) | 0b11100000);
      *utf8_output++ = char(((word >> 6) & 0b111111) | 0b10000000);
      *utf8_output++ = char((word & 0b111111) | 0b10000000);
      pos++;
    } else {
      // will generate four UTF-8 bytes
      // we have 0b11110XXX 0b10XXXXXX 0b10XXXXXX 0b10XXXXXX
      if (word > 0x10FFFF) {
        return result(error_code::TOO_LARGE, pos);
      }
      *utf8_output++ = char((word >> 18) | 0b11110000);
      *utf8_output++ = char(((word >> 12) & 0b111111) | 0b10000000);
      *utf8_output++ = char(((word >> 6) & 0b111111) | 0b10000000);
      *utf8_output++ = char((word & 0b111111) | 0b10000000);
      pos++;
    }
  }
  return result(error_code::SUCCESS, utf8_output - start);
}

} // namespace utf32_to_utf8
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif
/* end file include/simdutf/scalar/utf32_to_utf8/utf32_to_utf8.h */
/* begin file include/simdutf/scalar/utf32_to_utf8/valid_utf32_to_utf8.h */
#ifndef SIMDUTF_VALID_UTF32_TO_UTF8_H
#define SIMDUTF_VALID_UTF32_TO_UTF8_H

#include <cstring>

namespace simdutf {
namespace scalar {
namespace {
namespace utf32_to_utf8 {

template <typename InputPtr, typename OutputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires(simdutf::detail::indexes_into_utf32<InputPtr> &&
           simdutf::detail::index_assignable_from_char<OutputPtr>)
#endif
simdutf_constexpr23 size_t convert_valid(InputPtr data, size_t len,
                                         OutputPtr utf8_output) {
  size_t pos = 0;
  auto start = utf8_output;
  while (pos < len) {
#if SIMDUTF_CPLUSPLUS23
    if !consteval
#endif
    { // try to convert the next block of 2 ASCII characters
      if (pos + 2 <= len) { // if it is safe to read 8 more bytes, check that
                            // they are ascii
        uint64_t v;
        ::memcpy(&v, data + pos, sizeof(uint64_t));
        if ((v & 0xFFFFFF80FFFFFF80) == 0) {
          *utf8_output++ = char(data[pos]);
          *utf8_output++ = char(data[pos + 1]);
          pos += 2;
          continue;
        }
      }
    }

    uint32_t word = data[pos];
    if ((word & 0xFFFFFF80) == 0) {
      // will generate one UTF-8 bytes
      *utf8_output++ = char(word);
      pos++;
    } else if ((word & 0xFFFFF800) == 0) {
      // will generate two UTF-8 bytes
      // we have 0b110XXXXX 0b10XXXXXX
      *utf8_output++ = char((word >> 6) | 0b11000000);
      *utf8_output++ = char((word & 0b111111) | 0b10000000);
      pos++;
    } else if ((word & 0xFFFF0000) == 0) {
      // will generate three UTF-8 bytes
      // we have 0b1110XXXX 0b10XXXXXX 0b10XXXXXX
      *utf8_output++ = char((word >> 12) | 0b11100000);
      *utf8_output++ = char(((word >> 6) & 0b111111) | 0b10000000);
      *utf8_output++ = char((word & 0b111111) | 0b10000000);
      pos++;
    } else {
      // will generate four UTF-8 bytes
      // we have 0b11110XXX 0b10XXXXXX 0b10XXXXXX 0b10XXXXXX
      *utf8_output++ = char((word >> 18) | 0b11110000);
      *utf8_output++ = char(((word >> 12) & 0b111111) | 0b10000000);
      *utf8_output++ = char(((word >> 6) & 0b111111) | 0b10000000);
      *utf8_output++ = char((word & 0b111111) | 0b10000000);
      pos++;
    }
  }
  return utf8_output - start;
}

} // namespace utf32_to_utf8
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif
/* end file include/simdutf/scalar/utf32_to_utf8/valid_utf32_to_utf8.h */
/* begin file include/simdutf/scalar/utf8.h */
#ifndef SIMDUTF_UTF8_H
#define SIMDUTF_UTF8_H

#include <cstring>

namespace simdutf {
namespace scalar {
namespace {
namespace utf8 {

// credit: based on code from Google Fuchsia (Apache Licensed)
template <class BytePtr>
simdutf_constexpr23 simdutf_warn_unused bool validate(BytePtr data,
                                                      size_t len) noexcept {
  static_assert(
      std::is_same<typename std::decay<decltype(*data)>::type, uint8_t>::value,
      "dereferencing the data pointer must result in a uint8_t");
  uint64_t pos = 0;
  uint32_t code_point = 0;
  while (pos < len) {
    uint64_t next_pos;
#if SIMDUTF_CPLUSPLUS23
    if !consteval
#endif
    { // check if the next 16 bytes are ascii.
      next_pos = pos + 16;
      if (next_pos <= len) { // if it is safe to read 16 more bytes, check
                             // that they are ascii
        uint64_t v1{};
        std::memcpy(&v1, data + pos, sizeof(uint64_t));
        uint64_t v2{};
        std::memcpy(&v2, data + pos + sizeof(uint64_t), sizeof(uint64_t));
        uint64_t v{v1 | v2};
        if ((v & 0x8080808080808080) == 0) {
          pos = next_pos;
          continue;
        }
      }
    }

    unsigned char byte = data[pos];

    while (byte < 0b10000000) {
      if (++pos == len) {
        return true;
      }
      byte = data[pos];
    }

    if ((byte & 0b11100000) == 0b11000000) {
      next_pos = pos + 2;
      if (next_pos > len) {
        return false;
      }
      if ((data[pos + 1] & 0b11000000) != 0b10000000) {
        return false;
      }
      // range check
      code_point = (byte & 0b00011111) << 6 | (data[pos + 1] & 0b00111111);
      if (code_point < 0x80) {
        return false;
      }
    } else if ((byte & 0b11110000) == 0b11100000) {
      next_pos = pos + 3;
      if (next_pos > len) {
        return false;
      }
      if ((data[pos + 1] & 0b11000000) != 0b10000000) {
        return false;
      }
      if ((data[pos + 2] & 0b11000000) != 0b10000000) {
        return false;
      }
      // range check
      code_point = (byte & 0b00001111) << 12 |
                   (data[pos + 1] & 0b00111111) << 6 |
                   (data[pos + 2] & 0b00111111);
      if ((code_point < 0x800) ||
          (0xd7ff < code_point && code_point < 0xe000)) {
        return false;
      }
    } else if ((byte & 0b11111000) == 0b11110000) { // 0b11110000
      next_pos = pos + 4;
      if (next_pos > len) {
        return false;
      }
      if ((data[pos + 1] & 0b11000000) != 0b10000000) {
        return false;
      }
      if ((data[pos + 2] & 0b11000000) != 0b10000000) {
        return false;
      }
      if ((data[pos + 3] & 0b11000000) != 0b10000000) {
        return false;
      }
      // range check
      code_point =
          (byte & 0b00000111) << 18 | (data[pos + 1] & 0b00111111) << 12 |
          (data[pos + 2] & 0b00111111) << 6 | (data[pos + 3] & 0b00111111);
      if (code_point <= 0xffff || 0x10ffff < code_point) {
        return false;
      }
    } else {
      // we may have a continuation
      return false;
    }
    pos = next_pos;
  }
  return true;
}

simdutf_really_inline simdutf_warn_unused bool validate(const char *buf,
                                                        size_t len) noexcept {
  return validate(reinterpret_cast<const uint8_t *>(buf), len);
}

template <class BytePtr>
simdutf_constexpr23 simdutf_warn_unused result
validate_with_errors(BytePtr data, size_t len) noexcept {
  static_assert(
      std::is_same<typename std::decay<decltype(*data)>::type, uint8_t>::value,
      "dereferencing the data pointer must result in a uint8_t");
  size_t pos = 0;
  uint32_t code_point = 0;
  while (pos < len) {
    // check of the next 16 bytes are ascii.
    size_t next_pos = pos + 16;
    if (next_pos <=
        len) { // if it is safe to read 16 more bytes, check that they are ascii
      uint64_t v1;
      std::memcpy(&v1, data + pos, sizeof(uint64_t));
      uint64_t v2;
      std::memcpy(&v2, data + pos + sizeof(uint64_t), sizeof(uint64_t));
      uint64_t v{v1 | v2};
      if ((v & 0x8080808080808080) == 0) {
        pos = next_pos;
        continue;
      }
    }
    unsigned char byte = data[pos];

    while (byte < 0b10000000) {
      if (++pos == len) {
        return result(error_code::SUCCESS, len);
      }
      byte = data[pos];
    }

    if ((byte & 0b11100000) == 0b11000000) {
      next_pos = pos + 2;
      if (next_pos > len) {
        return result(error_code::TOO_SHORT, pos);
      }
      if ((data[pos + 1] & 0b11000000) != 0b10000000) {
        return result(error_code::TOO_SHORT, pos);
      }
      // range check
      code_point = (byte & 0b00011111) << 6 | (data[pos + 1] & 0b00111111);
      if (code_point < 0x80) {
        return result(error_code::OVERLONG, pos);
      }
    } else if ((byte & 0b11110000) == 0b11100000) {
      next_pos = pos + 3;
      if (next_pos > len) {
        return result(error_code::TOO_SHORT, pos);
      }
      if ((data[pos + 1] & 0b11000000) != 0b10000000) {
        return result(error_code::TOO_SHORT, pos);
      }
      if ((data[pos + 2] & 0b11000000) != 0b10000000) {
        return result(error_code::TOO_SHORT, pos);
      }
      // range check
      code_point = (byte & 0b00001111) << 12 |
                   (data[pos + 1] & 0b00111111) << 6 |
                   (data[pos + 2] & 0b00111111);
      if (code_point < 0x800) {
        return result(error_code::OVERLONG, pos);
      }
      if (0xd7ff < code_point && code_point < 0xe000) {
        return result(error_code::SURROGATE, pos);
      }
    } else if ((byte & 0b11111000) == 0b11110000) { // 0b11110000
      next_pos = pos + 4;
      if (next_pos > len) {
        return result(error_code::TOO_SHORT, pos);
      }
      if ((data[pos + 1] & 0b11000000) != 0b10000000) {
        return result(error_code::TOO_SHORT, pos);
      }
      if ((data[pos + 2] & 0b11000000) != 0b10000000) {
        return result(error_code::TOO_SHORT, pos);
      }
      if ((data[pos + 3] & 0b11000000) != 0b10000000) {
        return result(error_code::TOO_SHORT, pos);
      }
      // range check
      code_point =
          (byte & 0b00000111) << 18 | (data[pos + 1] & 0b00111111) << 12 |
          (data[pos + 2] & 0b00111111) << 6 | (data[pos + 3] & 0b00111111);
      if (code_point <= 0xffff) {
        return result(error_code::OVERLONG, pos);
      }
      if (0x10ffff < code_point) {
        return result(error_code::TOO_LARGE, pos);
      }
    } else {
      // we either have too many continuation bytes or an invalid leading byte
      if ((byte & 0b11000000) == 0b10000000) {
        return result(error_code::TOO_LONG, pos);
      } else {
        return result(error_code::HEADER_BITS, pos);
      }
    }
    pos = next_pos;
  }
  return result(error_code::SUCCESS, len);
}

simdutf_really_inline simdutf_warn_unused result
validate_with_errors(const char *buf, size_t len) noexcept {
  return validate_with_errors(reinterpret_cast<const uint8_t *>(buf), len);
}

// Finds the previous leading byte starting backward from buf and validates with
// errors from there Used to pinpoint the location of an error when an invalid
// chunk is detected We assume that the stream starts with a leading byte, and
// to check that it is the case, we ask that you pass a pointer to the start of
// the stream (start). Note that the resulting count is underflowed if an error
// is encountered in the rewinded segment.
inline simdutf_warn_unused result rewind_and_validate_with_errors(
    const char *start, const char *buf, size_t len) noexcept {
  // First check that we start with a leading byte
  if ((*start & 0b11000000) == 0b10000000) {
    return result(error_code::TOO_LONG, 0);
  }
  size_t extra_len{0};
  // A leading byte cannot be further than 4 bytes away
  for (int i = 0; i < 5; i++) {
    unsigned char byte = *buf;
    if ((byte & 0b11000000) != 0b10000000) {
      break;
    } else {
      buf--;
      extra_len++;
    }
  }

  result res = validate_with_errors(buf, len + extra_len);
  res.count -= extra_len; // Might underflow
  return res;
}

template <typename InputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires simdutf::detail::indexes_into_byte_like<InputPtr>
#endif
simdutf_constexpr23 size_t count_code_points(InputPtr data, size_t len) {
  size_t counter{0};
  for (size_t i = 0; i < len; i++) {
    // -65 is 0b10111111, anything larger in two-complement's should start a new
    // code point.
    if (int8_t(data[i]) > -65) {
      counter++;
    }
  }
  return counter;
}

template <typename InputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires simdutf::detail::indexes_into_byte_like<InputPtr>
#endif
simdutf_constexpr23 size_t utf16_length_from_utf8(InputPtr data, size_t len) {
  size_t counter{0};
  for (size_t i = 0; i < len; i++) {
    if (int8_t(data[i]) > -65) {
      counter++;
    }
    if (uint8_t(data[i]) >= 240) {
      counter++;
    }
  }
  return counter;
}

template <typename InputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires simdutf::detail::indexes_into_byte_like<InputPtr>
#endif
simdutf_warn_unused simdutf_constexpr23 size_t
trim_partial_utf8(InputPtr input, size_t length) {
  if (length < 3) {
    switch (length) {
    case 2:
      if (uint8_t(input[length - 1]) >= 0xc0) {
        return length - 1;
      } // 2-, 3- and 4-byte characters with only 1 byte left
      if (uint8_t(input[length - 2]) >= 0xe0) {
        return length - 2;
      } // 3- and 4-byte characters with only 2 bytes left
      return length;
    case 1:
      if (uint8_t(input[length - 1]) >= 0xc0) {
        return length - 1;
      } // 2-, 3- and 4-byte characters with only 1 byte left
      return length;
    case 0:
      return length;
    }
  }
  if (uint8_t(input[length - 1]) >= 0xc0) {
    return length - 1;
  } // 2-, 3- and 4-byte characters with only 1 byte left
  if (uint8_t(input[length - 2]) >= 0xe0) {
    return length - 2;
  } // 3- and 4-byte characters with only 1 byte left
  if (uint8_t(input[length - 3]) >= 0xf0) {
    return length - 3;
  } // 4-byte characters with only 3 bytes left
  return length;
}

} // namespace utf8
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif
/* end file include/simdutf/scalar/utf8.h */
/* begin file include/simdutf/scalar/utf8_to_latin1/utf8_to_latin1.h */
#ifndef SIMDUTF_UTF8_TO_LATIN1_H
#define SIMDUTF_UTF8_TO_LATIN1_H

#include <cstring>

namespace simdutf {
namespace scalar {
namespace {
namespace utf8_to_latin1 {

template <typename InputPtr, typename OutputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires(simdutf::detail::indexes_into_byte_like<InputPtr> &&
           simdutf::detail::indexes_into_byte_like<OutputPtr>)
#endif
simdutf_constexpr23 size_t convert(InputPtr data, size_t len,
                                   OutputPtr latin_output) {
  size_t pos = 0;
  auto start = latin_output;

  while (pos < len) {
#if SIMDUTF_CPLUSPLUS23
    if !consteval
#endif
    {
      // try to convert the next block of 16 ASCII bytes
      if (pos + 16 <= len) { // if it is safe to read 16 more bytes, check that
                             // they are ascii
        uint64_t v1;
        ::memcpy(&v1, data + pos, sizeof(uint64_t));
        uint64_t v2;
        ::memcpy(&v2, data + pos + sizeof(uint64_t), sizeof(uint64_t));
        uint64_t v{v1 | v2}; // We are only interested in these bits: 1000 1000
                             // 1000 1000 .... etc
        if ((v & 0x8080808080808080) ==
            0) { // if NONE of these are set, e.g. all of them are zero, then
                 // everything is ASCII
          size_t final_pos = pos + 16;
          while (pos < final_pos) {
            *latin_output++ = char(data[pos]);
            pos++;
          }
          continue;
        }
      }
    }

    // suppose it is not an all ASCII byte sequence
    uint8_t leading_byte = data[pos]; // leading byte
    if (leading_byte < 0b10000000) {
      // converting one ASCII byte !!!
      *latin_output++ = char(leading_byte);
      pos++;
    } else if ((leading_byte & 0b11100000) ==
               0b11000000) { // the first three bits indicate:
      // We have a two-byte UTF-8
      if (pos + 1 >= len) {
        return 0;
      } // minimal bound checking
      if ((data[pos + 1] & 0b11000000) != 0b10000000) {
        return 0;
      } // checks if the next byte is a valid continuation byte in UTF-8. A
        // valid continuation byte starts with 10.
      // range check -
      uint32_t code_point =
          (leading_byte & 0b00011111) << 6 |
          (data[pos + 1] &
           0b00111111); // assembles the Unicode code point from the two bytes.
                        // It does this by discarding the leading 110 and 10
                        // bits from the two bytes, shifting the remaining bits
                        // of the first byte, and then combining the results
                        // with a bitwise OR operation.
      if (code_point < 0x80 || 0xFF < code_point) {
        return 0; // We only care about the range 129-255 which is Non-ASCII
                  // latin1 characters. A code_point beneath 0x80 is invalid as
                  // it is already covered by bytes whose leading bit is zero.
      }
      *latin_output++ = char(code_point);
      pos += 2;
    } else {
      return 0;
    }
  }
  return latin_output - start;
}

template <typename InputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires simdutf::detail::indexes_into_byte_like<InputPtr>
#endif
simdutf_constexpr23 result convert_with_errors(InputPtr data, size_t len,
                                               char *latin_output) {
  size_t pos = 0;
  char *start{latin_output};

  while (pos < len) {
#if SIMDUTF_CPLUSPLUS23
    if !consteval
#endif
    {
      // try to convert the next block of 16 ASCII bytes
      if (pos + 16 <= len) { // if it is safe to read 16 more bytes, check that
                             // they are ascii
        uint64_t v1;
        ::memcpy(&v1, data + pos, sizeof(uint64_t));
        uint64_t v2;
        ::memcpy(&v2, data + pos + sizeof(uint64_t), sizeof(uint64_t));
        uint64_t v{v1 | v2}; // We are only interested in these bits: 1000 1000
                             // 1000 1000...etc
        if ((v & 0x8080808080808080) ==
            0) { // if NONE of these are set, e.g. all of them are zero, then
                 // everything is ASCII
          size_t final_pos = pos + 16;
          while (pos < final_pos) {
            *latin_output++ = char(data[pos]);
            pos++;
          }
          continue;
        }
      }
    }
    // suppose it is not an all ASCII byte sequence
    uint8_t leading_byte = data[pos]; // leading byte
    if (leading_byte < 0b10000000) {
      // converting one ASCII byte !!!
      *latin_output++ = char(leading_byte);
      pos++;
    } else if ((leading_byte & 0b11100000) ==
               0b11000000) { // the first three bits indicate:
      // We have a two-byte UTF-8
      if (pos + 1 >= len) {
        return result(error_code::TOO_SHORT, pos);
      } // minimal bound checking
      if ((data[pos + 1] & 0b11000000) != 0b10000000) {
        return result(error_code::TOO_SHORT, pos);
      } // checks if the next byte is a valid continuation byte in UTF-8. A
        // valid continuation byte starts with 10.
      // range check -
      uint32_t code_point =
          (leading_byte & 0b00011111) << 6 |
          (data[pos + 1] &
           0b00111111); // assembles the Unicode code point from the two bytes.
                        // It does this by discarding the leading 110 and 10
                        // bits from the two bytes, shifting the remaining bits
                        // of the first byte, and then combining the results
                        // with a bitwise OR operation.
      if (code_point < 0x80) {
        return result(error_code::OVERLONG, pos);
      }
      if (0xFF < code_point) {
        return result(error_code::TOO_LARGE, pos);
      } // We only care about the range 129-255 which is Non-ASCII latin1
        // characters
      *latin_output++ = char(code_point);
      pos += 2;
    } else if ((leading_byte & 0b11110000) == 0b11100000) {
      // We have a three-byte UTF-8
      return result(error_code::TOO_LARGE, pos);
    } else if ((leading_byte & 0b11111000) == 0b11110000) { // 0b11110000
      // we have a 4-byte UTF-8 word.
      return result(error_code::TOO_LARGE, pos);
    } else {
      // we either have too many continuation bytes or an invalid leading byte
      if ((leading_byte & 0b11000000) == 0b10000000) {
        return result(error_code::TOO_LONG, pos);
      }

      return result(error_code::HEADER_BITS, pos);
    }
  }
  return result(error_code::SUCCESS, latin_output - start);
}

inline result rewind_and_convert_with_errors(size_t prior_bytes,
                                             const char *buf, size_t len,
                                             char *latin1_output) {
  size_t extra_len{0};
  // We potentially need to go back in time and find a leading byte.
  // In theory '3' would be sufficient, but sometimes the error can go back
  // quite far.
  size_t how_far_back = prior_bytes;
  // size_t how_far_back = 3; // 3 bytes in the past + current position
  // if(how_far_back >= prior_bytes) { how_far_back = prior_bytes; }
  bool found_leading_bytes{false};
  // important: it is i <= how_far_back and not 'i < how_far_back'.
  for (size_t i = 0; i <= how_far_back; i++) {
    unsigned char byte = buf[-static_cast<std::ptrdiff_t>(i)];
    found_leading_bytes = ((byte & 0b11000000) != 0b10000000);
    if (found_leading_bytes) {
      if (i > 0 && byte < 128) {
        // If we had to go back and the leading byte is ascii
        // then we can stop right away.
        return result(error_code::TOO_LONG, 0 - i + 1);
      }
      buf -= i;
      extra_len = i;
      break;
    }
  }
  //
  // It is possible for this function to return a negative count in its result.
  // C++ Standard Section 18.1 defines size_t is in <cstddef> which is described
  // in C Standard as <stddef.h>. C Standard Section 4.1.5 defines size_t as an
  // unsigned integral type of the result of the sizeof operator
  //
  // An unsigned type will simply wrap round arithmetically (well defined).
  //
  if (!found_leading_bytes) {
    // If how_far_back == 3, we may have four consecutive continuation bytes!!!
    // [....] [continuation] [continuation] [continuation] | [buf is
    // continuation] Or we possibly have a stream that does not start with a
    // leading byte.
    return result(error_code::TOO_LONG, 0 - how_far_back);
  }
  result res = convert_with_errors(buf, len + extra_len, latin1_output);
  if (res.error) {
    res.count -= extra_len;
  }
  return res;
}

} // namespace utf8_to_latin1
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif
/* end file include/simdutf/scalar/utf8_to_latin1/utf8_to_latin1.h */
/* begin file include/simdutf/scalar/utf8_to_latin1/valid_utf8_to_latin1.h */
#ifndef SIMDUTF_VALID_UTF8_TO_LATIN1_H
#define SIMDUTF_VALID_UTF8_TO_LATIN1_H

#include <cstring>

namespace simdutf {
namespace scalar {
namespace {
namespace utf8_to_latin1 {

template <typename InputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires simdutf::detail::indexes_into_byte_like<InputPtr>
#endif
simdutf_constexpr23 size_t convert_valid(InputPtr data, size_t len,
                                         char *latin_output) {

  size_t pos = 0;
  char *start{latin_output};

  while (pos < len) {
#if SIMDUTF_CPLUSPLUS23
    if !consteval
#endif
    {
      // try to convert the next block of 16 ASCII bytes
      if (pos + 16 <= len) { // if it is safe to read 16 more bytes, check that
                             // they are ascii
        uint64_t v1;
        ::memcpy(&v1, data + pos, sizeof(uint64_t));
        uint64_t v2;
        ::memcpy(&v2, data + pos + sizeof(uint64_t), sizeof(uint64_t));
        uint64_t v{v1 |
                   v2}; // We are only interested in these bits: 1000 1000 1000
                        // 1000, so it makes sense to concatenate everything
        if ((v & 0x8080808080808080) ==
            0) { // if NONE of these are set, e.g. all of them are zero, then
                 // everything is ASCII
          size_t final_pos = pos + 16;
          while (pos < final_pos) {
            *latin_output++ = uint8_t(data[pos]);
            pos++;
          }
          continue;
        }
      }
    }

    // suppose it is not an all ASCII byte sequence
    auto leading_byte = uint8_t(data[pos]); // leading byte
    if (leading_byte < 0b10000000) {
      // converting one ASCII byte !!!
      *latin_output++ = char(leading_byte);
      pos++;
    } else if ((leading_byte & 0b11100000) ==
               0b11000000) { // the first three bits indicate:
      // We have a two-byte UTF-8
      if (pos + 1 >= len) {
        break;
      } // minimal bound checking
      if ((uint8_t(data[pos + 1]) & 0b11000000) != 0b10000000) {
        return 0;
      } // checks if the next byte is a valid continuation byte in UTF-8. A
        // valid continuation byte starts with 10.
      // range check -
      uint32_t code_point =
          (leading_byte & 0b00011111) << 6 |
          (uint8_t(data[pos + 1]) &
           0b00111111); // assembles the Unicode code point from the two bytes.
                        // It does this by discarding the leading 110 and 10
                        // bits from the two bytes, shifting the remaining bits
                        // of the first byte, and then combining the results
                        // with a bitwise OR operation.
      *latin_output++ = char(code_point);
      pos += 2;
    } else {
      // we may have a continuation but we do not do error checking
      return 0;
    }
  }
  return latin_output - start;
}

} // namespace utf8_to_latin1
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif
/* end file include/simdutf/scalar/utf8_to_latin1/valid_utf8_to_latin1.h */
/* begin file include/simdutf/scalar/utf8_to_utf16/utf8_to_utf16.h */
#ifndef SIMDUTF_UTF8_TO_UTF16_H
#define SIMDUTF_UTF8_TO_UTF16_H

#include <cstring>

namespace simdutf {
namespace scalar {
namespace {
namespace utf8_to_utf16 {

template <endianness big_endian, typename InputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires simdutf::detail::indexes_into_byte_like<InputPtr>
#endif
simdutf_constexpr23 size_t convert(InputPtr data, size_t len,
                                   char16_t *utf16_output) {
  size_t pos = 0;
  char16_t *start{utf16_output};
  while (pos < len) {
#if SIMDUTF_CPLUSPLUS23
    if !consteval
#endif
    // try to convert the next block of 16 ASCII bytes
    {
      if (pos + 16 <= len) { // if it is safe to read 16 more bytes, check that
                             // they are ascii
        uint64_t v1;
        ::memcpy(&v1, data + pos, sizeof(uint64_t));
        uint64_t v2;
        ::memcpy(&v2, data + pos + sizeof(uint64_t), sizeof(uint64_t));
        uint64_t v{v1 | v2};
        if ((v & 0x8080808080808080) == 0) {
          size_t final_pos = pos + 16;
          while (pos < final_pos) {
            *utf16_output++ = !match_system(big_endian)
                                  ? char16_t(u16_swap_bytes(data[pos]))
                                  : char16_t(data[pos]);
            pos++;
          }
          continue;
        }
      }
    }

    uint8_t leading_byte = data[pos]; // leading byte
    if (leading_byte < 0b10000000) {
      // converting one ASCII byte !!!
      *utf16_output++ = !match_system(big_endian)
                            ? char16_t(u16_swap_bytes(leading_byte))
                            : char16_t(leading_byte);
      pos++;
    } else if ((leading_byte & 0b11100000) == 0b11000000) {
      // We have a two-byte UTF-8, it should become
      // a single UTF-16 word.
      if (pos + 1 >= len) {
        return 0;
      } // minimal bound checking
      if ((data[pos + 1] & 0b11000000) != 0b10000000) {
        return 0;
      }
      // range check
      uint32_t code_point =
          (leading_byte & 0b00011111) << 6 | (data[pos + 1] & 0b00111111);
      if (code_point < 0x80) {
        return 0;
      }
      if constexpr (!match_system(big_endian)) {
        code_point = uint32_t(u16_swap_bytes(uint16_t(code_point)));
      }
      *utf16_output++ = char16_t(code_point);
      pos += 2;
    } else if ((leading_byte & 0b11110000) == 0b11100000) {
      // We have a three-byte UTF-8, it should become
      // a single UTF-16 word.
      if (pos + 2 >= len) {
        return 0;
      } // minimal bound checking

      if ((data[pos + 1] & 0b11000000) != 0b10000000) {
        return 0;
      }
      if ((data[pos + 2] & 0b11000000) != 0b10000000) {
        return 0;
      }
      // range check
      uint32_t code_point = (leading_byte & 0b00001111) << 12 |
                            (data[pos + 1] & 0b00111111) << 6 |
                            (data[pos + 2] & 0b00111111);
      if (code_point < 0x800 || (0xd7ff < code_point && code_point < 0xe000)) {
        return 0;
      }
      if constexpr (!match_system(big_endian)) {
        code_point = uint32_t(u16_swap_bytes(uint16_t(code_point)));
      }
      *utf16_output++ = char16_t(code_point);
      pos += 3;
    } else if ((leading_byte & 0b11111000) == 0b11110000) { // 0b11110000
      // we have a 4-byte UTF-8 word.
      if (pos + 3 >= len) {
        return 0;
      } // minimal bound checking
      if ((data[pos + 1] & 0b11000000) != 0b10000000) {
        return 0;
      }
      if ((data[pos + 2] & 0b11000000) != 0b10000000) {
        return 0;
      }
      if ((data[pos + 3] & 0b11000000) != 0b10000000) {
        return 0;
      }

      // range check
      uint32_t code_point = (leading_byte & 0b00000111) << 18 |
                            (data[pos + 1] & 0b00111111) << 12 |
                            (data[pos + 2] & 0b00111111) << 6 |
                            (data[pos + 3] & 0b00111111);
      if (code_point <= 0xffff || 0x10ffff < code_point) {
        return 0;
      }
      code_point -= 0x10000;
      uint16_t high_surrogate = uint16_t(0xD800 + (code_point >> 10));
      uint16_t low_surrogate = uint16_t(0xDC00 + (code_point & 0x3FF));
      if constexpr (!match_system(big_endian)) {
        high_surrogate = u16_swap_bytes(high_surrogate);
        low_surrogate = u16_swap_bytes(low_surrogate);
      }
      *utf16_output++ = char16_t(high_surrogate);
      *utf16_output++ = char16_t(low_surrogate);
      pos += 4;
    } else {
      return 0;
    }
  }
  return utf16_output - start;
}

template <endianness big_endian, typename InputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires simdutf::detail::indexes_into_byte_like<InputPtr>
#endif
simdutf_constexpr23 result convert_with_errors(InputPtr data, size_t len,
                                               char16_t *utf16_output) {
  size_t pos = 0;
  char16_t *start{utf16_output};
  while (pos < len) {
#if SIMDUTF_CPLUSPLUS23
    if !consteval
#endif
    {
      // try to convert the next block of 16 ASCII bytes
      if (pos + 16 <= len) { // if it is safe to read 16 more bytes, check that
                             // they are ascii
        uint64_t v1;
        ::memcpy(&v1, data + pos, sizeof(uint64_t));
        uint64_t v2;
        ::memcpy(&v2, data + pos + sizeof(uint64_t), sizeof(uint64_t));
        uint64_t v{v1 | v2};
        if ((v & 0x8080808080808080) == 0) {
          size_t final_pos = pos + 16;
          while (pos < final_pos) {
            const char16_t byte = uint8_t(data[pos]);
            *utf16_output++ =
                !match_system(big_endian) ? u16_swap_bytes(byte) : byte;
            pos++;
          }
          continue;
        }
      }
    }

    auto leading_byte = uint8_t(data[pos]); // leading byte
    if (leading_byte < 0b10000000) {
      // converting one ASCII byte !!!
      *utf16_output++ = !match_system(big_endian)
                            ? char16_t(u16_swap_bytes(leading_byte))
                            : char16_t(leading_byte);
      pos++;
    } else if ((leading_byte & 0b11100000) == 0b11000000) {
      // We have a two-byte UTF-8, it should become
      // a single UTF-16 word.
      if (pos + 1 >= len) {
        return result(error_code::TOO_SHORT, pos);
      } // minimal bound checking
      if ((uint8_t(data[pos + 1]) & 0b11000000) != 0b10000000) {
        return result(error_code::TOO_SHORT, pos);
      }
      // range check
      uint32_t code_point = (leading_byte & 0b00011111) << 6 |
                            (uint8_t(data[pos + 1]) & 0b00111111);
      if (code_point < 0x80) {
        return result(error_code::OVERLONG, pos);
      }
      if constexpr (!match_system(big_endian)) {
        code_point = uint32_t(u16_swap_bytes(uint16_t(code_point)));
      }
      *utf16_output++ = char16_t(code_point);
      pos += 2;
    } else if ((leading_byte & 0b11110000) == 0b11100000) {
      // We have a three-byte UTF-8, it should become
      // a single UTF-16 word.
      if (pos + 2 >= len) {
        return result(error_code::TOO_SHORT, pos);
      } // minimal bound checking

      if ((uint8_t(data[pos + 1]) & 0b11000000) != 0b10000000) {
        return result(error_code::TOO_SHORT, pos);
      }
      if ((uint8_t(data[pos + 2]) & 0b11000000) != 0b10000000) {
        return result(error_code::TOO_SHORT, pos);
      }
      // range check
      uint32_t code_point = (leading_byte & 0b00001111) << 12 |
                            (uint8_t(data[pos + 1]) & 0b00111111) << 6 |
                            (uint8_t(data[pos + 2]) & 0b00111111);
      if (code_point < 0x800) {
        return result(error_code::OVERLONG, pos);
      }
      if (0xd7ff < code_point && code_point < 0xe000) {
        return result(error_code::SURROGATE, pos);
      }
      if constexpr (!match_system(big_endian)) {
        code_point = uint32_t(u16_swap_bytes(uint16_t(code_point)));
      }
      *utf16_output++ = char16_t(code_point);
      pos += 3;
    } else if ((leading_byte & 0b11111000) == 0b11110000) { // 0b11110000
      // we have a 4-byte UTF-8 word.
      if (pos + 3 >= len) {
        return result(error_code::TOO_SHORT, pos);
      } // minimal bound checking
      if ((uint8_t(data[pos + 1]) & 0b11000000) != 0b10000000) {
        return result(error_code::TOO_SHORT, pos);
      }
      if ((uint8_t(data[pos + 2]) & 0b11000000) != 0b10000000) {
        return result(error_code::TOO_SHORT, pos);
      }
      if ((uint8_t(data[pos + 3]) & 0b11000000) != 0b10000000) {
        return result(error_code::TOO_SHORT, pos);
      }

      // range check
      uint32_t code_point = (leading_byte & 0b00000111) << 18 |
                            (uint8_t(data[pos + 1]) & 0b00111111) << 12 |
                            (uint8_t(data[pos + 2]) & 0b00111111) << 6 |
                            (uint8_t(data[pos + 3]) & 0b00111111);
      if (code_point <= 0xffff) {
        return result(error_code::OVERLONG, pos);
      }
      if (0x10ffff < code_point) {
        return result(error_code::TOO_LARGE, pos);
      }
      code_point -= 0x10000;
      uint16_t high_surrogate = uint16_t(0xD800 + (code_point >> 10));
      uint16_t low_surrogate = uint16_t(0xDC00 + (code_point & 0x3FF));
      if constexpr (!match_system(big_endian)) {
        high_surrogate = u16_swap_bytes(high_surrogate);
        low_surrogate = u16_swap_bytes(low_surrogate);
      }
      *utf16_output++ = char16_t(high_surrogate);
      *utf16_output++ = char16_t(low_surrogate);
      pos += 4;
    } else {
      // we either have too many continuation bytes or an invalid leading byte
      if ((leading_byte & 0b11000000) == 0b10000000) {
        return result(error_code::TOO_LONG, pos);
      } else {
        return result(error_code::HEADER_BITS, pos);
      }
    }
  }
  return result(error_code::SUCCESS, utf16_output - start);
}

/**
 * When rewind_and_convert_with_errors is called, we are pointing at 'buf' and
 * we have up to len input bytes left, and we encountered some error. It is
 * possible that the error is at 'buf' exactly, but it could also be in the
 * previous bytes  (up to 3 bytes back).
 *
 * prior_bytes indicates how many bytes, prior to 'buf' may belong to the
 * current memory section and can be safely accessed. We prior_bytes to access
 * safely up to three bytes before 'buf'.
 *
 * The caller is responsible to ensure that len > 0.
 *
 * If the error is believed to have occurred prior to 'buf', the count value
 * contain in the result will be SIZE_T - 1, SIZE_T - 2, or SIZE_T - 3.
 */
template <endianness endian>
inline result rewind_and_convert_with_errors(size_t prior_bytes,
                                             const char *buf, size_t len,
                                             char16_t *utf16_output) {
  size_t extra_len{0};
  // We potentially need to go back in time and find a leading byte.
  // In theory '3' would be sufficient, but sometimes the error can go back
  // quite far.
  size_t how_far_back = prior_bytes;
  // size_t how_far_back = 3; // 3 bytes in the past + current position
  // if(how_far_back >= prior_bytes) { how_far_back = prior_bytes; }
  bool found_leading_bytes{false};
  // important: it is i <= how_far_back and not 'i < how_far_back'.
  for (size_t i = 0; i <= how_far_back; i++) {
    unsigned char byte = buf[-static_cast<std::ptrdiff_t>(i)];
    found_leading_bytes = ((byte & 0b11000000) != 0b10000000);
    if (found_leading_bytes) {
      if (i > 0 && byte < 128) {
        // If we had to go back and the leading byte is ascii
        // then we can stop right away.
        return result(error_code::TOO_LONG, 0 - i + 1);
      }
      buf -= i;
      extra_len = i;
      break;
    }
  }
  //
  // It is possible for this function to return a negative count in its result.
  // C++ Standard Section 18.1 defines size_t is in <cstddef> which is described
  // in C Standard as <stddef.h>. C Standard Section 4.1.5 defines size_t as an
  // unsigned integral type of the result of the sizeof operator
  //
  // An unsigned type will simply wrap round arithmetically (well defined).
  //
  if (!found_leading_bytes) {
    // If how_far_back == 3, we may have four consecutive continuation bytes!!!
    // [....] [continuation] [continuation] [continuation] | [buf is
    // continuation] Or we possibly have a stream that does not start with a
    // leading byte.
    return result(error_code::TOO_LONG, 0 - how_far_back);
  }
  result res = convert_with_errors<endian>(buf, len + extra_len, utf16_output);
  if (res.error) {
    res.count -= extra_len;
  }
  return res;
}

} // namespace utf8_to_utf16
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif
/* end file include/simdutf/scalar/utf8_to_utf16/utf8_to_utf16.h */
/* begin file include/simdutf/scalar/utf8_to_utf16/valid_utf8_to_utf16.h */
#ifndef SIMDUTF_VALID_UTF8_TO_UTF16_H
#define SIMDUTF_VALID_UTF8_TO_UTF16_H

#include <cstring>

namespace simdutf {
namespace scalar {
namespace {
namespace utf8_to_utf16 {

template <endianness big_endian, typename InputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires simdutf::detail::indexes_into_byte_like<InputPtr>
#endif
simdutf_constexpr23 size_t convert_valid(InputPtr data, size_t len,
                                         char16_t *utf16_output) {
  size_t pos = 0;
  char16_t *start{utf16_output};
  while (pos < len) {
#if SIMDUTF_CPLUSPLUS23
    if !consteval
#endif
    {                       // try to convert the next block of 8 ASCII bytes
      if (pos + 8 <= len) { // if it is safe to read 8 more bytes, check that
                            // they are ascii
        uint64_t v;
        ::memcpy(&v, data + pos, sizeof(uint64_t));
        if ((v & 0x8080808080808080) == 0) {
          size_t final_pos = pos + 8;
          while (pos < final_pos) {
            const char16_t byte = uint8_t(data[pos]);
            *utf16_output++ =
                !match_system(big_endian) ? u16_swap_bytes(byte) : byte;
            pos++;
          }
          continue;
        }
      }
    }

    auto leading_byte = uint8_t(data[pos]); // leading byte
    if (leading_byte < 0b10000000) {
      // converting one ASCII byte !!!
      *utf16_output++ = !match_system(big_endian)
                            ? char16_t(u16_swap_bytes(leading_byte))
                            : char16_t(leading_byte);
      pos++;
    } else if ((leading_byte & 0b11100000) == 0b11000000) {
      // We have a two-byte UTF-8, it should become
      // a single UTF-16 word.
      if (pos + 1 >= len) {
        break;
      } // minimal bound checking
      uint16_t code_point = uint16_t(((leading_byte & 0b00011111) << 6) |
                                     (uint8_t(data[pos + 1]) & 0b00111111));
      if constexpr (!match_system(big_endian)) {
        code_point = u16_swap_bytes(uint16_t(code_point));
      }
      *utf16_output++ = char16_t(code_point);
      pos += 2;
    } else if ((leading_byte & 0b11110000) == 0b11100000) {
      // We have a three-byte UTF-8, it should become
      // a single UTF-16 word.
      if (pos + 2 >= len) {
        break;
      } // minimal bound checking
      uint16_t code_point =
          uint16_t(((leading_byte & 0b00001111) << 12) |
                   ((uint8_t(data[pos + 1]) & 0b00111111) << 6) |
                   (uint8_t(data[pos + 2]) & 0b00111111));
      if constexpr (!match_system(big_endian)) {
        code_point = u16_swap_bytes(uint16_t(code_point));
      }
      *utf16_output++ = char16_t(code_point);
      pos += 3;
    } else if ((leading_byte & 0b11111000) == 0b11110000) { // 0b11110000
      // we have a 4-byte UTF-8 word.
      if (pos + 3 >= len) {
        break;
      } // minimal bound checking
      uint32_t code_point = ((leading_byte & 0b00000111) << 18) |
                            ((uint8_t(data[pos + 1]) & 0b00111111) << 12) |
                            ((uint8_t(data[pos + 2]) & 0b00111111) << 6) |
                            (uint8_t(data[pos + 3]) & 0b00111111);
      code_point -= 0x10000;
      uint16_t high_surrogate = uint16_t(0xD800 + (code_point >> 10));
      uint16_t low_surrogate = uint16_t(0xDC00 + (code_point & 0x3FF));
      if constexpr (!match_system(big_endian)) {
        high_surrogate = u16_swap_bytes(high_surrogate);
        low_surrogate = u16_swap_bytes(low_surrogate);
      }
      *utf16_output++ = char16_t(high_surrogate);
      *utf16_output++ = char16_t(low_surrogate);
      pos += 4;
    } else {
      // we may have a continuation but we do not do error checking
      return 0;
    }
  }
  return utf16_output - start;
}

} // namespace utf8_to_utf16
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif
/* end file include/simdutf/scalar/utf8_to_utf16/valid_utf8_to_utf16.h */
/* begin file include/simdutf/scalar/utf8_to_utf32/utf8_to_utf32.h */
#ifndef SIMDUTF_UTF8_TO_UTF32_H
#define SIMDUTF_UTF8_TO_UTF32_H

#include <cstring>

namespace simdutf {
namespace scalar {
namespace {
namespace utf8_to_utf32 {

template <typename InputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires simdutf::detail::indexes_into_byte_like<InputPtr>
#endif
simdutf_constexpr23 size_t convert(InputPtr data, size_t len,
                                   char32_t *utf32_output) {
  size_t pos = 0;
  char32_t *start{utf32_output};
  while (pos < len) {
#if SIMDUTF_CPLUSPLUS23
    if !consteval
#endif
    {
      // try to convert the next block of 16 ASCII bytes
      if (pos + 16 <= len) { // if it is safe to read 16 more bytes, check that
                             // they are ascii
        uint64_t v1;
        ::memcpy(&v1, data + pos, sizeof(uint64_t));
        uint64_t v2;
        ::memcpy(&v2, data + pos + sizeof(uint64_t), sizeof(uint64_t));
        uint64_t v{v1 | v2};
        if ((v & 0x8080808080808080) == 0) {
          size_t final_pos = pos + 16;
          while (pos < final_pos) {
            *utf32_output++ = uint8_t(data[pos]);
            pos++;
          }
          continue;
        }
      }
    }
    auto leading_byte = uint8_t(data[pos]); // leading byte
    if (leading_byte < 0b10000000) {
      // converting one ASCII byte !!!
      *utf32_output++ = char32_t(leading_byte);
      pos++;
    } else if ((leading_byte & 0b11100000) == 0b11000000) {
      // We have a two-byte UTF-8
      if (pos + 1 >= len) {
        return 0;
      } // minimal bound checking
      if ((data[pos + 1] & 0b11000000) != 0b10000000) {
        return 0;
      }
      // range check
      uint32_t code_point = (leading_byte & 0b00011111) << 6 |
                            (uint8_t(data[pos + 1]) & 0b00111111);
      if (code_point < 0x80) {
        return 0;
      }
      *utf32_output++ = char32_t(code_point);
      pos += 2;
    } else if ((leading_byte & 0b11110000) == 0b11100000) {
      // We have a three-byte UTF-8
      if (pos + 2 >= len) {
        return 0;
      } // minimal bound checking

      if ((uint8_t(data[pos + 1]) & 0b11000000) != 0b10000000) {
        return 0;
      }
      if ((uint8_t(data[pos + 2]) & 0b11000000) != 0b10000000) {
        return 0;
      }
      // range check
      uint32_t code_point = (leading_byte & 0b00001111) << 12 |
                            (uint8_t(data[pos + 1]) & 0b00111111) << 6 |
                            (uint8_t(data[pos + 2]) & 0b00111111);
      if (code_point < 0x800 || (0xd7ff < code_point && code_point < 0xe000)) {
        return 0;
      }
      *utf32_output++ = char32_t(code_point);
      pos += 3;
    } else if ((leading_byte & 0b11111000) == 0b11110000) { // 0b11110000
      // we have a 4-byte UTF-8 word.
      if (pos + 3 >= len) {
        return 0;
      } // minimal bound checking
      if ((uint8_t(data[pos + 1]) & 0b11000000) != 0b10000000) {
        return 0;
      }
      if ((uint8_t(data[pos + 2]) & 0b11000000) != 0b10000000) {
        return 0;
      }
      if ((uint8_t(data[pos + 3]) & 0b11000000) != 0b10000000) {
        return 0;
      }

      // range check
      uint32_t code_point = (leading_byte & 0b00000111) << 18 |
                            (uint8_t(data[pos + 1]) & 0b00111111) << 12 |
                            (uint8_t(data[pos + 2]) & 0b00111111) << 6 |
                            (uint8_t(data[pos + 3]) & 0b00111111);
      if (code_point <= 0xffff || 0x10ffff < code_point) {
        return 0;
      }
      *utf32_output++ = char32_t(code_point);
      pos += 4;
    } else {
      return 0;
    }
  }
  return utf32_output - start;
}

template <typename InputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires simdutf::detail::indexes_into_byte_like<InputPtr>
#endif
simdutf_constexpr23 result convert_with_errors(InputPtr data, size_t len,
                                               char32_t *utf32_output) {
  size_t pos = 0;
  char32_t *start{utf32_output};
  while (pos < len) {
#if SIMDUTF_CPLUSPLUS23
    if !consteval
#endif
    {
      // try to convert the next block of 16 ASCII bytes
      if (pos + 16 <= len) { // if it is safe to read 16 more bytes, check that
                             // they are ascii
        uint64_t v1;
        ::memcpy(&v1, data + pos, sizeof(uint64_t));
        uint64_t v2;
        ::memcpy(&v2, data + pos + sizeof(uint64_t), sizeof(uint64_t));
        uint64_t v{v1 | v2};
        if ((v & 0x8080808080808080) == 0) {
          size_t final_pos = pos + 16;
          while (pos < final_pos) {
            *utf32_output++ = uint8_t(data[pos]);
            pos++;
          }
          continue;
        }
      }
    }
    auto leading_byte = uint8_t(data[pos]); // leading byte
    if (leading_byte < 0b10000000) {
      // converting one ASCII byte !!!
      *utf32_output++ = char32_t(leading_byte);
      pos++;
    } else if ((leading_byte & 0b11100000) == 0b11000000) {
      // We have a two-byte UTF-8
      if (pos + 1 >= len) {
        return result(error_code::TOO_SHORT, pos);
      } // minimal bound checking
      if ((uint8_t(data[pos + 1]) & 0b11000000) != 0b10000000) {
        return result(error_code::TOO_SHORT, pos);
      }
      // range check
      uint32_t code_point = (leading_byte & 0b00011111) << 6 |
                            (uint8_t(data[pos + 1]) & 0b00111111);
      if (code_point < 0x80) {
        return result(error_code::OVERLONG, pos);
      }
      *utf32_output++ = char32_t(code_point);
      pos += 2;
    } else if ((leading_byte & 0b11110000) == 0b11100000) {
      // We have a three-byte UTF-8
      if (pos + 2 >= len) {
        return result(error_code::TOO_SHORT, pos);
      } // minimal bound checking

      if ((uint8_t(data[pos + 1]) & 0b11000000) != 0b10000000) {
        return result(error_code::TOO_SHORT, pos);
      }
      if ((uint8_t(data[pos + 2]) & 0b11000000) != 0b10000000) {
        return result(error_code::TOO_SHORT, pos);
      }
      // range check
      uint32_t code_point = (leading_byte & 0b00001111) << 12 |
                            (uint8_t(data[pos + 1]) & 0b00111111) << 6 |
                            (uint8_t(data[pos + 2]) & 0b00111111);
      if (code_point < 0x800) {
        return result(error_code::OVERLONG, pos);
      }
      if (0xd7ff < code_point && code_point < 0xe000) {
        return result(error_code::SURROGATE, pos);
      }
      *utf32_output++ = char32_t(code_point);
      pos += 3;
    } else if ((leading_byte & 0b11111000) == 0b11110000) { // 0b11110000
      // we have a 4-byte UTF-8 word.
      if (pos + 3 >= len) {
        return result(error_code::TOO_SHORT, pos);
      } // minimal bound checking
      if ((uint8_t(data[pos + 1]) & 0b11000000) != 0b10000000) {
        return result(error_code::TOO_SHORT, pos);
      }
      if ((uint8_t(data[pos + 2]) & 0b11000000) != 0b10000000) {
        return result(error_code::TOO_SHORT, pos);
      }
      if ((uint8_t(data[pos + 3]) & 0b11000000) != 0b10000000) {
        return result(error_code::TOO_SHORT, pos);
      }

      // range check
      uint32_t code_point = (leading_byte & 0b00000111) << 18 |
                            (uint8_t(data[pos + 1]) & 0b00111111) << 12 |
                            (uint8_t(data[pos + 2]) & 0b00111111) << 6 |
                            (uint8_t(data[pos + 3]) & 0b00111111);
      if (code_point <= 0xffff) {
        return result(error_code::OVERLONG, pos);
      }
      if (0x10ffff < code_point) {
        return result(error_code::TOO_LARGE, pos);
      }
      *utf32_output++ = char32_t(code_point);
      pos += 4;
    } else {
      // we either have too many continuation bytes or an invalid leading byte
      if ((leading_byte & 0b11000000) == 0b10000000) {
        return result(error_code::TOO_LONG, pos);
      } else {
        return result(error_code::HEADER_BITS, pos);
      }
    }
  }
  return result(error_code::SUCCESS, utf32_output - start);
}

/**
 * When rewind_and_convert_with_errors is called, we are pointing at 'buf' and
 * we have up to len input bytes left, and we encountered some error. It is
 * possible that the error is at 'buf' exactly, but it could also be in the
 * previous bytes location (up to 3 bytes back).
 *
 * prior_bytes indicates how many bytes, prior to 'buf' may belong to the
 * current memory section and can be safely accessed. We prior_bytes to access
 * safely up to three bytes before 'buf'.
 *
 * The caller is responsible to ensure that len > 0.
 *
 * If the error is believed to have occurred prior to 'buf', the count value
 * contain in the result will be SIZE_T - 1, SIZE_T - 2, or SIZE_T - 3.
 */
inline result rewind_and_convert_with_errors(size_t prior_bytes,
                                             const char *buf, size_t len,
                                             char32_t *utf32_output) {
  size_t extra_len{0};
  // We potentially need to go back in time and find a leading byte.
  size_t how_far_back = 3; // 3 bytes in the past + current position
  if (how_far_back > prior_bytes) {
    how_far_back = prior_bytes;
  }
  bool found_leading_bytes{false};
  // important: it is i <= how_far_back and not 'i < how_far_back'.
  for (size_t i = 0; i <= how_far_back; i++) {
    unsigned char byte = buf[-static_cast<std::ptrdiff_t>(i)];
    found_leading_bytes = ((byte & 0b11000000) != 0b10000000);
    if (found_leading_bytes) {
      if (i > 0 && byte < 128) {
        // If we had to go back and the leading byte is ascii
        // then we can stop right away.
        return result(error_code::TOO_LONG, 0 - i + 1);
      }
      buf -= i;
      extra_len = i;
      break;
    }
  }
  //
  // It is possible for this function to return a negative count in its result.
  // C++ Standard Section 18.1 defines size_t is in <cstddef> which is described
  // in C Standard as <stddef.h>. C Standard Section 4.1.5 defines size_t as an
  // unsigned integral type of the result of the sizeof operator
  //
  // An unsigned type will simply wrap round arithmetically (well defined).
  //
  if (!found_leading_bytes) {
    // If how_far_back == 3, we may have four consecutive continuation bytes!!!
    // [....] [continuation] [continuation] [continuation] | [buf is
    // continuation] Or we possibly have a stream that does not start with a
    // leading byte.
    return result(error_code::TOO_LONG, 0 - how_far_back);
  }

  result res = convert_with_errors(buf, len + extra_len, utf32_output);
  if (res.error) {
    res.count -= extra_len;
  }
  return res;
}

} // namespace utf8_to_utf32
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif
/* end file include/simdutf/scalar/utf8_to_utf32/utf8_to_utf32.h */
/* begin file include/simdutf/scalar/utf8_to_utf32/valid_utf8_to_utf32.h */
#ifndef SIMDUTF_VALID_UTF8_TO_UTF32_H
#define SIMDUTF_VALID_UTF8_TO_UTF32_H

#include <cstring>

namespace simdutf {
namespace scalar {
namespace {
namespace utf8_to_utf32 {

template <typename InputPtr>
#if SIMDUTF_CPLUSPLUS20
  requires simdutf::detail::indexes_into_byte_like<InputPtr>
#endif
simdutf_constexpr23 size_t convert_valid(InputPtr data, size_t len,
                                         char32_t *utf32_output) {
  size_t pos = 0;
  char32_t *start{utf32_output};
  while (pos < len) {
#if SIMDUTF_CPLUSPLUS23
    if !consteval
#endif
    {
      // try to convert the next block of 8 ASCII bytes
      if (pos + 8 <= len) { // if it is safe to read 8 more bytes, check that
                            // they are ascii
        uint64_t v;
        ::memcpy(&v, data + pos, sizeof(uint64_t));
        if ((v & 0x8080808080808080) == 0) {
          size_t final_pos = pos + 8;
          while (pos < final_pos) {
            *utf32_output++ = uint8_t(data[pos]);
            pos++;
          }
          continue;
        }
      }
    }
    auto leading_byte = uint8_t(data[pos]); // leading byte
    if (leading_byte < 0b10000000) {
      // converting one ASCII byte !!!
      *utf32_output++ = char32_t(leading_byte);
      pos++;
    } else if ((leading_byte & 0b11100000) == 0b11000000) {
      // We have a two-byte UTF-8
      if (pos + 1 >= len) {
        break;
      } // minimal bound checking
      *utf32_output++ = char32_t(((leading_byte & 0b00011111) << 6) |
                                 (uint8_t(data[pos + 1]) & 0b00111111));
      pos += 2;
    } else if ((leading_byte & 0b11110000) == 0b11100000) {
      // We have a three-byte UTF-8
      if (pos + 2 >= len) {
        break;
      } // minimal bound checking
      *utf32_output++ = char32_t(((leading_byte & 0b00001111) << 12) |
                                 ((uint8_t(data[pos + 1]) & 0b00111111) << 6) |
                                 (uint8_t(data[pos + 2]) & 0b00111111));
      pos += 3;
    } else if ((leading_byte & 0b11111000) == 0b11110000) { // 0b11110000
      // we have a 4-byte UTF-8 word.
      if (pos + 3 >= len) {
        break;
      } // minimal bound checking
      uint32_t code_word = ((leading_byte & 0b00000111) << 18) |
                           ((uint8_t(data[pos + 1]) & 0b00111111) << 12) |
                           ((uint8_t(data[pos + 2]) & 0b00111111) << 6) |
                           (uint8_t(data[pos + 3]) & 0b00111111);
      *utf32_output++ = char32_t(code_word);
      pos += 4;
    } else {
      // we may have a continuation but we do not do error checking
      return 0;
    }
  }
  return utf32_output - start;
}

} // namespace utf8_to_utf32
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif
/* end file include/simdutf/scalar/utf8_to_utf32/valid_utf8_to_utf32.h */

namespace simdutf {

constexpr size_t default_line_length =
    76; ///< default line length for base64 encoding with lines

/**
 * Validate the UTF-8 string. This function may be best when you expect
 * the input to be almost always valid. Otherwise, consider using
 * validate_utf8_with_errors.
 *
 * Overridden by each implementation.
 *
 * @param buf the UTF-8 string to validate.
 * @param len the length of the string in bytes.
 * @return true if and only if the string is valid UTF-8.
 */
simdutf_warn_unused bool validate_utf8(const char *buf, size_t len) noexcept;
  #if SIMDUTF_SPAN
simdutf_constexpr23 simdutf_really_inline simdutf_warn_unused bool
validate_utf8(const detail::input_span_of_byte_like auto &input) noexcept {
    #if SIMDUTF_CPLUSPLUS23
  if consteval {
    return scalar::utf8::validate(
        detail::constexpr_cast_ptr<uint8_t>(input.data()), input.size());
  } else
    #endif
  {
    return validate_utf8(reinterpret_cast<const char *>(input.data()),
                         input.size());
  }
}
  #endif // SIMDUTF_SPAN

/**
 * Validate the UTF-8 string and stop on error.
 *
 * Overridden by each implementation.
 *
 * @param buf the UTF-8 string to validate.
 * @param len the length of the string in bytes.
 * @return a result pair struct (of type simdutf::result containing the two
 * fields error and count) with an error code and either position of the error
 * (in the input in code units) if any, or the number of code units validated if
 * successful.
 */
simdutf_warn_unused result validate_utf8_with_errors(const char *buf,
                                                     size_t len) noexcept;
  #if SIMDUTF_SPAN
simdutf_really_inline simdutf_constexpr23 simdutf_warn_unused result
validate_utf8_with_errors(
    const detail::input_span_of_byte_like auto &input) noexcept {
    #if SIMDUTF_CPLUSPLUS23
  if consteval {
    return scalar::utf8::validate_with_errors(
        detail::constexpr_cast_ptr<uint8_t>(input.data()), input.size());
  } else
    #endif
  {
    return validate_utf8_with_errors(
        reinterpret_cast<const char *>(input.data()), input.size());
  }
}
  #endif // SIMDUTF_SPAN

/**
 * Validate the UTF-32 string. This function may be best when you expect
 * the input to be almost always valid. Otherwise, consider using
 * validate_utf32_with_errors.
 *
 * Overridden by each implementation.
 *
 * This function is not BOM-aware.
 *
 * @param buf the UTF-32 string to validate.
 * @param len the length of the string in number of 4-byte code units
 * (char32_t).
 * @return true if and only if the string is valid UTF-32.
 */
simdutf_warn_unused bool validate_utf32(const char32_t *buf,
                                        size_t len) noexcept;
  #if SIMDUTF_SPAN
simdutf_really_inline simdutf_warn_unused simdutf_constexpr23 bool
validate_utf32(std::span<const char32_t> input) noexcept {
    #if SIMDUTF_CPLUSPLUS23
  if consteval {
    return scalar::utf32::validate(
        detail::constexpr_cast_ptr<std::uint32_t>(input.data()), input.size());
  } else
    #endif
  {
    return validate_utf32(input.data(), input.size());
  }
}
  #endif // SIMDUTF_SPAN

/**
 * Validate the UTF-32 string and stop on error. It might be faster than
 * validate_utf32 when an error is expected to occur early.
 *
 * Overridden by each implementation.
 *
 * This function is not BOM-aware.
 *
 * @param buf the UTF-32 string to validate.
 * @param len the length of the string in number of 4-byte code units
 * (char32_t).
 * @return a result pair struct (of type simdutf::result containing the two
 * fields error and count) with an error code and either position of the error
 * (in the input in code units) if any, or the number of code units validated if
 * successful.
 */
simdutf_warn_unused result validate_utf32_with_errors(const char32_t *buf,
                                                      size_t len) noexcept;
  #if SIMDUTF_SPAN
simdutf_really_inline simdutf_warn_unused simdutf_constexpr23 result
validate_utf32_with_errors(std::span<const char32_t> input) noexcept {
    #if SIMDUTF_CPLUSPLUS23
  if consteval {
    return scalar::utf32::validate_with_errors(
        detail::constexpr_cast_ptr<std::uint32_t>(input.data()), input.size());
  } else
    #endif
  {
    return validate_utf32_with_errors(input.data(), input.size());
  }
}
  #endif // SIMDUTF_SPAN

/**
 * Convert possibly broken UTF-8 string into UTF-32 string.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * @param input         the UTF-8 string to convert
 * @param length        the length of the string in bytes
 * @param utf32_output  the pointer to buffer that can hold conversion result
 * @return the number of written char32_t; 0 if the input was not valid UTF-8
 * string
 */
simdutf_warn_unused size_t convert_utf8_to_utf32(
    const char *input, size_t length, char32_t *utf32_output) noexcept;
  #if SIMDUTF_SPAN
simdutf_really_inline simdutf_warn_unused simdutf_constexpr23 size_t
convert_utf8_to_utf32(const detail::input_span_of_byte_like auto &utf8_input,
                      std::span<char32_t> utf32_output) noexcept {
    #if SIMDUTF_CPLUSPLUS23
  if consteval {
    return scalar::utf8_to_utf32::convert(utf8_input.data(), utf8_input.size(),
                                          utf32_output.data());
  } else
    #endif
  {
    return convert_utf8_to_utf32(
        reinterpret_cast<const char *>(utf8_input.data()), utf8_input.size(),
        utf32_output.data());
  }
}
  #endif // SIMDUTF_SPAN

/**
 * Convert possibly broken UTF-8 string into UTF-32 string and stop on error.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * @param input         the UTF-8 string to convert
 * @param length        the length of the string in bytes
 * @param utf32_output  the pointer to buffer that can hold conversion result
 * @return a result pair struct (of type simdutf::result containing the two
 * fields error and count) with an error code and either position of the error
 * (in the input in code units) if any, or the number of char32_t written if
 * successful.
 */
simdutf_warn_unused result convert_utf8_to_utf32_with_errors(
    const char *input, size_t length, char32_t *utf32_output) noexcept;
  #if SIMDUTF_SPAN
simdutf_really_inline simdutf_warn_unused simdutf_constexpr23 result
convert_utf8_to_utf32_with_errors(
    const detail::input_span_of_byte_like auto &utf8_input,
    std::span<char32_t> utf32_output) noexcept {
    #if SIMDUTF_CPLUSPLUS23
  if consteval {
    return scalar::utf8_to_utf32::convert_with_errors(
        utf8_input.data(), utf8_input.size(), utf32_output.data());
  } else
    #endif
  {
    return convert_utf8_to_utf32_with_errors(
        reinterpret_cast<const char *>(utf8_input.data()), utf8_input.size(),
        utf32_output.data());
  }
}
  #endif // SIMDUTF_SPAN

/**
 * Convert valid UTF-8 string into UTF-32 string.
 *
 * This function assumes that the input string is valid UTF-8.
 *
 * @param input         the UTF-8 string to convert
 * @param length        the length of the string in bytes
 * @param utf32_buffer  the pointer to buffer that can hold conversion result
 * @return the number of written char32_t
 */
simdutf_warn_unused size_t convert_valid_utf8_to_utf32(
    const char *input, size_t length, char32_t *utf32_buffer) noexcept;
  #if SIMDUTF_SPAN
simdutf_really_inline simdutf_warn_unused simdutf_constexpr23 size_t
convert_valid_utf8_to_utf32(
    const detail::input_span_of_byte_like auto &valid_utf8_input,
    std::span<char32_t> utf32_output) noexcept {
    #if SIMDUTF_CPLUSPLUS23
  if consteval {
    return scalar::utf8_to_utf32::convert_valid(
        valid_utf8_input.data(), valid_utf8_input.size(), utf32_output.data());
  } else
    #endif
  {
    return convert_valid_utf8_to_utf32(
        reinterpret_cast<const char *>(valid_utf8_input.data()),
        valid_utf8_input.size(), utf32_output.data());
  }
}
  #endif // SIMDUTF_SPAN

/**
 * Compute the number of 4-byte code units that this UTF-8 string would require
 * in UTF-32 format.
 *
 * This function is equivalent to count_utf8
 *
 * This function does not validate the input. It is acceptable to pass invalid
 * UTF-8 strings but in such cases the result is implementation defined.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-8 string to process
 * @param length        the length of the string in bytes
 * @return the number of char32_t code units required to encode the UTF-8 string
 * as UTF-32
 */
simdutf_warn_unused size_t utf32_length_from_utf8(const char *input,
                                                  size_t length) noexcept;
  #if SIMDUTF_SPAN
simdutf_really_inline simdutf_warn_unused simdutf_constexpr23 size_t
utf32_length_from_utf8(
    const detail::input_span_of_byte_like auto &valid_utf8_input) noexcept {

    #if SIMDUTF_CPLUSPLUS23
  if consteval {
    return scalar::utf8::count_code_points(valid_utf8_input.data(),
                                           valid_utf8_input.size());
  } else
    #endif
  {
    return utf32_length_from_utf8(
        reinterpret_cast<const char *>(valid_utf8_input.data()),
        valid_utf8_input.size());
  }
}
  #endif // SIMDUTF_SPAN

/**
 * Convert possibly broken UTF-32 string into UTF-8 string.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-32 string to convert
 * @param length        the length of the string in 4-byte code units (char32_t)
 * @param utf8_buffer   the pointer to buffer that can hold conversion result
 * @return number of written code units; 0 if input is not a valid UTF-32 string
 */
simdutf_warn_unused size_t convert_utf32_to_utf8(const char32_t *input,
                                                 size_t length,
                                                 char *utf8_buffer) noexcept;
  #if SIMDUTF_SPAN
simdutf_really_inline simdutf_warn_unused simdutf_constexpr23 size_t
convert_utf32_to_utf8(
    std::span<const char32_t> utf32_input,
    detail::output_span_of_byte_like auto &&utf8_output) noexcept {
    #if SIMDUTF_CPLUSPLUS23
  if consteval {
    return scalar::utf32_to_utf8::convert(
        utf32_input.data(), utf32_input.size(), utf8_output.data());
  } else
    #endif
  {
    return convert_utf32_to_utf8(utf32_input.data(), utf32_input.size(),
                                 reinterpret_cast<char *>(utf8_output.data()));
  }
}
  #endif // SIMDUTF_SPAN

/**
 * Convert possibly broken UTF-32 string into UTF-8 string and stop on error.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-32 string to convert
 * @param length        the length of the string in 4-byte code units (char32_t)
 * @param utf8_buffer   the pointer to buffer that can hold conversion result
 * @return a result pair struct (of type simdutf::result containing the two
 * fields error and count) with an error code and either position of the error
 * (in the input in code units) if any, or the number of char written if
 * successful.
 */
simdutf_warn_unused result convert_utf32_to_utf8_with_errors(
    const char32_t *input, size_t length, char *utf8_buffer) noexcept;
  #if SIMDUTF_SPAN
simdutf_really_inline simdutf_warn_unused simdutf_constexpr23 result
convert_utf32_to_utf8_with_errors(
    std::span<const char32_t> utf32_input,
    detail::output_span_of_byte_like auto &&utf8_output) noexcept {
    #if SIMDUTF_CPLUSPLUS23
  if consteval {
    return scalar::utf32_to_utf8::convert_with_errors(
        utf32_input.data(), utf32_input.size(), utf8_output.data());
  } else
    #endif
  {
    return convert_utf32_to_utf8_with_errors(
        utf32_input.data(), utf32_input.size(),
        reinterpret_cast<char *>(utf8_output.data()));
  }
}
  #endif // SIMDUTF_SPAN

/**
 * Convert valid UTF-32 string into UTF-8 string.
 *
 * This function assumes that the input string is valid UTF-32.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-32 string to convert
 * @param length        the length of the string in 4-byte code units (char32_t)
 * @param utf8_buffer   the pointer to a buffer that can hold the conversion
 * result
 * @return number of written code units; 0 if conversion is not possible
 */
simdutf_warn_unused size_t convert_valid_utf32_to_utf8(
    const char32_t *input, size_t length, char *utf8_buffer) noexcept;
  #if SIMDUTF_SPAN
simdutf_really_inline simdutf_warn_unused simdutf_constexpr23 size_t
convert_valid_utf32_to_utf8(
    std::span<const char32_t> valid_utf32_input,
    detail::output_span_of_byte_like auto &&utf8_output) noexcept {
    #if SIMDUTF_CPLUSPLUS23
  if consteval {
    return scalar::utf32_to_utf8::convert_valid(
        valid_utf32_input.data(), valid_utf32_input.size(), utf8_output.data());
  } else
    #endif
  {
    return convert_valid_utf32_to_utf8(
        valid_utf32_input.data(), valid_utf32_input.size(),
        reinterpret_cast<char *>(utf8_output.data()));
  }
}
  #endif // SIMDUTF_SPAN

/**
 * Compute the number of bytes that this UTF-32 string would require in UTF-8
 * format.
 *
 * This function does not validate the input. It is acceptable to pass invalid
 * UTF-32 strings but in such cases the result is implementation defined.
 *
 * @param input         the UTF-32 string to convert
 * @param length        the length of the string in 4-byte code units (char32_t)
 * @return the number of bytes required to encode the UTF-32 string as UTF-8
 */
simdutf_warn_unused size_t utf8_length_from_utf32(const char32_t *input,
                                                  size_t length) noexcept;
  #if SIMDUTF_SPAN
simdutf_really_inline simdutf_warn_unused simdutf_constexpr23 size_t
utf8_length_from_utf32(std::span<const char32_t> valid_utf32_input) noexcept {
    #if SIMDUTF_CPLUSPLUS23
  if consteval {
    return scalar::utf32::utf8_length_from_utf32(valid_utf32_input.data(),
                                                 valid_utf32_input.size());
  } else
    #endif
  {
    return utf8_length_from_utf32(valid_utf32_input.data(),
                                  valid_utf32_input.size());
  }
}
  #endif // SIMDUTF_SPAN

/**
 * Count the number of code points (characters) in the string assuming that
 * it is valid.
 *
 * This function assumes that the input string is valid UTF-8.
 * It is acceptable to pass invalid UTF-8 strings but in such cases
 * the result is implementation defined.
 *
 * @param input         the UTF-8 string to process
 * @param length        the length of the string in bytes
 * @return number of code points
 */
simdutf_warn_unused size_t count_utf8(const char *input,
                                      size_t length) noexcept;
  #if SIMDUTF_SPAN
simdutf_really_inline simdutf_warn_unused simdutf_constexpr23 size_t count_utf8(
    const detail::input_span_of_byte_like auto &valid_utf8_input) noexcept {
    #if SIMDUTF_CPLUSPLUS23
  if consteval {
    return scalar::utf8::count_code_points(valid_utf8_input.data(),
                                           valid_utf8_input.size());
  } else
    #endif
  {
    return count_utf8(reinterpret_cast<const char *>(valid_utf8_input.data()),
                      valid_utf8_input.size());
  }
}
  #endif // SIMDUTF_SPAN

/**
 * Given a valid UTF-8 string having a possibly truncated last character,
 * this function checks the end of string. If the last character is truncated
 * (or partial), then it returns a shorter length (shorter by 1 to 3 bytes) so
 * that the short UTF-8 strings only contain complete characters. If there is no
 * truncated character, the original length is returned.
 *
 * This function assumes that the input string is valid UTF-8, but possibly
 * truncated.
 *
 * @param input         the UTF-8 string to process
 * @param length        the length of the string in bytes
 * @return the length of the string in bytes, possibly shorter by 1 to 3 bytes
 */
simdutf_warn_unused size_t trim_partial_utf8(const char *input, size_t length);
  #if SIMDUTF_SPAN
simdutf_really_inline simdutf_warn_unused simdutf_constexpr23 size_t
trim_partial_utf8(
    const detail::input_span_of_byte_like auto &valid_utf8_input) noexcept {
    #if SIMDUTF_CPLUSPLUS23
  if consteval {
    return scalar::utf8::trim_partial_utf8(valid_utf8_input.data(),
                                           valid_utf8_input.size());
  } else
    #endif
  {
    return trim_partial_utf8(
        reinterpret_cast<const char *>(valid_utf8_input.data()),
        valid_utf8_input.size());
  }
}
  #endif // SIMDUTF_SPAN

/**
 * An implementation of simdutf for a particular CPU architecture.
 *
 * Also used to maintain the currently active implementation. The active
 * implementation is automatically initialized on first use to the most advanced
 * implementation supported by the host.
 */
class implementation {
public:
  /**
   * The name of this implementation.
   *
   *     const implementation *impl = simdutf::active_implementation;
   *     cout << "simdutf is optimized for " << impl->name() << "(" <<
   * impl->description() << ")" << endl;
   *
   * @return the name of the implementation, e.g. "haswell", "westmere", "arm64"
   */
  virtual std::string_view name() const noexcept { return _name; }

  /**
   * The description of this implementation.
   *
   *     const implementation *impl = simdutf::active_implementation;
   *     cout << "simdutf is optimized for " << impl->name() << "(" <<
   * impl->description() << ")" << endl;
   *
   * @return the name of the implementation, e.g. "haswell", "westmere", "arm64"
   */
  virtual std::string_view description() const noexcept { return _description; }

  /**
   * The instruction sets this implementation is compiled against
   * and the current CPU match. This function may poll the current CPU/system
   * and should therefore not be called too often if performance is a concern.
   *
   *
   * @return true if the implementation can be safely used on the current system
   * (determined at runtime)
   */
  bool supported_by_runtime_system() const;

  /**
   * @private For internal implementation use
   *
   * The instruction sets this implementation is compiled against.
   *
   * @return a mask of all required `internal::instruction_set::` values
   */
  virtual uint32_t required_instruction_sets() const {
    return _required_instruction_sets;
  }

  /**
   * Validate the UTF-8 string.
   *
   * Overridden by each implementation.
   *
   * @param buf the UTF-8 string to validate.
   * @param len the length of the string in bytes.
   * @return true if and only if the string is valid UTF-8.
   */
  simdutf_warn_unused virtual bool validate_utf8(const char *buf,
                                                 size_t len) const noexcept = 0;

  /**
   * Validate the UTF-8 string and stop on errors.
   *
   * Overridden by each implementation.
   *
   * @param buf the UTF-8 string to validate.
   * @param len the length of the string in bytes.
   * @return a result pair struct (of type simdutf::result containing the two
   * fields error and count) with an error code and either position of the error
   * (in the input in code units) if any, or the number of code units validated
   * if successful.
   */
  simdutf_warn_unused virtual result
  validate_utf8_with_errors(const char *buf, size_t len) const noexcept = 0;

  /**
   * Validate the UTF-32 string.
   *
   * Overridden by each implementation.
   *
   * This function is not BOM-aware.
   *
   * @param buf the UTF-32 string to validate.
   * @param len the length of the string in number of 4-byte code units
   * (char32_t).
   * @return true if and only if the string is valid UTF-32.
   */
  simdutf_warn_unused virtual bool
  validate_utf32(const char32_t *buf, size_t len) const noexcept = 0;

  /**
   * Validate the UTF-32 string and stop on error.
   *
   * Overridden by each implementation.
   *
   * This function is not BOM-aware.
   *
   * @param buf the UTF-32 string to validate.
   * @param len the length of the string in number of 4-byte code units
   * (char32_t).
   * @return a result pair struct (of type simdutf::result containing the two
   * fields error and count) with an error code and either position of the error
   * (in the input in code units) if any, or the number of code units validated
   * if successful.
   */
  simdutf_warn_unused virtual result
  validate_utf32_with_errors(const char32_t *buf,
                             size_t len) const noexcept = 0;

  /**
   * Convert possibly broken UTF-8 string into UTF-32 string.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * @param input         the UTF-8 string to convert
   * @param length        the length of the string in bytes
   * @param utf32_output  the pointer to buffer that can hold conversion result
   * @return the number of written char16_t; 0 if the input was not valid UTF-8
   * string
   */
  simdutf_warn_unused virtual size_t
  convert_utf8_to_utf32(const char *input, size_t length,
                        char32_t *utf32_output) const noexcept = 0;

  /**
   * Convert possibly broken UTF-8 string into UTF-32 string and stop on error.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * @param input         the UTF-8 string to convert
   * @param length        the length of the string in bytes
   * @param utf32_output  the pointer to buffer that can hold conversion result
   * @return a result pair struct (of type simdutf::result containing the two
   * fields error and count) with an error code and either position of the error
   * (in the input in code units) if any, or the number of char32_t written if
   * successful.
   */
  simdutf_warn_unused virtual result
  convert_utf8_to_utf32_with_errors(const char *input, size_t length,
                                    char32_t *utf32_output) const noexcept = 0;

  /**
   * Convert valid UTF-8 string into UTF-32 string.
   *
   * This function assumes that the input string is valid UTF-8.
   *
   * @param input         the UTF-8 string to convert
   * @param length        the length of the string in bytes
   * @param utf32_buffer  the pointer to buffer that can hold conversion result
   * @return the number of written char32_t
   */
  simdutf_warn_unused virtual size_t
  convert_valid_utf8_to_utf32(const char *input, size_t length,
                              char32_t *utf32_buffer) const noexcept = 0;

  /**
   * Compute the number of 4-byte code units that this UTF-8 string would
   * require in UTF-32 format.
   *
   * This function is equivalent to count_utf8. It is acceptable to pass invalid
   * UTF-8 strings but in such cases the result is implementation defined.
   *
   * This function does not validate the input.
   *
   * @param input         the UTF-8 string to process
   * @param length        the length of the string in bytes
   * @return the number of char32_t code units required to encode the UTF-8
   * string as UTF-32
   */
  simdutf_warn_unused virtual size_t
  utf32_length_from_utf8(const char *input, size_t length) const noexcept = 0;

  /**
   * Convert possibly broken UTF-32 string into UTF-8 string.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-32 string to convert
   * @param length        the length of the string in 4-byte code units
   * (char32_t)
   * @param utf8_buffer   the pointer to buffer that can hold conversion result
   * @return number of written code units; 0 if input is not a valid UTF-32
   * string
   */
  simdutf_warn_unused virtual size_t
  convert_utf32_to_utf8(const char32_t *input, size_t length,
                        char *utf8_buffer) const noexcept = 0;

  /**
   * Convert possibly broken UTF-32 string into UTF-8 string and stop on error.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-32 string to convert
   * @param length        the length of the string in 4-byte code units
   * (char32_t)
   * @param utf8_buffer   the pointer to buffer that can hold conversion result
   * @return a result pair struct (of type simdutf::result containing the two
   * fields error and count) with an error code and either position of the error
   * (in the input in code units) if any, or the number of char written if
   * successful.
   */
  simdutf_warn_unused virtual result
  convert_utf32_to_utf8_with_errors(const char32_t *input, size_t length,
                                    char *utf8_buffer) const noexcept = 0;

  /**
   * Convert valid UTF-32 string into UTF-8 string.
   *
   * This function assumes that the input string is valid UTF-32.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-32 string to convert
   * @param length        the length of the string in 4-byte code units
   * (char32_t)
   * @param utf8_buffer   the pointer to a buffer that can hold the conversion
   * result
   * @return number of written code units; 0 if conversion is not possible
   */
  simdutf_warn_unused virtual size_t
  convert_valid_utf32_to_utf8(const char32_t *input, size_t length,
                              char *utf8_buffer) const noexcept = 0;

  /**
   * Compute the number of bytes that this UTF-32 string would require in UTF-8
   * format.
   *
   * This function does not validate the input. It is acceptable to pass invalid
   * UTF-32 strings but in such cases the result is implementation defined.
   *
   * @param input         the UTF-32 string to convert
   * @param length        the length of the string in 4-byte code units
   * (char32_t)
   * @return the number of bytes required to encode the UTF-32 string as UTF-8
   */
  simdutf_warn_unused virtual size_t
  utf8_length_from_utf32(const char32_t *input,
                         size_t length) const noexcept = 0;

  /**
   * Count the number of code points (characters) in the string assuming that
   * it is valid.
   *
   * This function assumes that the input string is valid UTF-8.
   * It is acceptable to pass invalid UTF-8 strings but in such cases
   * the result is implementation defined.
   *
   * @param input         the UTF-8 string to process
   * @param length        the length of the string in bytes
   * @return number of code points
   */
  simdutf_warn_unused virtual size_t
  count_utf8(const char *input, size_t length) const noexcept = 0;

#ifdef SIMDUTF_INTERNAL_TESTS
  // This method is exported only in developer mode, its purpose
  // is to expose some internal test procedures from the given
  // implementation and then use them through our standard test
  // framework.
  //
  // Regular users should not use it, the tests of the public
  // API are enough.

  struct TestProcedure {
    // display name
    std::string_view name;

    // procedure should return whether given test pass or not
    void (*procedure)(const implementation &);
  };

  virtual std::vector<TestProcedure> internal_tests() const;
#endif

protected:
  /** @private Construct an implementation with the given name and description.
   * For subclasses.
   * @param name the name of this implementation
   * @param description a description of this implementation
   * @param required_instruction_sets the instruction sets this implementation
   * requires
   */
  simdutf_really_inline implementation(const char *name,
                                       const char *description,
                                       uint32_t required_instruction_sets)
      : _name(name), _description(description),
        _required_instruction_sets(required_instruction_sets) {}

protected:
  ~implementation() = default;

private:
  /**
   * The name of this implementation.
   */
  const char *_name;

  /**
   * The description of this implementation.
   */
  const char *_description;

  /**
   * Instruction sets required for this implementation.
   */
  const uint32_t _required_instruction_sets;
};

/** @private */
namespace internal {

/**
 * The list of available implementations compiled into simdutf.
 */
class available_implementation_list {
public:
  /** Get the list of available implementations compiled into simdutf */
  simdutf_really_inline available_implementation_list() {}
  /** Number of implementations */
  size_t size() const noexcept;
  /** STL const begin() iterator */
  const implementation *const *begin() const noexcept;
  /** STL const end() iterator */
  const implementation *const *end() const noexcept;

  /**
   * Get the implementation with the given name.
   *
   * Case sensitive.
   *
   *     const implementation *impl =
   * simdutf::available_implementations["westmere"]; if (!impl) { exit(1); } if
   * (!imp->supported_by_runtime_system()) { exit(1); }
   *     simdutf::active_implementation = impl;
   *
   * @param name the implementation to find, e.g. "westmere", "haswell", "arm64"
   * @return the implementation, or nullptr if the parse failed.
   */
  const implementation *operator[](std::string_view name) const noexcept {
    for (const implementation *impl : *this) {
      if (impl->name() == name) {
        return impl;
      }
    }
    return nullptr;
  }

  /**
   * Detect the most advanced implementation supported by the current host.
   *
   * This is used to initialize the implementation on startup.
   *
   *     const implementation *impl =
   * simdutf::available_implementation::detect_best_supported();
   *     simdutf::active_implementation = impl;
   *
   * @return the most advanced supported implementation for the current host, or
   * an implementation that returns UNSUPPORTED_ARCHITECTURE if there is no
   * supported implementation. Will never return nullptr.
   */
  const implementation *detect_best_supported() const noexcept;
};

template <typename T> class atomic_ptr {
public:
  atomic_ptr(T *_ptr) : ptr{_ptr} {}

#if defined(SIMDUTF_NO_THREADS)
  operator const T *() const { return ptr; }
  const T &operator*() const { return *ptr; }
  const T *operator->() const { return ptr; }

  operator T *() { return ptr; }
  T &operator*() { return *ptr; }
  T *operator->() { return ptr; }
  atomic_ptr &operator=(T *_ptr) {
    ptr = _ptr;
    return *this;
  }

#else
  operator const T *() const { return ptr.load(); }
  const T &operator*() const { return *ptr; }
  const T *operator->() const { return ptr.load(); }

  operator T *() { return ptr.load(); }
  T &operator*() { return *ptr; }
  T *operator->() { return ptr.load(); }
  atomic_ptr &operator=(T *_ptr) {
    ptr = _ptr;
    return *this;
  }

#endif

private:
#if defined(SIMDUTF_NO_THREADS)
  T *ptr;
#else
  std::atomic<T *> ptr;
#endif
};

class detect_best_supported_implementation_on_first_use;

} // namespace internal

/**
 * The list of available implementations compiled into simdutf.
 */
extern SIMDUTF_DLLIMPORTEXPORT const internal::available_implementation_list &
get_available_implementations();

/**
 * The active implementation.
 *
 * Automatically initialized on first use to the most advanced implementation
 * supported by this hardware.
 */
extern SIMDUTF_DLLIMPORTEXPORT internal::atomic_ptr<const implementation> &
get_active_implementation();

} // namespace simdutf

#if SIMDUTF_CPLUSPLUS23 && SIMDUTF_FEATURE_BASE64

namespace simdutf {
namespace literals {

namespace detail {

// the detail namespace is not part of the public api

template <std::size_t N> struct base64_literal_helper {
  std::array<char, N - 1> storage{};
  static constexpr std::size_t size() noexcept { return N - 1; }
  consteval base64_literal_helper(const char (&str)[N]) {
    for (std::size_t i = 0; i < size(); i++) {
      storage[i] = str[i];
    }
  }
};

template <std::size_t InputLen> struct base64_decode_result {
  static constexpr std::size_t max_out = (InputLen + 3) / 4 * 3;
  std::array<char, max_out> buffer{};
  std::size_t output_count{};
};

template <std::size_t InputLen>
consteval auto base64_decode_literal(const char *str) {
  base64_decode_result<InputLen> result{};
  auto r = scalar::base64::base64_to_binary_details_impl(
      str, InputLen, result.buffer.data(), base64_default, loose);
  if (r.error != error_code::SUCCESS) {
  #if __cpp_lib_unreachable >= 202202L
    std::unreachable(); // invalid base64 input in _base64 literal
  #else
    // workaround for older stdlib
    throw "invalid base64 input in _base64 literal";
  #endif
  }
  result.output_count = r.output_count;
  return result;
}

template <base64_literal_helper a> consteval auto base64_make_array() {
  constexpr auto decoded = base64_decode_literal<a.size()>(a.storage.data());
  std::array<char, decoded.output_count> ret{};
  for (std::size_t i = 0; i < decoded.output_count; i++) {
    ret[i] = decoded.buffer[i];
  }
  return ret;
}

} // namespace detail

/**
 * User-defined literal for compile-time base64 decoding.
 *
 * Usage:
 *   using namespace simdutf::literals;
 *   constexpr auto decoded = "SGVsbG8gV29ybGQh"_base64;
 *   // decoded is a std::array<char, 12> containing "Hello World!"
 *
 * The input must be valid base64. Whitepace is allowed and ignored.
 * A compilation error occurs if the input is invalid.
 */
template <detail::base64_literal_helper a> consteval auto operator""_base64() {
  return detail::base64_make_array<a>();
}

} // namespace literals
} // namespace simdutf

#endif // SIMDUTF_CPLUSPLUS23 && SIMDUTF_FEATURE_BASE64

#endif // SIMDUTF_IMPLEMENTATION_H
/* end file include/simdutf/implementation.h */

// Implementation-internal files (must be included before the implementations
// themselves, to keep amalgamation working--otherwise, the first time a file is
// included, it might be put inside the #ifdef
// SIMDUTF_IMPLEMENTATION_ARM64/FALLBACK/etc., which means the other
// implementations can't compile unless that implementation is turned on).

SIMDUTF_POP_DISABLE_WARNINGS

#endif // SIMDUTF_H
/* end file include/simdutf.h */
