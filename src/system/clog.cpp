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
		mData += "\n";
}

void cLog::Write( const char* format, ... ) {
	char buf[256];
	
	va_list( args );
	
	va_start( args, format );
	
	#ifdef EE_COMPILER_MSVC
	int nb = _vsnprintf_s( buf, 256, 256, format, args );
	#else
	int nb = vsnprintf(buf, 256, format, args);
	#endif
	
	va_end( args );
	
	if ( nb < 256 ) {
		Write( std::string( buf ) );
		return;
	}
	
	// The static size was not big enough, try again with a dynamic allocation.
	++nb;
	
	char * buf2 = new char[nb];
	
	va_start( args, format );
	
	#ifdef EE_COMPILER_MSVC
	_vsnprintf_s( buf2, nb, nb, format, args );
	#else
	vsnprintf( buf2, nb, format, args );
	#endif
	
	va_end( args );
	
	Write( std::string( buf2 ) );
	
	delete [] buf2;
}

}}
