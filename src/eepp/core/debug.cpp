#include <cassert>
#include <cstdarg>
#include <eepp/core/debug.hpp>
#include <eepp/system/log.hpp>

using namespace EE::System;

#ifdef EE_COMPILER_MSVC
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <crtdbg.h>
#include <windows.h>
#endif

namespace EE {

bool PrintDebugInLog = true;

void eeREPORT_ASSERT( const char* File, int Line, const char* Exp ) {
#if defined( EE_COMPILER_MSVC ) && defined( EE_DEBUG )
	_CrtDbgReport( _CRT_ASSERT, File, Line, "", Exp );

	DebugBreak();
#else

	if ( PrintDebugInLog ) {
		Log::instance()->writef( LogLevel::Assert, "%s file:%s line:%d\n", Exp, File, Line );

		if ( !Log::instance()->isLoggingToStdOut() )
			printf( "ASSERT: %s file:%s line:%d\n", Exp, File, Line );
	} else {
		printf( "ASSERT: %s file:%s line:%d\n", Exp, File, Line );
	}

#if defined( EE_COMPILER_GCC ) && !defined( EE_ARM ) && EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN && \
	EE_PLATFORM != EE_PLATFORM_ANDROID && EE_PLATFORM != EE_PLATFORM_IOS
	asm( "int3" );
#else
	assert( false );
#endif

#endif
}

#ifndef EE_SILENT

static void printBuffer( std::string& buf, bool newLine ) {
	if ( newLine )
		buf += "\n";

#ifdef EE_COMPILER_MSVC
	OutputDebugStringA( buf.c_str() );
#else
	if ( PrintDebugInLog && Log::instance()->isLoggingToStdOut() ) {
		Log::instance()->write( buf );
		return;
	} else {
		printf( "%s", buf.c_str() );
	}
#endif

	if ( PrintDebugInLog )
		Log::instance()->write( buf );
}

void eePRINT( const char* format, ... ) {
	int n, size = 2048;
	std::string buf( size, '\0' );

	va_list args;

	va_start( args, format );

	while ( 1 ) {
		n = vsnprintf( &buf[0], buf.size(), format, args );

		if ( n > -1 && n < size ) {
			buf.resize( n );

			printBuffer( buf, false );

			va_end( args );

			return;
		}

		if ( n > -1 )	  // glibc 2.1
			size = n + 1; // precisely what is needed
		else			  // glibc 2.0
			size *= 2;	  // twice the old size

		buf.resize( size );
	}
}

void eePRINTL( const char* format, ... ) {
	int n, size = 2048;
	std::string buf( size, '\0' );

	va_list args;

	va_start( args, format );

	while ( 1 ) {
		n = vsnprintf( &buf[0], buf.size(), format, args );

		if ( n > -1 && n < size ) {
			buf.resize( n );

			printBuffer( buf, true );

			va_end( args );

			return;
		}

		if ( n > -1 )	  // glibc 2.1
			size = n + 1; // precisely what is needed
		else			  // glibc 2.0
			size *= 2;	  // twice the old size

		buf.resize( size );
	}
}

void eePRINTC( unsigned int cond, const char* format, ... ) {
	if ( 0 == cond )
		return;

	int n, size = 2048;
	std::string buf( size, '\0' );

	va_list args;

	va_start( args, format );

	while ( 1 ) {
		n = vsnprintf( &buf[0], buf.size(), format, args );

		if ( n > -1 && n < size ) {
			buf.resize( n );

			printBuffer( buf, false );

			va_end( args );

			return;
		}

		if ( n > -1 )	  // glibc 2.1
			size = n + 1; // precisely what is needed
		else			  // glibc 2.0
			size *= 2;	  // twice the old size

		buf.resize( size );
	}
}

#endif

} // namespace EE
