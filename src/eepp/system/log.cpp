#include <eepp/system/log.hpp>
#include <cstdarg>

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

SINGLETON_DECLARE_IMPLEMENTATION(Log)

Log::Log() :
	mSave( false ),
	mConsoleOutput( false ),
	mLiveWrite( false ),
	mFS( NULL )
{
	Write("...::: Entropia Engine++ Loaded :::...");
	Write( "Loaded on " + Sys::GetDateTimeStr() + "\n" );
}

Log::~Log() {
	Write( "\nUnloaded on " + Sys::GetDateTimeStr() );
	Write( "...::: Entropia Engine++ Unloaded :::...\n" );

	if ( mSave && !mLiveWrite ) {
		OpenFS();

		mFS->Write( mData.c_str(), mData.size() );
	}

	CloseFS();
}

void Log::Save( const std::string& filepath ) {
	if ( filepath.size() ) {
		mFilePath	= filepath;
	} else {
		mFilePath	= Sys::GetProcessPath();
	}

	mSave		= true;
}

void Log::Write( std::string Text, const bool& newLine ) {
	if ( newLine ) {
		Text += '\n';
	}

	mData += Text;

	WriteToReaders( Text );

	if ( mConsoleOutput ) {
	#if EE_PLATFORM == EE_PLATFORM_ANDROID
		__android_log_print( ANDROID_LOG_INFO, "eepp", "%s", Text.c_str() );
	#elif defined( EE_COMPILER_MSVC )
		OutputDebugString( Text.c_str() );
	#else
		std::cout << Text;
	#endif
	}
	
	if ( mLiveWrite ) {
		OpenFS();

		mFS->Write( Text.c_str(), Text.size() );

		mFS->Flush();
	}
}

void Log::OpenFS() {
	if ( mFilePath.empty() ) {
		mFilePath = Sys::GetProcessPath();
	}

	if ( NULL == mFS ) {
		std::string str = mFilePath + "log.log";

		mFS = eeNew( IOStreamFile, ( str, std::ios::app | std::ios::out | std::ios::binary ) );
	}
}

void Log::CloseFS() {
	Lock();

	eeSAFE_DELETE( mFS );

	Unlock();
}

void Log::Writef( const char* format, ... ) {
	int n, size = 256;
	std::string tstr( size, '\0' );

	va_list args;

	while (1) {
		va_start( args, format );

		n = vsnprintf( &tstr[0], size, format, args );

		if ( n > -1 && n < size ) {
			tstr.resize( n );
			tstr += '\n';

			mData += tstr;

			WriteToReaders( tstr );

			if ( mConsoleOutput ) {
			#if EE_PLATFORM == EE_PLATFORM_ANDROID
				__android_log_print( ANDROID_LOG_INFO, "eepp", "%s", tstr.c_str() );
			#elif defined( EE_COMPILER_MSVC )
				OutputDebugString( tstr.c_str() );
			#else
				std::cout << tstr;
			#endif
			}

			if ( mLiveWrite ) {
				OpenFS();

				mFS->Write( tstr.c_str(), tstr.size() );

				mFS->Flush();
			}

			va_end( args );

			return;
		}

		if ( n > -1 )	// glibc 2.1
			size = n+1; // precisely what is needed
		else			// glibc 2.0
			size *= 2;	// twice the old size

		tstr.resize( size, '\0' );
	}
}

std::string Log::Buffer() const {
	return mData;
}

const bool& Log::ConsoleOutput() const {
	return mConsoleOutput;
}

void Log::ConsoleOutput( const bool& output ) {
	bool OldOutput = mConsoleOutput;

	mConsoleOutput = output;

	if ( !OldOutput && output ) {
		std::string data( mData );
		mData = "";
		Write( data, false );
	}

}

const bool& Log::LiveWrite() const {
    return mLiveWrite;
}

void Log::LiveWrite( const bool& lw ) {
	mLiveWrite = lw;
}

void Log::AddLogReader( LogReaderInterface * reader ) {
	mReaders.push_back( reader );
}

void Log::RemoveLogReader( LogReaderInterface * reader ) {
	mReaders.remove( reader );
}

void Log::WriteToReaders( std::string& text ) {
	for ( std::list<LogReaderInterface*>::iterator it = mReaders.begin(); it != mReaders.end(); it++ ) {
		(*it)->WriteLog( text );
	}
}

}}
