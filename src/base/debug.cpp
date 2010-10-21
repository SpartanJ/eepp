#include "debug.hpp"

#ifdef EE_COMPILER_MSVC
#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include "../system/clog.hpp"
using namespace EE::System;

namespace EE {

#ifdef EE_DEBUG

bool PrintDebugInLog = true;

void eeREPORT_ASSERT( const char * File, int Line, const char * Exp ) {
	#ifdef EE_COMPILER_MSVC

	_CrtDbgReport( _CRT_ASSERT, File, Line, "", Exp);

	DebugBreak();

	#else

	printf( "ASSERT: %s file:%s line:%d", Exp, File, Line );

	if ( PrintDebugInLog )
		cLog::instance()->Writef( "ASSERT: %s file:%s line:%d", Exp, File, Line );

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

	if ( PrintDebugInLog )
		cLog::instance()->Write( std::string( buf ) );
}

void eePRINTC( unsigned int cond, const char * format, ...) {
	if ( 0 == cond )
		return;

	char		buf[2048];
	va_list		args;

	va_start( args, format );

	#ifdef EE_COMPILER_MSVC
	_vsnprintf_s( buf, eeARRAY_SIZE( buf ), eeARRAY_SIZE( buf ), format, args );
	#else
	vsnprintf( buf, sizeof( buf ) / sizeof( buf[0]), format, args );
	#endif

	va_end( args );

	#ifdef EE_COMPILER_MSVC
	OutputDebugStringA( buf );
	#else
	printf("%s", buf );
	#endif

	if ( PrintDebugInLog )
		cLog::instance()->Write( std::string( buf ) );
}

#endif

}
