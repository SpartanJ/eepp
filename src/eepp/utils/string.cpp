#include <cctype>
#include <eepp/utils/string.hpp>

namespace EE { namespace Utils {

bool IsCharacter( const eeInt& mValue ) {
	return (mValue >= 32 && mValue <= 126) || (mValue >= 161 && mValue <= 255) || (mValue == 9);
}

bool IsNumber( const eeInt& mValue, bool AllowDot ) {
	if ( AllowDot )
		return ( mValue >= 48 && mValue <= 57 ) || mValue == 46;

	return mValue >= 48 && mValue <= 57;
}

bool IsLetter( const eeInt& mValue ) {
	return ( ( (mValue >= 65 && mValue <= 90) || (mValue >= 97 && mValue <= 122) || (mValue >= 192 && mValue <= 255) ) && (mValue != 215) && (mValue != 247) );
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

std::vector < String > SplitString ( const String& str, const Uint32& splitchar, const bool& pushEmptyString ) {
	std::vector < String > tmp;
	String tmpstr;

	for ( eeUint i = 0; i < str.size(); i++ ) {
		if ( str[i] == splitchar ) {
			if ( pushEmptyString || tmpstr.size() ) {
				tmp.push_back(tmpstr);
				tmpstr = "";
			}
		} else {
			tmpstr += str[i];
		}
	}

	if ( tmpstr.size() )
		tmp.push_back(tmpstr);

	return tmp;
}

std::vector < std::string > SplitString ( const std::string& str, const Int8& splitchar, const bool& pushEmptyString ) {
	std::vector < std::string > tmp;
	std::string tmpstr;

	for ( eeUint i = 0; i < str.size(); i++ ) {
		if ( str[i] == splitchar ) {
			if ( pushEmptyString || tmpstr.size() ) {
				tmp.push_back(tmpstr);
				tmpstr = "";
			}
		} else {
			tmpstr += str[i];
		}
	}

	if ( tmpstr.size() )
		tmp.push_back( tmpstr );

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

std::string StrFormated( const char * format, ... ) {
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
			return tstr;
		}

		if ( n > -1 )	// glibc 2.1
			size = n+1; // precisely what is needed
		else			// glibc 2.0
			size *= 2;	// twice the old size

		tstr.resize( size, '\0' );
	}
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

void ToUpper( std::string & str ) {
	std::transform(str.begin(), str.end(), str.begin(), (int(*)(int)) std::toupper);
}

void ToLower( std::string & str ) {
	std::transform(str.begin(), str.end(), str.begin(), (int(*)(int)) std::tolower);
}

std::vector<Uint8> stringToUint8( const std::string& str ) {
	return std::vector<Uint8>( str.begin(), str.end() );
}

std::string Uint8Tostring( const std::vector<Uint8> v ) {
	return std::string( v.begin(), v.end() );
}

void InsertChar( String& str, const eeUint& pos, const Uint32& tchar ) {
	str.insert( str.begin() + pos, tchar );
}

std::string StoragePath( std::string appname ) {
	char path[256];

	#if EE_PLATFORM == EE_PLATFORM_WIN
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

        #if EE_PLATFORM != EE_PLATFORM_MACOSX
        int i;
        #endif
        if(!home)
            return std::string();

        #if EE_PLATFORM == EE_PLATFORM_MACOSX
            snprintf(path, 256, "%s/Library/Application Support/%s", home, appname.c_str() );
        #else
            snprintf(path, 256, "%s/.%s", home, appname.c_str() );
            for(i = strlen(home)+2; path[i]; i++)
				path[i] = std::tolower(path[i]);
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

Int32 StrStartsWith( const std::string& Start, const std::string Str ) {
	Int32 Pos = -1;

	if ( Str.size() >= Start.size() ) {
		for ( Uint32 i = 0; i < Start.size(); i++ ) {
			if ( Start[i] == Str[i] ) {
				Pos = (Int32)i;
			} else {
				Pos = -1;
				break;
			}
		}
	}

	return Pos;
}

Int32 StrStartsWith( const String& Start, const String Str ) {
	Int32 Pos = -1;

	if ( Str.size() >= Start.size() ) {
		for ( Uint32 i = 0; i < Start.size(); i++ ) {
			if ( Start[i] == Str[i] ) {
				Pos = (Int32)i;
			} else {
				Pos = -1;
				break;
			}
		}
	}

	return Pos;
}

void ReplaceSubStr( std::string &target, const std::string& that, const std::string& with ) {
	std::string::size_type pos=0;

	while( ( pos = target.find( that, pos ) ) != std::string::npos ) {
		target.erase( pos, that.length() );
		target.insert( pos, with );
		pos += with.length();
	}
}

std::string RemoveNumbersAtEnd( std::string txt ) {
	while ( txt.size() && txt[ txt.size() - 1 ] >= '0' && txt[ txt.size() - 1 ] <= '9' ) {
		txt.resize( txt.size() - 1 );
	}

	return txt;
}

}}
