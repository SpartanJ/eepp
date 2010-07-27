#include "clog.hpp"

namespace EE { namespace System {

cLog::cLog() : mSave(false) {
	Write("...::: Entropia Engine++ Loaded :::...");
	Write( "Loaded on " + GetDateTimeStr() );
}

cLog::~cLog() {
	Write( "\nUnloaded on " + GetDateTimeStr(), false );
	Write( "...::: Entropia Engine++ Unloaded :::..." );

	if ( !mFilePath.empty() )
		mFilePath = AppPath();

	if (mSave) {
		std::string str = mFilePath;
		str += "log.log";

		ofstream fs(str.c_str(), ios::app);

		fs << mData << endl;
		fs.close();
	}
}

void cLog::Save(const std::string& filepath) {
	mFilePath = filepath;
	mSave = true;
}

void cLog::Write(const std::string& Text, const bool& newLine) {
	mData += Text;
	if ( newLine )
		mData += '\n';
}

void cLog::Write( const char* format, ... ) {
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

			return;
		}

		if ( n > -1 )	// glibc 2.1
			size = n+1; // precisely what is needed
		else			// glibc 2.0
			size *= 2;	// twice the old size

		tstr.resize( size, '\0' );
	}
}

}}
