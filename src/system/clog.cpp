#include "clog.hpp"

namespace EE { namespace System {

SINGLETON_DECLARE_IMPLEMENTATION(cLog)

cLog::cLog() :
	mSave( false ),
	mConsoleOutput( false ),
	mLiveWrite( false ),
	mFS( NULL )
{
	Write("...::: Entropia Engine++ Loaded :::...");
	Write( "Loaded on " + GetDateTimeStr() );
}

cLog::~cLog() {
	Write( "\nUnloaded on " + GetDateTimeStr(), false );
	Write( "...::: Entropia Engine++ Unloaded :::...\n" );

	if ( mSave && !mLiveWrite ) {
        openfs();

		mFS->Write( mData.c_str(), mData.size() );
	}

	eeSAFE_DELETE( mFS );
}

void cLog::Save( const std::string& filepath ) {
	if ( filepath.size() ) {
		mFilePath	= filepath;
	} else {
		mFilePath	= GetProcessPath();
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
	}
}

void cLog::openfs() {
	if ( mFilePath.empty() ) {
		mFilePath = GetProcessPath();
	}

	if ( NULL == mFS ) {
        std::string str = mFilePath + "log.log";

		mFS = eeNew( cIOStreamFile, ( str, std::ios::app | std::ios::out | std::ios::binary ) );
    }
}

void cLog::Writef( const char* format, ... ) {
	int n, size = 256;
	std::string tstr( size, '\0' );

	va_list args;

	while (1) {
		va_start( args, format );

		#ifdef EE_COMPILER_MSVC
			n = _vsnprintf_s( &tstr[0], size, size, format, args );
		#else
			n = vsnprintf( &tstr[0], size, format, args );
		#endif

		va_end( args );

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
