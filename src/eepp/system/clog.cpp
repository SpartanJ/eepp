#include <eepp/system/clog.hpp>
#include <cstdarg>

#if EE_PLATFORM == EE_PLATFORM_ANDROID
	#include <android/log.h>
	#define ANDROID_LOGI(...) __android_log_print(ANDROID_LOG_INFO   , "eepp", __VA_ARGS__)
#endif

#if defined( EE_COMPILER_MSVC )
	#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
	#endif
	#include <windows.h>
#endif

namespace EE { namespace System {

SINGLETON_DECLARE_IMPLEMENTATION(cLog)

cLog::cLog() :
	mSave( false ),
	mConsoleOutput( false ),
	mLiveWrite( false ),
	mFS( NULL )
{
	Write("...::: Entropia Engine++ Loaded :::...");
	Write( "Loaded on " + Sys::GetDateTimeStr() + "\n" );
}

cLog::~cLog() {
	Write( "\nUnloaded on " + Sys::GetDateTimeStr() );
	Write( "...::: Entropia Engine++ Unloaded :::...\n" );

	if ( mSave && !mLiveWrite ) {
		OpenFS();

		mFS->Write( mData.c_str(), mData.size() );
	}

	CloseFS();
}

void cLog::Save( const std::string& filepath ) {
	if ( filepath.size() ) {
		mFilePath	= filepath;
	} else {
		mFilePath	= Sys::GetProcessPath();
	}

	mSave		= true;
}

void cLog::Write( std::string Text, const bool& newLine ) {
	if ( newLine ) {
		Text += '\n';
	}

	mData += Text;

	WriteToReaders( Text );

	if ( mConsoleOutput ) {
	#if EE_PLATFORM == EE_PLATFORM_ANDROID
		ANDROID_LOGI( Text.c_str() );
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

void cLog::OpenFS() {
	if ( mFilePath.empty() ) {
		mFilePath = Sys::GetProcessPath();
	}

	if ( NULL == mFS ) {
		std::string str = mFilePath + "log.log";

		mFS = eeNew( cIOStreamFile, ( str, std::ios::app | std::ios::out | std::ios::binary ) );
	}
}

void cLog::CloseFS() {
	Lock();

	eeSAFE_DELETE( mFS );

	Unlock();
}

void cLog::Writef( const char* format, ... ) {
	int n, size = 256;
	std::string tstr( size, '\0' );

	va_list args;

	while (1) {
		va_start( args, format );

		n = eevsnprintf( &tstr[0], size, format, args );

		if ( n > -1 && n < size ) {
			tstr.resize( n );
			tstr += '\n';

			mData += tstr;

			WriteToReaders( tstr );

			if ( mConsoleOutput ) {
			#if EE_PLATFORM == EE_PLATFORM_ANDROID
				ANDROID_LOGI( tstr.c_str() );
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

std::string cLog::Buffer() const {
	return mData;
}

const bool& cLog::ConsoleOutput() const {
	return mConsoleOutput;
}

void cLog::ConsoleOutput( const bool& output ) {
	bool OldOutput = mConsoleOutput;

	mConsoleOutput = output;

	if ( !OldOutput && output ) {
		std::string data( mData );
		mData = "";
		Write( data, false );
	}

}

const bool& cLog::LiveWrite() const {
    return mLiveWrite;
}

void cLog::LiveWrite( const bool& lw ) {
	mLiveWrite = lw;
}

void cLog::AddLogReader( iLogReader * reader ) {
	mReaders.push_back( reader );
}

void cLog::RemoveLogReader( iLogReader * reader ) {
	mReaders.remove( reader );
}

void cLog::WriteToReaders( std::string& text ) {
	for ( std::list<iLogReader*>::iterator it = mReaders.begin(); it != mReaders.end(); it++ ) {
		(*it)->WriteLog( text );
	}
}

}}
