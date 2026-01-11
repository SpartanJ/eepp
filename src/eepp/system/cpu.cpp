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

bool CPU::hasAVX2() {
#ifdef EE_ARCH_X86_64
	static bool isAVX2 = []() {
#if defined( COMPILER_MSVC )
		int cpuInfo[4];
		__cpuid( cpuInfo, 0 );
		if ( cpuInfo[0] < 7 )
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
