#pragma once

#include <eepp/config.hpp>

#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN

#if __has_include( <simd>) && __cplusplus >= 202002L
#include <simd> // C++23 std::simd
#define USE_STD_SIMD
#elif __has_include( <experimental/simd>)
#include <experimental/simd> // Parallelism TS v2
#define USE_EXPERIMENTAL_SIMD
#endif

#ifdef USE_STD_SIMD
namespace simd = std;
#elif defined( USE_EXPERIMENTAL_SIMD )
namespace simd = std::experimental;
#endif

#if defined( USE_STD_SIMD ) || defined( USE_EXPERIMENTAL_SIMD )
#define EE_STD_SIMD
#endif

#endif
