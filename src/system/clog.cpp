#include "clog.hpp"

namespace EE { namespace System {

cLog::cLog() :
	mSave( false ),
	mConsoleOutput( false ),
	mLiveWrite( false )
{
	Write("...::: Entropia Engine++ Loaded :::...");
	Write( "Loaded on " + GetDateTimeStr() );
}

cLog::~cLog() {
	Write( "\nUnloaded on " + GetDateTimeStr(), false );
	Write( "...::: Entropia Engine++ Unloaded :::..." );

	if ( mSave && !mLiveWrite ) {
        openfs();
		mFS << mData << std::endl;
	}

	if ( mFS.is_open() ) {
        mFS.close();
	}
}

void cLog::Save( const std::string& filepath ) {
	mFilePath	= filepath;
	mSave		= true;
}

void cLog::Write( const std::string& Text, const bool& newLine ) {
	mData += Text;

	if ( newLine ) {
		mData += '\n';
	}

	if ( mConsoleOutput ) {
		if ( newLine ) {
			std::cout << Text << std::endl;
		} else {
			std::cout << Text;
		}
	}

	if ( mLiveWrite ) {
        openfs();
		mFS << Text << std::endl;
	}
}

void cLog::openfs() {
	if ( !mFilePath.empty() )
		mFilePath = GetProcessPath();

    if ( !mFS.is_open() ) {
        std::string str = mFilePath + "log.log";
        mFS.open( str.c_str(), std::ios::app );
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
				std::cout << tstr << std::endl;
			}

            if ( mLiveWrite ) {
                openfs();
                mFS << tstr << std::endl;
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
	mConsoleOutput = output;
}

const bool& cLog::LiveWrite() const {
    return mLiveWrite;
}

void cLog::LiveWrite( const bool& lw ) {
    mLiveWrite = lw;
}

}}
