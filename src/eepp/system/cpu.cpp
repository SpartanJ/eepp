#include <eepp/system/cpu.hpp>

#ifdef EE_ARCH_X86_64
#if defined( _MSC_VER )
#define COMPILER_MSVC 1
#include <intrin.h>
#elif ( defined( __GNUC__ ) || defined( __clang__ ) )
#define COMPILER_GCC_CLANG 1
#include <cpuid.h>
#endif
#endif

namespace EE { namespace System {

bool CPU::hasSSSE3() {
#ifdef EE_ARCH_X86_64
	static bool isSSSE3 = []() {
#if defined( COMPILER_MSVC )
		int cpuInfo[4];
		__cpuid( cpuInfo, 0 );
		if ( cpuInfo[0] < 1 )
			return false;
		__cpuid( cpuInfo, 1 );
		return ( cpuInfo[2] & ( 1 << 9 ) ) != 0;
#elif defined( COMPILER_GCC_CLANG )
		return __builtin_cpu_supports( "ssse3" );
#else
		return false;
#endif
	}();
	return isSSSE3;
#else
	return false;
#endif
}

bool CPU::hasAVX2() {
#ifdef EE_ARCH_X86_64
	static bool isAVX2 = []() {
#if defined( COMPILER_MSVC )
		int cpuInfo[4];
		__cpuid( cpuInfo, 0 );
		if ( cpuInfo[0] < 7 )
			return false;
		__cpuid( cpuInfo, 1 );
		constexpr int OSXSAVE = 1 << 27;
		constexpr int AVX = 1 << 28;
		if ( ( cpuInfo[2] & ( OSXSAVE | AVX ) ) != ( OSXSAVE | AVX ) ||
			 ( _xgetbv( 0 ) & 0x6 ) != 0x6 )
			return false;
		__cpuid( cpuInfo, 7 );
		return ( cpuInfo[1] & ( 1 << 5 ) ) != 0;
#elif defined( COMPILER_GCC_CLANG )
		return __builtin_cpu_supports( "avx2" );
#else
		return false;
#endif
	}();
	return isAVX2;
#else
	return false;
#endif
}

bool CPU::hasNEON() {
#ifdef EE_ARCH_ARM64
	return true; // NEON is mandatory in AArch64
#else
	return false;
#endif
}

}} // namespace EE::System
