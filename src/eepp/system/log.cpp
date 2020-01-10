#include <cstdarg>
#include <eepp/system/log.hpp>
#include <iostream>

#if EE_PLATFORM == EE_PLATFORM_ANDROID
#include <android/log.h>
#endif

#if defined( EE_COMPILER_MSVC )
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace EE { namespace System {

SINGLETON_DECLARE_IMPLEMENTATION( Log )

Log::Log() : mSave( false ), mConsoleOutput( false ), mLiveWrite( false ), mFS( NULL ) {
	write( "...::: Entropia Engine++ Loaded :::..." );
	write( "Loaded on " + Sys::getDateTimeStr() + "\n" );
}

Log::~Log() {
	write( "\nUnloaded on " + Sys::getDateTimeStr() );
	write( "...::: Entropia Engine++ Unloaded :::...\n" );

	if ( mSave && !mLiveWrite ) {
		openFS();

		mFS->write( mData.c_str(), mData.size() );
	}

	closeFS();
}

void Log::save( const std::string& filepath ) {
	if ( filepath.size() ) {
		mFilePath = filepath;
	} else {
		mFilePath = Sys::getProcessPath();
	}

	mSave = true;
}

void Log::write( std::string Text, const bool& newLine ) {
	if ( newLine ) {
		Text += '\n';
	}

	mData += Text;

	writeToReaders( Text );

	if ( mConsoleOutput ) {
#if EE_PLATFORM == EE_PLATFORM_ANDROID
		__android_log_print( ANDROID_LOG_INFO, "eepp", "%s", Text.c_str() );
#elif defined( EE_COMPILER_MSVC )
#ifdef UNICODE
		OutputDebugString( String::fromUtf8( Text ).toWideString().c_str() );
#else
		OutputDebugString( Text.c_str() );
#endif
#else
		std::cout << Text;
#endif
	}

	if ( mLiveWrite ) {
		openFS();

		mFS->write( Text.c_str(), Text.size() );

		mFS->flush();
	}
}

void Log::openFS() {
	if ( mFilePath.empty() ) {
		mFilePath = Sys::getProcessPath();
	}

	if ( NULL == mFS ) {
		std::string str = mFilePath + "log.log";

		mFS = IOStreamFile::New( str, "a" );
	}
}

void Log::closeFS() {
	lock();

	eeSAFE_DELETE( mFS );

	unlock();
}

void Log::writef( const char* format, ... ) {
	int n, size = 256;
	std::string tstr( size, '\0' );

	va_list args;

	while ( 1 ) {
		va_start( args, format );

		n = vsnprintf( &tstr[0], size, format, args );

		if ( n > -1 && n < size ) {
			tstr.resize( n );
			tstr += '\n';

			mData += tstr;

			writeToReaders( tstr );

			if ( mConsoleOutput ) {
#if EE_PLATFORM == EE_PLATFORM_ANDROID
				__android_log_print( ANDROID_LOG_INFO, "eepp", "%s", tstr.c_str() );
#elif defined( EE_COMPILER_MSVC )
#ifdef UNICODE
				OutputDebugString( String::fromUtf8( tstr ).toWideString().c_str() );
#else
				OutputDebugString( tstr.c_str() );
#endif
#else
				std::cout << tstr;
#endif
			}

			if ( mLiveWrite ) {
				openFS();

				mFS->write( tstr.c_str(), tstr.size() );

				mFS->flush();
			}

			va_end( args );

			return;
		}

		if ( n > -1 )	  // glibc 2.1
			size = n + 1; // precisely what is needed
		else			  // glibc 2.0
			size *= 2;	  // twice the old size

		tstr.resize( size, '\0' );
	}
}

std::string Log::getBuffer() const {
	return mData;
}

const bool& Log::isConsoleOutput() const {
	return mConsoleOutput;
}

void Log::setConsoleOutput( const bool& output ) {
	bool OldOutput = mConsoleOutput;

	mConsoleOutput = output;

	if ( !OldOutput && output ) {
		std::string data( mData );
		mData = "";
		write( data, false );
	}
}

const bool& Log::isLiveWrite() const {
	return mLiveWrite;
}

void Log::setLiveWrite( const bool& lw ) {
	mLiveWrite = lw;
}

void Log::addLogReader( LogReaderInterface* reader ) {
	mReaders.push_back( reader );
}

void Log::removeLogReader( LogReaderInterface* reader ) {
	mReaders.remove( reader );
}

void Log::writeToReaders( std::string& text ) {
	for ( std::list<LogReaderInterface*>::iterator it = mReaders.begin(); it != mReaders.end();
		  ++it ) {
		( *it )->writeLog( text );
	}
}

}} // namespace EE::System
