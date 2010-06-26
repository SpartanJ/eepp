#include "string.hpp"

namespace EE { namespace Utils {

bool isCharacter( const eeInt& mValue ) {
	return (mValue >= 32 && mValue <= 126) || (mValue >= 161 && mValue <= 255) || (mValue == 9);
}

bool isNumber( const eeInt& mValue ) {
	return mValue >= 48 && mValue <= 57;
}

bool isLetter( const eeInt& mValue ) {
	return ( ( (mValue >= 65 && mValue <= 90) || (mValue >= 97 && mValue <= 122) || (mValue >= 192 && mValue <= 255) ) && (mValue != 215) && (mValue != 247) );
}

std::wstring stringTowstring(const std::string& s) {
	std::wstring temp(s.length(), L' ');
	std::copy(s.begin(), s.end(), temp.begin());
	return temp;
}

std::string wstringTostring(const std::wstring& s) {
	std::string temp(s.length(), ' ');
	std::copy(s.begin(), s.end(), temp.begin());
	return temp;
}

std::string intToStr(Int32 n) {
	char buf[10];

#ifdef EE_COMPILER_MSVC
	_itoa_s(n, &buf[0], 10, 10 );
#else
	std::sprintf(buf, "%d", n);
#endif

	return std::string( buf );
}

std::string floatToStr(eeFloat n) {
	char buf[24];
#ifdef EE_COMPILER_MSVC
	_snprintf_s( &buf[0], 24, 24, "%f", n );
#else
	std::sprintf(buf, "%f", n);
#endif
	return std::string( buf );
}

std::string GetDateTimeStr() {
	time_t rawtime;
	time ( &rawtime );

#ifdef EE_COMPILER_MSVC
	char buf[256];
	struct tm timeinfo;
	localtime_s ( &timeinfo, &rawtime );
	asctime_s( &buf[0], 256, &timeinfo );
	return std::string( buf );
#else
	struct tm * timeinfo;
	timeinfo = localtime ( &rawtime );
	return std::string( asctime (timeinfo) );
#endif
}

std::vector < std::wstring > SplitString ( const std::wstring& str, const Uint32& splitchar ) {
	std::vector < std::wstring > tmp;
	std::wstring tmpstr;

	for ( eeUint i = 0; i < str.size(); i++ ) {
		if ( str[i] == (eeInt)splitchar ) {
			tmp.push_back(tmpstr);
			tmpstr = L"";
		} else {
			tmpstr += str[i];
		}
	}
	tmp.push_back(tmpstr);

	return tmp;
}

std::vector < std::string > SplitString ( const std::string& str, const Int8& splitchar ) {
	std::vector < std::string > tmp;
	std::string tmpstr;

	for ( eeUint i = 0; i < str.size(); i++ ) {
		if ( str[i] == splitchar ) {
			tmp.push_back(tmpstr);
			tmpstr = "";
		} else {
			tmpstr += str[i];
		}
	}
	tmp.push_back(tmpstr);

	return tmp;
}

void StrFormat( char * Buffer, int BufferSize, const char * format, ... ) {
	va_list	args;
	va_start( args, format );
#ifdef EE_COMPILER_MSVC
	_vsnprintf_s( Buffer, BufferSize, BufferSize, format, args );
#else
	vsnprintf( Buffer, BufferSize-1, format, args );
#endif
	va_end( args );
}

std::string StrFormated( const char* format, ... ) {
	char buf[256];

	va_list( args );

	va_start( args, format );

	#ifdef EE_COMPILER_MSVC
	int nb = _vsnprintf_s( buf, 256, 256, format, args );
	#else
	int nb = vsnprintf( buf, 256, format, args );
	#endif

	va_end( args );

	if ( nb < 256 )
		return buf;

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

	std::string res( buf2 );

	delete [] buf2;

	return res;
}

std::string LTrim( const std::string & str ) {
	std::string::size_type pos1 = str.find_first_not_of(' ');
	return ( pos1 == std::string::npos ) ? str : str.substr( pos1 );
}

std::string Trim( const std::string & str ) {
	std::string::size_type pos1 = str.find_first_not_of(' ');
	std::string::size_type pos2 = str.find_last_not_of(' ');
	return str.substr(pos1 == std::string::npos ? 0 : pos1, pos2 == std::string::npos ? str.length() - 1 : pos2 - pos1 + 1);
}

void toUpper( std::string & str ) {
	std::transform(str.begin(), str.end(), str.begin(), (int(*)(int)) std::toupper);
}

void toLower( std::string & str ) {
	std::transform(str.begin(), str.end(), str.begin(), (int(*)(int)) std::tolower);
}

std::vector<Uint8> stringToUint8( const std::string& str ) {
	return std::vector<Uint8>( str.begin(), str.end() );
}

std::vector<Uint8> wstringToUint8( const std::wstring& str ) {
	return std::vector<Uint8>( str.begin(), str.end() );
}

std::string Uint8Tostring( const std::vector<Uint8> v ) {
	return std::string( v.begin(), v.end() );
}

std::wstring Uint8Towstring( const std::vector<Uint8> v ) {
	return std::wstring( v.begin(), v.end() );
}

void InsertChar( std::wstring& str, const eeUint& pos, const Uint32& tchar ) {
	std::wstring tStr( str.length() + 1, L' ');
	for ( eeUint i = 0; i < tStr.size(); i++ ) {
		if ( i < pos ) {
			tStr[i] = str[i];
		} else {
			if ( pos != i ) {
				tStr[i] = str[i-1];
			} else {
				tStr[i] = tchar;
			}
		}
	}
	str = tStr;
}

std::string StoragePath( std::string appname ) {
	char path[256];

	#if EE_PLATFORM == EE_PLATFORM_WIN32
		#ifdef EE_COMPILER_MSVC

		char * ppath;
		size_t ssize = 256;

		_dupenv_s( &ppath, &ssize, "APPDATA" );

		StrCopy( path, ppath, 256 );

		free( ppath );

		if( !ssize )
			return std::string();

		#else
		char * home = getenv("APPDATA");

		if( !home )
			return std::string();

		_snprintf(path, 256, "%s\\%s", home, appname.c_str() );

		#endif
	#else
        char *home = getenv("HOME");

        #if EE_PLATFORM != EE_PLATFORM_APPLE
        int i;
        #endif
        if(!home)
            return std::string();

        #if EE_PLATFORM == EE_PLATFORM_APPLE
            snprintf(path, 256, "%s/Library/Application Support/%s", home, appname.c_str() );
        #else
            snprintf(path, 256, "%s/.%s", home, appname.c_str() );
            for(i = strlen(home)+2; path[i]; i++)
                path[i] = tolower(path[i]);
        #endif
    #endif

	return std::string( path );
}

void StrCopy( char * Dst, const char * Src, eeUint DstSize ) {
	char * DstEnd = Dst + DstSize - 1;

	while ( Dst < DstEnd && *Src ) {
		*Dst = *Src;
		Dst++;
		Src++;
	}

	*Dst = 0;
}

}}
