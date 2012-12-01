#include <eepp/system/clog.hpp>
#include <cstdarg>

#if EE_PLATFORM == EE_PLATFORM_ANDROID
	#include <android/log.h>
	#define ANDROID_LOGI(...) __android_log_print(ANDROID_LOG_INFO   , "eepp", __VA_ARGS__)
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
	Write( "Loaded on " + Sys::GetDateTimeStr() );
}

cLog::~cLog() {
	Write( "\nUnloaded on " + Sys::GetDateTimeStr(), false );
	Write( "...::: Entropia Engine++ Unloaded :::...\n" );

	if ( mSave && !mLiveWrite ) {
        openfs();

		mFS->Write( mData.c_str(), mData.size() );
	}

	closefs();
}

void cLog::Save( const std::string& filepath ) {
	if ( filepath.size() ) {
		mFilePath	= filepath;
	} else {
		mFilePath	= Sys::GetProcessPath();
	}

	mSave		= true;
}

void cLog::Write( const std::string& Text, const bool& newLine ) {
	mData += Text;

	if ( newLine ) {
		mData += '\n';
	}
	
	if ( mConsoleOutput ) {
	#if EE_PLATFORM == EE_PLATFORM_ANDROID
		if ( newLine ) {
			ANDROID_LOGI( ( Text + std::string( "\n" ) ).c_str() );
		} else {
			ANDROID_LOGI( Text.c_str() );
		}
	#else
		if ( newLine ) {
			std::cout << Text << std::endl;
		} else {
			std::cout << Text;
		}
	#endif
	}
	
	if ( mLiveWrite ) {
        openfs();

		mFS->Write( Text.c_str(), Text.size() );

		if ( newLine ) {
			mFS->Write( "\n", 1 );
		}

		closefs();
	}
}

void cLog::openfs() {
	if ( mFilePath.empty() ) {
		mFilePath = Sys::GetProcessPath();
	}

	closefs();

	if ( NULL == mFS ) {
        std::string str = mFilePath + "log.log";

		mFS = eeNew( cIOStreamFile, ( str, std::ios::app | std::ios::out | std::ios::binary ) );
    }
}

void cLog::closefs() {
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

			mData += tstr + '\n';

			if ( mConsoleOutput ) {
				#if EE_PLATFORM != EE_PLATFORM_ANDROID
				std::cout << tstr << std::endl;
				#else
				ANDROID_LOGI( ( tstr + std::string( "\n" ) ).c_str() );
				#endif
			}

            if ( mLiveWrite ) {
                openfs();

				tstr += '\n';

				mFS->Write( tstr.c_str(), tstr.size() );

				closefs();
            }

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

}}
