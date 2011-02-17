#ifndef EE_UTILSCSTRING_H
#define EE_UTILSCSTRING_H

#include "base.hpp"

namespace EE { namespace Utils {

/** @return If the value passed is a character */
bool EE_API isCharacter( const eeInt& mValue );

/** @return If the value passed is a number */
bool EE_API isNumber( const eeInt& mValue, bool AllowDot = false );

/** @return If the value passed is a letter */
bool EE_API isLetter( const eeInt& mValue );

/** Converts from std::string to std::wstring */
std::wstring EE_API stringTowstring(const std::string& s);

/** Converts from std::wstring to std::string */
std::string EE_API wstringTostring(const std::wstring& s);

/** Converts an integer value to std::string. It's faster than toStr. */
std::string EE_API intToStr(Int32 n);

/** Converts an float value to std::string. It's faster than toStr. */
std::string EE_API floatToStr(eeFloat n);

/** Returns the current date time */
std::string EE_API GetDateTimeStr();

/** Converts from any basic type to std::string */
template <class T>
std::string toStr(const T& i) {
	std::ostringstream ss;
	ss << i;
	return ss.str();
}

/** Converts from any basic type to std::wstring */
template <class T>
std::wstring toWStr(const T& i) {
	std::string str;
	str = toStr(i);
	return stringTowstring(str);
}

/** Converts from a string to type */
template <class T>
bool fromString(T& t, const std::string& s, std::ios_base& (*f)(std::ios_base&) = std::dec  ) {
	std::istringstream iss(s);
	return !(iss >> f >> t).fail();
}

/** Converts from a wstring to type */
template <class T>
bool fromWString(T& t, const std::wstring& s, std::ios_base& (*f)(std::ios_base&) = std::dec ) {
	std::istringstream iss( wstringTostring( s ) );
	return !(iss >> f >> t).fail();
}

/** Split a wstring and hold it on a vector */
std::vector < std::wstring > EE_API SplitString ( const std::wstring& str, const Uint32& splitchar = L'\n' );

/** Split a string and hold it on a vector */
std::vector < std::string > EE_API SplitString ( const std::string& str, const Int8& splitchar = '\n' );

/** Returning a std::string from a formated string */
std::string EE_API StrFormated( const char* format, ... )
#ifdef __GNUC__
    /* This attribute is nice: it even works through gettext invokation. For
       example, gcc will complain that StrFormat(_("%s"), 42) is ill-formed. */
    __attribute__((format(printf, 1, 2)))
#endif
;

/** Format a char buffer */
void EE_API StrFormat( char * Buffer, int BufferSize, const char * format, ... );

/** Remove the first space on the string */
std::string EE_API LTrim( const std::string & str );

/** Removes all spaces on the string */
std::string EE_API Trim( const std::string & str );

/** Convert the string into upper case string */
void EE_API toUpper( std::string & str );

/** Convert the string into lower case string */
void EE_API toLower( std::string & str );

/** Convert the string to an std::vector<Uint8> */
std::vector<Uint8> EE_API stringToUint8( const std::string& str );

/** Convert the wstring to an std::vector<Uint8> */
std::vector<Uint8> EE_API wstringToUint8( const std::wstring& str );

/** Convert the std::vector<Uint8> to an string */
std::string EE_API Uint8Tostring( const std::vector<Uint8> v );

/** Convert the std::vector<Uint8> to an wstring */
std::wstring EE_API Uint8Towstring( const std::vector<Uint8> v );

/** Insert a char into wstr on pos (added this function to avoid a bug on wstring) */
void EE_API InsertChar( std::wstring& str, const eeUint& pos, const Uint32& tchar );

/** @return A storage path for config files for every platform */
std::string EE_API StoragePath( std::string appname );

/** Copy a string to another
* @param Dst Destination String
* @param Src Source String
* @param DstSize Destination Size
*/
void EE_API StrCopy( char * Dst, const char * Src, eeUint DstSize );

/** Compare two strings from its beginning.
* @param Start String start
* @param Str String to compare
* @return The position of the last char compared ( -1 if fails )
*/
Int32 EE_API StrStartsWith( const std::string& Start, const std::string Str );

Int32 EE_API StrStartsWith( const std::wstring& Start, const std::wstring Str );

/** Replaces a substring by another string inside a string */
void EE_API ReplaceSubStr(std::string &target, const std::string& that, const std::string& with );

}}

#endif
