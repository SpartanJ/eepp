#include <iterator>
#include <cctype>
#include <algorithm>
#include <cstdarg>
#include <eepp/core/string.hpp>
#include <eepp/core/utf.hpp>

namespace EE {

const std::size_t String::InvalidPos = StringType::npos;

Uint32 String::hash( const Uint8 * str ) {
	//! djb2
	if ( NULL != str ) {
		Uint32 hash = 5381;
		Int32 c;

		while ( ( c = *str++ ) )
			hash = ( ( hash << 5 ) + hash ) + c;

		return hash;
	}

	return 0;
}

Uint32 String::hash( const char * str ) {
	return String::hash( reinterpret_cast<const Uint8*>( str ) );
}

Uint32 String::hash( const std::string& str ) {
	return String::hash( reinterpret_cast<const Uint8*>( &str[0] ) );
}

Uint32 String::hash( const String& str ) {
	return String::hash( reinterpret_cast<const Uint8*>( &(str.mString[0]) ) );
}

bool String::isCharacter( const int& mValue ) {
	return (mValue >= 32 && mValue <= 126) || (mValue >= 161 && mValue <= 255) || (mValue == 9);
}

bool String::isNumber( const int& mValue, bool AllowDot ) {
	if ( AllowDot )
		return ( mValue >= 48 && mValue <= 57 ) || mValue == 46;

	return mValue >= 48 && mValue <= 57;
}

bool String::isLetter( const int& mValue ) {
	return ( ( (mValue >= 65 && mValue <= 90) || (mValue >= 97 && mValue <= 122) || (mValue >= 192 && mValue <= 255) ) && (mValue != 215) && (mValue != 247) );
}

std::vector < String > String::split ( const String& str, const Uint32& splitchar, const bool& pushEmptyString ) {
	std::vector < String > tmp;
	String tmpstr;

	for ( unsigned int i = 0; i < str.size(); i++ ) {
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

std::vector < std::string > String::split ( const std::string& str, const Int8& splitchar, const bool& pushEmptyString ) {
	std::vector < std::string > tmp;
	std::string tmpstr;

	for ( unsigned int i = 0; i < str.size(); i++ ) {
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

std::string String::join( const std::vector<std::string> & strArray, const Int8 & joinchar, const bool& appendLastJoinChar ) {
	size_t s = strArray.size();
	std::string str;

	if ( s > 0 ) {
		for ( size_t i = 0; i < s; i++ ) {
			str += strArray[i];

			if ( i != s - 1 || appendLastJoinChar ) {
				str += joinchar;
			}
		}
	}

	return str;
}

String String::join( const std::vector<String> & strArray, const Int8 & joinchar, const bool& appendLastJoinChar ) {
	size_t s = strArray.size();
	String str;

	if ( s > 0 ) {
		for ( size_t i = 0; i < s; i++ ) {
			str += strArray[i];

			if ( i != s - 1 || appendLastJoinChar ) {
				str += joinchar;
			}
		}
	}

	return str;
}

std::string String::lTrim(const std::string & str , char character) {
	std::string::size_type pos1 = str.find_first_not_of(character);
	return ( pos1 == std::string::npos ) ? str : str.substr( pos1 );
}

std::string String::trim(const std::string & str , char character) {
	std::string::size_type pos1 = str.find_first_not_of(character);
	std::string::size_type pos2 = str.find_last_not_of(character);
	return str.substr(pos1 == std::string::npos ? 0 : pos1, pos2 == std::string::npos ? str.length() - 1 : pos2 - pos1 + 1);
}

String String::lTrim(const String & str , char character) {
	StringType::size_type pos1 = str.find_first_not_of(character);
	return ( pos1 == String::InvalidPos ) ? str : str.substr( pos1 );
}

String String::trim(const String & str , char character) {
	StringType::size_type pos1 = str.find_first_not_of(character);
	StringType::size_type pos2 = str.find_last_not_of(character);
	return str.substr(pos1 == String::InvalidPos ? 0 : pos1, pos2 == String::InvalidPos ? str.length() - 1 : pos2 - pos1 + 1);
}

void String::toUpperInPlace( std::string & str ) {
	std::transform(str.begin(), str.end(), str.begin(), (int(*)(int)) std::toupper);
}

std::string String::toUpper( std::string str ) {
	for (std::string::iterator i = str.begin(); i != str.end(); ++i)
		*i = static_cast<char>(std::toupper(*i));
	return str;
}

void String::toLowerInPlace( std::string & str ) {
	std::transform(str.begin(), str.end(), str.begin(), (int(*)(int)) std::tolower);
}

std::string String::toLower( std::string str ) {
	for (std::string::iterator i = str.begin(); i != str.end(); ++i)
		*i = static_cast<char>(std::tolower(*i));
	return str;
}

std::vector<Uint8> String::stringToUint8( const std::string& str ) {
	return std::vector<Uint8>( str.begin(), str.end() );
}

std::string String::Uint8ToString( const std::vector<Uint8>& v ) {
	return std::string( v.begin(), v.end() );
}

void String::strCopy( char * Dst, const char * Src, unsigned int DstSize ) {
	strncpy( Dst, Src, DstSize );
}

bool String::startsWith( const std::string& haystack, const std::string & needle ) {
	return needle.length() <= haystack.length() && std::equal(needle.begin(), needle.end(), haystack.begin() );
}

bool String::startsWith( const String& haystack, const String & needle ) {
	return needle.length() <= haystack.length() && std::equal(needle.begin(), needle.end(), haystack.begin() );
}

void String::replaceAll( std::string &target, const std::string& that, const std::string& with ) {
	std::string::size_type pos=0;

	while( ( pos = target.find( that, pos ) ) != std::string::npos ) {
		target.erase( pos, that.length() );
		target.insert( pos, with );
		pos += with.length();
	}
}

void String::replace( std::string& target, const std::string& that, const std::string& with ) {
	std::size_t start_pos = target.find( that );

	if( start_pos == std::string::npos )
		return;

	target.replace( start_pos, that.length(), with );
}

std::string String::removeNumbersAtEnd( std::string txt ) {
	while ( txt.size() && txt[ txt.size() - 1 ] >= '0' && txt[ txt.size() - 1 ] <= '9' ) {
		txt.resize( txt.size() - 1 );
	}

	return txt;
}

void String::insertChar( String& str, const unsigned int& pos, const Uint32& tchar ) {
	str.insert( str.begin() + pos, tchar );
}

void String::formatBuffer( char * Buffer, int BufferSize, const char * format, ... ) {
	va_list	args;
	va_start( args, format );
#ifdef EE_COMPILER_MSVC
	_vsnprintf_s( Buffer, BufferSize, BufferSize, format, args );
#else
	vsnprintf( Buffer, BufferSize-1, format, args );
#endif
	va_end( args );
}

std::string String::format( const char * format, ... ) {
	int n, size = 256;
	std::string tstr( size, '\0' );

	va_list args;

	while (1) {
		va_start( args, format );

		n = vsnprintf( &tstr[0], size, format, args );

		if ( n > -1 && n < size ) {
			tstr.resize( n );

			va_end( args );

			return tstr;
		}

		if ( n > -1 )	// glibc 2.1
			size = n+1; // precisely what is needed
		else			// glibc 2.0
			size *= 2;	// twice the old size

		tstr.resize( size );
	}
}

String::String()
{
}

String::String(char ansiChar, const std::locale& locale)
{
	mString += Utf32::decodeAnsi(ansiChar, locale);
}

#ifndef EE_NO_WIDECHAR
String::String(wchar_t wideChar)
{
	mString += Utf32::decodeWide(wideChar);
}
#endif

String::String(StringBaseType utf32Char)
{
	mString += utf32Char;
}

String::String( const char* uf8String ) {
	if (uf8String)
	{
		std::size_t length = strlen(uf8String);

		if (length > 0)
		{
			mString.reserve(length + 1);

			Utf8::toUtf32(uf8String, uf8String + length, std::back_inserter(mString));
		}
	}
}

String::String( const std::string& utf8String ) {
	mString.reserve( utf8String.length() + 1 );

	Utf8::toUtf32( utf8String.begin(), utf8String.end(), std::back_inserter( mString ) );
}

String::String(const char* ansiString, const std::locale& locale)
{
	if (ansiString)
	{
		std::size_t length = strlen(ansiString);
		if (length > 0)
		{
			mString.reserve(length + 1);
			Utf32::fromAnsi(ansiString, ansiString + length, std::back_inserter(mString), locale);
		}
	}
}

String::String(const std::string& ansiString, const std::locale& locale)
{
	mString.reserve(ansiString.length() + 1);
	Utf32::fromAnsi(ansiString.begin(), ansiString.end(), std::back_inserter(mString), locale);
}

#ifndef EE_NO_WIDECHAR
String::String(const wchar_t* wideString)
{
	if (wideString)
	{
		std::size_t length = std::wcslen(wideString);
		if (length > 0)
		{
			mString.reserve(length + 1);
			Utf32::fromWide(wideString, wideString + length, std::back_inserter(mString));
		}
	}
}

String::String(const std::wstring& wideString)
{
	mString.reserve(wideString.length() + 1);
	Utf32::fromWide(wideString.begin(), wideString.end(), std::back_inserter(mString));
}
#endif

String::String(const StringBaseType* utf32String)
{
	if (utf32String)
		mString = utf32String;
}

String::String(const StringType& utf32String) :
mString(utf32String)
{
}

String::String(const String& str) :
mString(str.mString)
{
}

String String::fromUtf8( const std::string& utf8String )
{
	String::StringType utf32;

	utf32.reserve( utf8String.length() + 1 );

	Utf8::toUtf32( utf8String.begin(), utf8String.end(), std::back_inserter( utf32 ) );

	return String( utf32 );
}

String::operator std::string() const
{
	return toAnsiString();
}

std::string String::toAnsiString(const std::locale& locale) const
{
	// Prepare the output string
	std::string output;
	output.reserve(mString.length() + 1);

	// Convert
	Utf32::toAnsi(mString.begin(), mString.end(), std::back_inserter(output), 0, locale);

	return output;
}

#ifndef EE_NO_WIDECHAR
std::wstring String::toWideString() const
{
	// Prepare the output string
	std::wstring output;
	output.reserve(mString.length() + 1);

	// Convert
	Utf32::toWide(mString.begin(), mString.end(), std::back_inserter(output), 0);

	return output;
}
#endif

std::string String::toUtf8() const {
	// Prepare the output string
	std::string output;
	output.reserve(mString.length() + 1);

	// Convert
	Utf32::toUtf8(mString.begin(), mString.end(), std::back_inserter(output) );

	return output;
}

std::basic_string<Uint16> String::toUtf16() const {
	// Prepare the output string
	std::basic_string<Uint16> output;
	output.reserve(mString.length());

	// Convert
	Utf32::toUtf16(mString.begin(), mString.end(), std::back_inserter(output));

	return output;
}

Uint32 String::getHash() const
{
	return String::hash( *this );
}

String& String::operator =(const String& right)
{
	mString = right.mString;
	return *this;
}

String& String::operator =( const StringBaseType& right )
{
	mString = right;
	return *this;
}

String& String::operator +=(const String& right)
{
	mString += right.mString;
	return *this;
}

String& String::operator +=( const StringBaseType& right )
{
	mString += right;
	return *this;
}


String::StringBaseType String::operator [](std::size_t index) const
{
	return mString[index];
}

String::StringBaseType& String::operator [](std::size_t index)
{
	return mString[index];
}

String::StringBaseType String::at( std::size_t index ) const
{
	return mString.at( index );
}

void String::push_back( StringBaseType c )
{
	mString.push_back( c );
}

void String::swap ( String& str )
{
	mString.swap( str.mString );
}

void String::clear()
{
	mString.clear();
}

std::size_t String::size() const
{
	return mString.size();
}

std::size_t String::length() const
{
	return mString.length();
}

bool String::empty() const
{
	return mString.empty();
}

void String::erase(std::size_t position, std::size_t count)
{
	mString.erase(position, count);
}

String& String::insert(std::size_t position, const String& str)
{
	mString.insert(position, str.mString);
	return *this;
}

String& String::insert( std::size_t pos1, const String& str, std::size_t pos2, std::size_t n )
{
	mString.insert( pos1, str.mString, pos2, n );
	return *this;
}

String& String::insert ( size_t pos1, const char* s, size_t n )
{
	String tmp( s );

	mString.insert( pos1, tmp.data(), n );

	return *this;
}

String& String::insert ( size_t pos1, size_t n, char c )
{
	mString.insert( pos1, n, c );
	return *this;
}

String& String::insert ( size_t pos1, const char* s )
{
	String tmp( s );

	mString.insert( pos1, tmp.data() );

	return *this;
}

String::Iterator String::insert ( Iterator p, char c )
{
	return mString.insert( p, c );
}

void String::insert ( Iterator p, size_t n, char c )
{
	mString.insert( p, n, c );
}

const String::StringBaseType* String::c_str() const
{
	return mString.c_str();
}

const String::StringBaseType* String::data() const
{
	return mString.data();
}

String::Iterator String::begin()
{
	return mString.begin();
}

String::ConstIterator String::begin() const
{
	return mString.begin();
}

String::Iterator String::end()
{
	return mString.end();
}

String::ConstIterator String::end() const
{
	return mString.end();
}

String::ReverseIterator String::rbegin()
{
	return mString.rbegin();
}

String::ConstReverseIterator String::rbegin() const
{
	return mString.rbegin();
}

String::ReverseIterator String::rend()
{
	return mString.rend();
}

String::ConstReverseIterator String::rend() const
{
	return mString.rend();
}

void String::resize( std::size_t n, StringBaseType c )
{
	mString.resize( n, c );
}

void String::resize( std::size_t n )
{
	mString.resize( n );
}

std::size_t String::max_size() const
{
	return mString.max_size();
}

void String::reserve( size_t res_arg )
{
	mString.reserve( res_arg );
}

std::size_t String::capacity() const
{
	return mString.capacity();
}

String& String::assign ( const String& str )
{
	mString.assign( str.mString );
	return *this;
}

String& String::assign ( const String& str, size_t pos, size_t n )
{
	mString.assign( str.mString, pos, n );
	return *this;
}

String& String::assign ( const char* s, size_t n )
{
	String tmp( s );

	mString.assign( tmp.mString );

	return *this;
}

String& String::assign ( const char* s )
{
	String tmp( s );

	mString.assign( tmp.mString );

	return *this;
}

String& String::assign ( size_t n, char c )
{
	mString.assign( n, c );

	return *this;
}

String& String::append ( const String& str )
{
	mString.append( str.mString );

	return *this;
}

String& String::append ( const String& str, size_t pos, size_t n )
{
	mString.append( str.mString, pos, n );

	return *this;
}

String& String::append ( const char* s, size_t n )
{
	String tmp( s );

	mString.append( tmp.mString );

	return *this;
}

String& String::append ( const char* s )
{
	String tmp( s );

	mString.append( tmp.mString );

	return *this;
}

String& String::append ( size_t n, char c )
{
	mString.append( n, c );

	return *this;
}

String& String::append ( std::size_t n, StringBaseType c )
{
	mString.append( n, c );

	return *this;
}

String& String::replace ( size_t pos1, size_t n1, const String& str )
{
	mString.replace( pos1, n1, str.mString );

	return *this;
}

String& String::replace ( Iterator i1, Iterator i2, const String& str )
{
	mString.replace( i1, i2, str.mString );

	return *this;
}

String& String::replace ( size_t pos1, size_t n1, const String& str, size_t pos2, size_t n2 )
{
	mString.replace( pos1, n1, str.mString, pos2, n2 );

	return *this;
}

String& String::replace ( size_t pos1, size_t n1, const char* s, size_t n2 )
{
	String tmp( s );

	mString.replace( pos1, n1, tmp.data(), n2 );

	return *this;
}

String& String::replace ( Iterator i1, Iterator i2, const char* s, size_t n2 )
{
	String tmp( s );

	mString.replace( i1, i2, tmp.data(), n2 );

	return *this;
}

String& String::replace ( size_t pos1, size_t n1,   const char* s )
{
	String tmp( s );

	mString.replace( pos1, n1, tmp.mString );

	return *this;
}

String& String::replace ( Iterator i1, Iterator i2, const char* s )
{
	String tmp( s );

	mString.replace( i1, i2, tmp.mString );

	return *this;
}

String& String::replace ( size_t pos1, size_t n1,   size_t n2, char c )
{
	mString.replace( pos1, n1, n2, (StringBaseType)c );

	return *this;
}

String& String::replace ( Iterator i1, Iterator i2, size_t n2, char c )
{
	mString.replace( i1, i2, n2, (StringBaseType)c );

	return *this;
}

std::size_t String::find( const String& str, std::size_t start ) const
{
	return mString.find( str.mString, start );
}

std::size_t String::find ( const char* s, std::size_t pos, std::size_t n ) const
{
	return find( String( s ), pos );
}

std::size_t String::find ( const char* s, std::size_t pos ) const
{
	return find( String( s ), pos );
}

size_t String::find ( char c, std::size_t pos ) const
{
	return mString.find( (StringBaseType)c, pos );
}

std::size_t String::rfind ( const String& str, std::size_t pos ) const
{
	return mString.rfind( str.mString, pos );
}

std::size_t String::rfind ( const char* s, std::size_t pos, std::size_t n ) const
{
	return rfind( String( s ), pos );
}

std::size_t String::rfind ( const char* s, std::size_t pos ) const
{
	return rfind( String( s ), pos );
}

std::size_t String::rfind ( char c, std::size_t pos ) const
{
	return mString.rfind( c, pos );
}

std::size_t String::copy ( StringBaseType* s, std::size_t n, std::size_t pos ) const
{
	return mString.copy( s, n, pos );
}

String String::substr ( std::size_t pos, std::size_t n ) const
{
	return String( mString.substr( pos, n ) );
}

int String::compare ( const String& str ) const
{
	return mString.compare( str.mString );
}

int String::compare ( const char* s ) const
{
	return compare( String( s ) );
}

int String::compare ( std::size_t pos1, std::size_t n1, const String& str ) const
{
	return mString.compare( pos1, n1, str.mString );
}

int String::compare ( std::size_t pos1, std::size_t n1, const char* s) const
{
	return compare( pos1, n1, String( s ) );
}

int String::compare ( std::size_t pos1, std::size_t n1, const String& str, std::size_t pos2, std::size_t n2 ) const
{
	return mString.compare( pos1, n1, str.mString, pos2, n2 );
}

int String::compare ( std::size_t pos1, std::size_t n1, const char* s, std::size_t n2) const
{
	return compare( pos1, n1, String( s ), 0, n2 );
}

std::size_t String::find_first_of ( const String& str, std::size_t pos ) const
{
	return mString.find_first_of( str.mString, pos );
}

std::size_t String::find_first_of ( const char* s, std::size_t pos, std::size_t n ) const
{
	return find_first_of( String( s ), pos );
}

std::size_t String::find_first_of ( const char* s, std::size_t pos ) const
{
	return find_first_of( String( s ), pos );
}

std::size_t String::find_first_of ( StringBaseType c, std::size_t pos ) const
{
	return mString.find_first_of( c, pos );
}

std::size_t String::find_last_of ( const String& str, std::size_t pos ) const
{
	return mString.find_last_of( str.mString, pos );
}

std::size_t String::find_last_of ( const char* s, std::size_t pos, std::size_t n ) const
{
	return find_last_of( String( s ), pos );
}

std::size_t String::find_last_of ( const char* s, std::size_t pos ) const
{
	return find_last_of( String( s ), pos );
}

std::size_t String::find_last_of ( StringBaseType c, std::size_t pos) const
{
	return mString.find_last_of( c, pos );
}

std::size_t String::find_first_not_of ( const String& str, std::size_t pos ) const
{
	return mString.find_first_not_of( str.mString, pos );
}

std::size_t String::find_first_not_of ( const char* s, std::size_t pos, std::size_t n ) const
{
	return find_first_not_of( String( s ), pos );
}

std::size_t String::find_first_not_of ( const char* s, std::size_t pos ) const
{
	return find_first_not_of( String( s ), pos );
}

std::size_t String::find_first_not_of ( StringBaseType c, std::size_t pos ) const
{
	return mString.find_first_not_of( c, pos );
}

std::size_t String::find_last_not_of ( const String& str, std::size_t pos ) const
{
	return mString.find_last_not_of( str.mString, pos );
}

std::size_t String::find_last_not_of ( const char* s, std::size_t pos, std::size_t n ) const
{
	return find_last_not_of( String( s ), pos );
}

std::size_t String::find_last_not_of ( const char* s, std::size_t pos ) const
{
	return find_last_not_of( String( s ), pos );
}

std::size_t String::find_last_not_of ( StringBaseType c, std::size_t pos ) const
{
	return mString.find_last_not_of( c, pos );
}

bool operator ==(const String& left, const String& right)
{
	return left.mString == right.mString;
}

bool operator !=(const String& left, const String& right)
{
	return !(left == right);
}

bool operator <(const String& left, const String& right)
{
	return left.mString < right.mString;
}

bool operator >(const String& left, const String& right)
{
	return right < left;
}

bool operator <=(const String& left, const String& right)
{
	return !(right < left);
}

bool operator >=(const String& left, const String& right)
{
	return !(left < right);
}

String operator +(const String& left, const String& right)
{
	String string = left;
	string += right;

	return string;
}

}
