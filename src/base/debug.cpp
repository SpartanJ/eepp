#include "debug.hpp"

namespace EE {

void eeREPORT_ASSERT( const char * File, int Line, const char * Exp ) {
	#ifdef EE_COMPILER_MSVC

	_CrtDbgReport( _CRT_ASSERT, File, Line, "", Exp);

	DebugBreak();

	#else

	printf("ASSERT: %s file:%s line:%d", Exp, File, Line );

	#if defined(EE_COMPILER_GCC) && defined(EE_32BIT)
	asm("int3");
	#else
	assert( false );
	#endif

	#endif
}

void eePRINT( const char * format, ... ) {
	char		buf[2048];
	va_list		args;

	va_start( args, format );

	#ifdef EE_COMPILER_MSVC
	_vsnprintf_s( buf, sizeof( buf ), sizeof( buf ) / sizeof( buf[0]), format, args );
	#else
	vsnprintf( buf, sizeof( buf ) / sizeof( buf[0]), format, args );
	#endif

	va_end( args );

	#ifdef EE_COMPILER_MSVC
	OutputDebugStringA( buf );
	#else
	printf("%s", buf );
	#endif
}

void eePRINTC( unsigned int cond, const char * format, ...) {
	if ( 0 == cond )
		return;

	char		buf[2048];
	va_list		args;

	va_start( args, format );

	#ifdef EE_COMPILER_MSVC
	_vsnprintf_s( buf, sizeof( buf ), sizeof( buf ) / sizeof( buf[0]), 8format, args );
	#else
	vsnprintf( buf, sizeof( buf ) / sizeof( buf[0]), format, args );
	#endif

	va_end( args );

	#ifdef EE_COMPILER_MSVC
	OutputDebugStringA( buf );
	#else
	printf("%s", buf );
	#endif
}

}
