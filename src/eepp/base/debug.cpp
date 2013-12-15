#include <eepp/base/debug.hpp>
#include <eepp/system/clog.hpp>
#include <cassert>
#include <cstdarg>

using namespace EE::System;

#ifdef EE_COMPILER_MSVC
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <crtdbg.h>
#endif

namespace EE {

bool PrintDebugInLog = true;

#ifdef EE_DEBUG

void eeREPORT_ASSERT( const char * File, int Line, const char * Exp ) {
	#ifdef EE_COMPILER_MSVC

	_CrtDbgReport( _CRT_ASSERT, File, Line, "", Exp);

	DebugBreak();

	#else

	if ( PrintDebugInLog ) {
		cLog::instance()->Writef( "ASSERT: %s file:%s line:%d", Exp, File, Line );

		if ( !cLog::instance()->ConsoleOutput() )
			printf( "ASSERT: %s file:%s line:%d", Exp, File, Line );
	} else {
		printf( "ASSERT: %s file:%s line:%d", Exp, File, Line );
	}

	#if defined(EE_COMPILER_GCC) && defined(EE_32BIT) && !defined(EE_ARM)
	asm("int3");
	#else
	assert( false );
	#endif

	#endif
}

#endif

#ifndef EE_SILENT

static void print_buffer( std::string& buf, bool newLine ) {
	if ( newLine )
		buf += "\n";

	#ifdef EE_COMPILER_MSVC
		OutputDebugStringA( buf.c_str() );
	#else
		if ( PrintDebugInLog && cLog::instance()->ConsoleOutput() ) {
			cLog::instance()->Write( buf, false );
			return;
		} else {
			printf("%s", buf.c_str() );
		}
	#endif

	if ( PrintDebugInLog )
		cLog::instance()->Write( buf, false );
}

void eePRINT( const char * format, ... ) {
	int n, size = 2048;
	std::string buf( size, '\0' );

	va_list		args;

	while ( 1 ) {
		va_start( args, format );

		n = vsnprintf( &buf[0], buf.size(), format, args );

		if ( n > -1 && n < size ) {
			buf.resize( n );

			print_buffer( buf, false );

			va_end( args );

			return;
		}

		if ( n > -1 )	// glibc 2.1
			size = n+1; // precisely what is needed
		else			// glibc 2.0
			size *= 2;	// twice the old size

		buf.resize( size );
	}
}

void eePRINTL( const char * format, ... ) {
	int n, size = 2048;
	std::string buf( size, '\0' );

	va_list		args;

	while ( 1 ) {
		va_start( args, format );

		n = vsnprintf( &buf[0], buf.size(), format, args );

		if ( n > -1 && n < size ) {
			buf.resize( n );

			print_buffer( buf, true );

			va_end( args );

			return;
		}

		if ( n > -1 )	// glibc 2.1
			size = n+1; // precisely what is needed
		else			// glibc 2.0
			size *= 2;	// twice the old size

		buf.resize( size );
	}
}

void eePRINTC( unsigned int cond, const char * format, ...) {
	if ( 0 == cond )
		return;

	int n, size = 2048;
	std::string buf( size, '\0' );

	va_list		args;

	while ( 1 ) {
		va_start( args, format );

		n = vsnprintf( &buf[0], buf.size(), format, args );

		if ( n > -1 && n < size ) {
			buf.resize( n );

			print_buffer( buf, false );

			va_end( args );

			return;
		}

		if ( n > -1 )	// glibc 2.1
			size = n+1; // precisely what is needed
		else			// glibc 2.0
			size *= 2;	// twice the old size

		buf.resize( size );
	}
}

#endif

}
