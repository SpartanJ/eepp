#include <eepp/base/debug.hpp>
#include <eepp/system/clog.hpp>
#include <cassert>
#include <cstdarg>

using namespace EE::System;

#ifdef EE_COMPILER_MSVC
#include <windows.h>
#include <crtdbg.h>
#endif

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

	#if defined(EE_COMPILER_GCC) && defined(EE_32BIT) && !defined(EE_ARM)
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

	eevsnprintf( buf, sizeof( buf ), format, args );

	va_end( args );

	#ifdef EE_COMPILER_MSVC
	OutputDebugStringA( buf );
	#else
	if ( PrintDebugInLog && cLog::instance()->ConsoleOutput() && cLog::instance()->LiveWrite() )
		cLog::instance()->Write( std::string( buf ) );
	else
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

	eevsnprintf( buf, sizeof( buf ) / sizeof( buf[0]), format, args );

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
