////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2007-2009 Laurent Gomila (laurent.gom@gmail.com)
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it freely,
// subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented;
//    you must not claim that you wrote the original software.
//    If you use this software in a product, an acknowledgment
//    in the product documentation would be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such,
//    and must not be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.
//
////////////////////////////////////////////////////////////
/** NOTE:
** The class was modified to fit EEPP own needs. This is not the original implementation from SFML2.
** Functions and methods are the same that in std::string to facilitate portability.
** Also added a lot of utilities for string manipulation
**/

#ifndef EE_STRING_HPP
#define EE_STRING_HPP

#include <eepp/config.hpp>
#include <eepp/core/utf.hpp>
#include <locale>
#include <string>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>

namespace EE {

/** @brief Utility string class that automatically handles conversions between types and encodings **/
class EE_API String {
	public :
	typedef Uint32								StringBaseType;
	typedef std::basic_string<StringBaseType>	StringType;
	typedef StringType::iterator				Iterator;				//! Iterator type
	typedef StringType::const_iterator			ConstIterator;			//! Constant iterator type
	typedef StringType::reverse_iterator		ReverseIterator;		//! Reverse Iterator type
	typedef StringType::const_reverse_iterator	ConstReverseIterator;	//! Constant iterator type

	static const std::size_t InvalidPos; ///< Represents an invalid position in the string

	/** @return string hash */
	static Uint32 hash( const Uint8 * str );

	/** @return string hash */
	static Uint32 hash( const char * str );

	/** @return string hash */
	static Uint32 hash( const std::string& str );

	/** @return string hash. Note: String::hash( std::string( "text" ) ) is != to String::hash( String( "text" ) ) */
	static Uint32 hash( const String& str );

	/** @return If the value passed is a character */
	static bool isCharacter( const int& mValue );

	/** @return If the value passed is a number */
	static bool isNumber( const int& mValue, bool AllowDot = false );

	/** @return If the value passed is a letter */
	static bool isLetter( const int& mValue );

	/** Split a String and hold it on a vector */
	static std::vector < String > split( const String& str, const Uint32& splitchar = '\n', const bool& pushEmptyString = false );

	/** Split a string and hold it on a vector */
	static std::vector < std::string > split( const std::string& str, const Int8& splitchar = '\n', const bool& pushEmptyString = false );

	/** Joins a string vector into a single string */
	static std::string join( const std::vector<std::string>& strArray, const Int8& joinchar = ' ', const bool& appendLastJoinChar = false );

	/** Joins a string vector into a single string */
	static String join( const std::vector<String>& strArray, const Int8& joinchar = ' ', const bool& appendLastJoinChar = false );

	/** Remove the first space ( or the specified character ) on the string */
	static std::string lTrim( const std::string & str, char character = ' ' );

	/** Removes all spaces ( or the specified character ) on the string */
	static std::string trim( const std::string & str, char character = ' ' );

	/** Remove the first space ( or the specified character ) on the string */
	static String lTrim(const String & str, char character = ' ' );

	/** Removes all spaces ( or the specified character ) on the string */
	static String trim( const String & str, char character = ' ' );

	/** Convert the string into upper case string */
	static void toUpperInPlace( std::string & str );

	/** Convert a string to lower case */
	static std::string toUpper( std::string str );

	/** Convert the reference of a string into lower case string */
	static void toLowerInPlace( std::string & str );

	/** Convert a string to lower case */
	static std::string toLower( std::string str );

	/** Convert the string to an std::vector<Uint8> */
	static std::vector<Uint8> stringToUint8( const std::string& str );

	/** Convert the std::vector<Uint8> to an string */
	static std::string Uint8ToString( const std::vector<Uint8> & v );

	/** Insert a char into String on pos (added this function to avoid a bug on String) */
	static void insertChar( String& str, const unsigned int& pos, const Uint32& tchar );

	/** Copy a string to another
	* @param Dst Destination String
	* @param Src Source String
	* @param DstSize Destination Size
	*/
	static void strCopy( char * Dst, const char * Src, unsigned int DstSize );

	/** Compare two strings from its beginning.
	* @param haystack The string to search in.
	* @param needle The searched string.
	* @return true if string starts with the substring
	*/
	static bool startsWith( const std::string& haystack, const std::string& needle );

	/** Compare two strings from its beginning.
	* @param haystack The string to search in.
	* @param needle The searched string.
	* @return true if string starts with the substring
	*/
	static bool startsWith( const String& haystack, const String& needle );

	/** Replace all occurrences of the search string with the replacement string. */
	static void replaceAll( std::string &target, const std::string& that, const std::string& with );

	/** Replace the first ocurrence of the search string with the replacement string. */
	static void replace( std::string& target, const std::string& that, const std::string& with );

	/** Removes the numbers at the end of the string */
	static std::string removeNumbersAtEnd( std::string txt );

	/** Converts from any basic type to std::string */
	template <class T>
	static std::string toStr(const T& i) {
		std::ostringstream ss;
		ss << i;
		return ss.str();
	}

	/** Converts from a string to type */
	template <class T>
	static bool fromString(T& t, const std::string& s, std::ios_base& (*f)(std::ios_base&) = std::dec  ) {
		std::istringstream iss(s);
		return !(iss >> f >> t).fail();
	}

	/** Converts from a String to type */
	template <class T>
	static bool fromString(T& t, const String& s, std::ios_base& (*f)(std::ios_base&) = std::dec ) {
		std::istringstream iss( s.toUtf8() );
		return !(iss >> f >> t).fail();
	}

	/** Returning a std::string from a formated string */
	static std::string format( const char* format, ... )
	#ifdef __GNUC__
		/* This attribute is nice: it even works through gettext invokation. For
		   example, gcc will complain that StrFormat(_("%s"), 42) is ill-formed. */
		__attribute__((format(printf, 1, 2)))
	#endif
	;

	/** Format a char buffer */
	static void formatBuffer( char * Buffer, int BufferSize, const char * format, ... );

	/** @brief Construct from an UTF-8 string to UTF-32 according
	** @param utf8String UTF-8 string to convert
	**/
	static String fromUtf8( const std::string& utf8String );

	/** @brief Default constructor
	** This constructor creates an empty string.
	**/
	String();

	/** @brief Construct from a single ANSI character and a locale
	** The source character is converted to UTF-32 according
	** to the given locale. If you want to use the current global
	** locale, rather use the other constructor.
	** @param ansiChar ANSI character to convert
	** @param locale   Locale to use for conversion
	**/
	String( char ansiChar, const std::locale& locale = std::locale() );

#ifndef EE_NO_WIDECHAR
	/** @brief Construct from single wide character
	** @param wideChar Wide character to convert
	**/
	String( wchar_t wideChar );
#endif

	/** @brief Construct from single UTF-32 character
	** @param utf32Char UTF-32 character to convert
	**/
	String( StringBaseType utf32Char );

	/** @brief Construct from an from a null-terminated C-style UTF-8 string to UTF-32
	** @param uf8String UTF-8 string to convert
	**/
	String( const char* uf8String );

	/** @brief Construct from an UTF-8 string to UTF-32 according
	** @param utf8String UTF-8 string to convert
	**/
	String( const std::string& utf8String );

	/** @brief Construct from a null-terminated C-style ANSI string and a locale
	** The source string is converted to UTF-32 according
	** to the given locale. If you want to use the current global
	** locale, rather use the other constructor.
	** @param ansiString ANSI string to convert
	** @param locale     Locale to use for conversion
	**/
	String( const char* ansiString, const std::locale& locale );

	/** @brief Construct from an ANSI string and a locale
	** The source string is converted to UTF-32 according
	** to the given locale. If you want to use the current global
	** locale, rather use the other constructor.
	** @param ansiString ANSI string to convert
	** @param locale     Locale to use for conversion
	**/
	String( const std::string& ansiString, const std::locale& locale );

#ifndef EE_NO_WIDECHAR
	/** @brief Construct from null-terminated C-style wide string
	** @param wideString Wide string to convert
	**/
	String( const wchar_t* wideString );

	/** @brief Construct from a wide string
	** @param wideString Wide string to convert
	**/
	String( const std::wstring& wideString );
#endif

	/** @brief Construct from a null-terminated C-style UTF-32 string
	** @param utf32String UTF-32 string to assign
	**/
	String( const StringBaseType* utf32String );

	/** @brief Construct from an UTF-32 string
	** @param utf32String UTF-32 string to assign
	**/
	String( const StringType& utf32String );


	/** @brief Copy constructor
	** @param str Instance to copy
	**/
	String( const String& str );

	/** @brief Create a new String from a UTF-8 encoded string
	**  @param begin Forward iterator to the begining of the UTF-8 sequence
	**  @param end   Forward iterator to the end of the UTF-8 sequence
	**  @return A String containing the source string
	**  @see FromUtf16, FromUtf32 */
	template <typename T>
	static String fromUtf8(T begin, T end) {
		String string;
		Utf8::toUtf32(begin, end, std::back_inserter(string.mString));
		return string;
	}

	/** @brief Create a new String from a UTF-16 encoded string
	**  @param begin Forward iterator to the begining of the UTF-16 sequence
	**  @param end   Forward iterator to the end of the UTF-16 sequence
	**  @return A String containing the source string
	**  @see FromUtf8, FromUtf32 */
	template <typename T>
	static String fromUtf16(T begin, T end) {
		String string;
		Utf16::toUtf32(begin, end, std::back_inserter(string.mString));
		return string;
	}

	/** @brief Create a new String from a UTF-32 encoded string
	**  @param begin Forward iterator to the begining of the UTF-32 sequence
	**  @param end   Forward iterator to the end of the UTF-32 sequence
	**  @return A String containing the source string
	**  @see FromUtf8, FromUtf32 */
	template <typename T>
	static String fromUtf32(T begin, T end) {
		String string;
		Utf32::toUtf32(begin, end, std::back_inserter(string.mString));
		return string;
	}

	/** @brief Implicit cast operator to std::string (ANSI string)
	** The current global locale is used for conversion. If you
	** want to explicitely specify a locale, see toAnsiString.
	** Characters that do not fit in the target encoding are
	** discarded from the returned string.
	** This operator is defined for convenience, and is equivalent
	** to calling toAnsiString().
	** @return Converted ANSI string
	** @see toAnsiString, operator String
	**/
	operator std::string() const;

	/** @brief Convert the unicode string to an ANSI string
	** The UTF-32 string is converted to an ANSI string in
	** the encoding defined by \a locale. If you want to use
	** the current global locale, see the other overload
	** of toAnsiString.
	** Characters that do not fit in the target encoding are
	** discarded from the returned string.
	** @param locale Locale to use for conversion
	** @return Converted ANSI string
	** @see toWideString, operator std::string
	**/
	std::string toAnsiString( const std::locale& locale = std::locale() ) const;

#ifndef EE_NO_WIDECHAR
	/** @brief Convert the unicode string to a wide string
	** Characters that do not fit in the target encoding are
	** discarded from the returned string.
	** @return Converted wide string
	** @see toAnsiString, operator String
	**/
	std::wstring toWideString() const;
#endif

	/** Convert the string to a UTF-8 string */
	std::string toUtf8() const;

	/** Convert the string to a UTF-16 string */
	std::basic_string<Uint16> toUtf16() const;

	/** @return The hash code of the String */
	Uint32 getHash() const;

	/** @brief Overload of assignment operator
	** @param right Instance to assign
	** @return Reference to self
	**/
	String& operator =(const String& right);

	String& operator =( const StringBaseType& right );

	/** @brief Overload of += operator to append an UTF-32 string
	** @param right String to append
	** @return Reference to self
	**/
	String& operator +=(const String& right);

	String& operator +=( const StringBaseType& right );

	/** @brief Overload of [] operator to access a character by its position
	** This function provides read-only access to characters.
	** Note: this function doesn't throw if \a index is out of range.
	** @param index Index of the character to get
	** @return Character at position \a index
	**/
	StringBaseType operator [](std::size_t index) const;

	/** @brief Overload of [] operator to access a character by its position
	** This function provides read and write access to characters.
	** Note: this function doesn't throw if \a index is out of range.
	** @param index Index of the character to get
	** @return Reference to the character at position \a index
	**/

	StringBaseType& operator [](std::size_t index);

	/** @brief Get character in string
	** Performs a range check, throwing an exception of type out_of_range in case that pos is not an actual position in the string.
	** @return The character at position pos in the string.
	*/
	StringBaseType at( std::size_t index ) const;
	
	/** @brief clear the string
	** This function removes all the characters from the string.
	** @see empty, erase
	**/
	void clear();

	/** @brief Get the size of the string
	** @return Number of characters in the string
	** @see empty
	**/
	std::size_t size() const;
	
	/** @see size() */
	std::size_t length() const;

	/** @brief Check whether the string is empty or not
	** @return True if the string is empty (i.e. contains no character)
	** @see clear, size
	**/
	bool empty() const;

	/** @brief Erase one or more characters from the string
	** This function removes a sequence of \a count characters
	** starting from \a position.
	** @param position Position of the first character to erase
	** @param count    Number of characters to erase
	**/
	void erase(std::size_t position, std::size_t count = 1);


	/** @brief Insert one or more characters into the string
	** This function inserts the characters of \a str
	** into the string, starting from \a position.
	** @param position Position of insertion
	** @param str      Characters to insert
	**/
	String& insert(std::size_t position, const String& str);

	String& insert( std::size_t pos1, const String& str, std::size_t pos2, std::size_t n );

	String& insert ( std::size_t pos1, const char* s, std::size_t n );

	String& insert ( std::size_t pos1, const char* s );

	String& insert ( std::size_t pos1, size_t n, char c );

	Iterator insert ( Iterator p, char c );

	void insert ( Iterator p, std::size_t n, char c );

	template<class InputIterator>
	void insert ( Iterator p, InputIterator first, InputIterator last )
	{
		mString.insert( p, first, last );
	}

	/** @brief Find a sequence of one or more characters in the string
	** This function searches for the characters of \a str
	** into the string, starting from \a start.
	** @param str   Characters to find
	** @param start Where to begin searching
	** @return Position of \a str in the string, or String::InvalidPos if not found
	**/
	std::size_t find( const String& str, std::size_t start = 0 ) const;

	std::size_t find ( const char* s, std::size_t pos, std::size_t n ) const;

	std::size_t find ( const char* s, std::size_t pos = 0 ) const;

	std::size_t find ( char c, std::size_t pos = 0 ) const;

	/** @brief Get a pointer to the C-style array of characters
	** This functions provides a read-only access to a
	** null-terminated C-style representation of the string.
	** The returned pointer is temporary and is meant only for
	** immediate use, thus it is not recommended to store it.
	** @return Read-only pointer to the array of characters
	**/
	const StringBaseType* c_str() const;
	
	/** @brief Get string data
	** Notice that no terminating null character is appended (see member c_str for such a functionality).
	** The returned array points to an internal location which should not be modified directly in the program.
	** Its contents are guaranteed to remain unchanged only until the next call to a non-constant member function of the string object.
	** @return Pointer to an internal array containing the same content as the string.
	**/
	const StringBaseType* data() const;

	/** @brief Return an iterator to the beginning of the string
	** @return Read-write iterator to the beginning of the string characters
	** @see end
	**/
	Iterator begin();

	/** @brief Return an iterator to the beginning of the string
	** @return Read-only iterator to the beginning of the string characters
	** @see end
	**/
	ConstIterator begin() const;

	/** @brief Return an iterator to the beginning of the string
	** The end iterator refers to 1 position past the last character;
	** thus it represents an invalid character and should never be
	** accessed.
	** @return Read-write iterator to the end of the string characters
	** @see begin
	**/
	Iterator end();

	/** @brief Return an iterator to the beginning of the string
	** The end iterator refers to 1 position past the last character;
	** thus it represents an invalid character and should never be
	** accessed.
	** @return Read-only iterator to the end of the string characters
	** @see begin
	**/
	ConstIterator end() const;
	
	/** @brief Return an reverse iterator to the beginning of the string
	** @return Read-write reverse iterator to the beginning of the string characters
	** @see end
	**/
	ReverseIterator rbegin();

	/** @brief Return an reverse iterator to the beginning of the string
	** @return Read-only reverse iterator to the beginning of the string characters
	** @see end
	**/
	ConstReverseIterator rbegin() const;

	/** @brief Return an reverse iterator to the beginning of the string
	** The end reverse iterator refers to 1 position past the last character;
	** thus it represents an invalid character and should never be
	** accessed.
	** @return Read-write reverse iterator to the end of the string characters
	** @see begin
	**/
	ReverseIterator rend();

	/** @brief Return an reverse iterator to the beginning of the string
	** The end reverse iterator refers to 1 position past the last character;
	** thus it represents an invalid character and should never be
	** accessed.
	** @return Read-only reverse iterator to the end of the string characters
	** @see begin
	**/
	ConstReverseIterator rend() const;
	
	/** @brief Resize String */
	void resize ( std::size_t n, StringBaseType c );
	
	/** @brief Resize String */
	void resize ( std::size_t n );
	
	/** @return Maximum size of string */
	std::size_t max_size() const;
	
	/** @brief Request a change in capacity */
	void reserve ( size_t res_arg=0 );
	
	/** @return Size of allocated storage */
	std::size_t capacity() const;

	/** @brief Append character to string */
	void push_back( StringBaseType c );
	
	/** @brief Swap contents with another string */
	void swap ( String& str );
	
	String& assign ( const String& str );

	String& assign ( const String& str, std::size_t pos, std::size_t n );

	String& assign ( const char* s, std::size_t n );

	String& assign ( const char* s );

	String& assign ( std::size_t n, char c );

	template <class InputIterator>
	String& assign ( InputIterator first, InputIterator last )
	{
		mString.assign( first, last );
		return *this;
	}

	String& append ( const String& str );

	String& append ( const String& str, std::size_t pos, std::size_t n );

	String& append ( const char* s, std::size_t n );

	String& append ( const char* s );

	String& append ( std::size_t n, char c );

	String& append ( std::size_t n, StringBaseType c );

	template <class InputIterator>
	String& append ( InputIterator first, InputIterator last )
	{
		mString.append( first, last );
		return *this;
	}

	String& replace ( std::size_t pos1, std::size_t n1,   const String& str );

	String& replace ( Iterator i1, Iterator i2, const String& str );

	String& replace ( std::size_t pos1, std::size_t n1, const String& str, std::size_t pos2, std::size_t n2 );

	String& replace ( std::size_t pos1, std::size_t n1, const char* s, std::size_t n2 );

	String& replace ( Iterator i1, Iterator i2, const char* s, std::size_t n2 );

	String& replace ( std::size_t pos1, std::size_t n1,   const char* s );

	String& replace ( Iterator i1, Iterator i2, const char* s );

	String& replace ( std::size_t pos1, std::size_t n1, std::size_t n2, char c );

	String& replace ( Iterator i1, Iterator i2, std::size_t n2, char c );

	template<class InputIterator>
	String& replace ( Iterator i1, Iterator i2, InputIterator j1, InputIterator j2 )
	{
		mString.replace( i1, i2, j1, j2 );
		return *this;
	}

	std::size_t rfind ( const String& str, std::size_t pos = StringType::npos ) const;

	std::size_t rfind ( const char* s, std::size_t pos, std::size_t n ) const;

	std::size_t rfind ( const char* s, std::size_t pos = StringType::npos ) const;

	std::size_t rfind ( char c, std::size_t pos = StringType::npos ) const;

	String substr ( std::size_t pos = 0, std::size_t n = StringType::npos ) const;

	std::size_t copy ( StringBaseType* s, std::size_t n, std::size_t pos = 0 ) const;

	int compare ( const String& str ) const;

	int compare ( const char* s ) const;

	int compare ( std::size_t pos1, std::size_t n1, const String& str ) const;

	int compare ( std::size_t pos1, std::size_t n1, const char* s) const;

	int compare ( std::size_t pos1, std::size_t n1, const String& str, std::size_t pos2, std::size_t n2 ) const;

	int compare ( std::size_t pos1, std::size_t n1, const char* s, std::size_t n2) const;

	std::size_t find_first_of ( const String& str, std::size_t pos = 0 ) const;

	std::size_t find_first_of ( const char* s, std::size_t pos, std::size_t n ) const;

	std::size_t find_first_of ( const char* s, std::size_t pos = 0 ) const;

	std::size_t find_first_of ( StringBaseType c, std::size_t pos = 0 ) const;

	std::size_t find_last_of ( const String& str, std::size_t pos = StringType::npos ) const;

	std::size_t find_last_of ( const char* s, std::size_t pos, std::size_t n ) const;

	std::size_t find_last_of ( const char* s, std::size_t pos = StringType::npos ) const;

	std::size_t find_last_of ( StringBaseType c, std::size_t pos = StringType::npos ) const;

	std::size_t find_first_not_of ( const String& str, std::size_t pos = 0 ) const;

	std::size_t find_first_not_of ( const char* s, std::size_t pos, std::size_t n ) const;

	std::size_t find_first_not_of ( const char* s, std::size_t pos = 0 ) const;

	std::size_t find_first_not_of ( StringBaseType c, std::size_t pos = 0 ) const;

	std::size_t find_last_not_of ( const String& str, std::size_t pos = StringType::npos ) const;

	std::size_t find_last_not_of ( const char* s, std::size_t pos, std::size_t n ) const;

	std::size_t find_last_not_of ( const char* s, std::size_t pos = StringType::npos ) const;

	std::size_t find_last_not_of ( StringBaseType c, std::size_t pos = StringType::npos ) const;
private :
    friend EE_API bool operator ==(const String& left, const String& right);
    friend EE_API bool operator <(const String& left, const String& right);

	StringType mString; ///< Internal string of UTF-32 characters
};

/** @relates String
** @brief Overload of == operator to compare two UTF-32 strings
** @param left  Left operand (a string)
** @param right Right operand (a string)
** @return True if both strings are equal
**/
EE_API bool operator ==(const String& left, const String& right);

/** @relates String
** @brief Overload of != operator to compare two UTF-32 strings
** @param left  Left operand (a string)
** @param right Right operand (a string)
** @return True if both strings are different
**/
EE_API bool operator !=(const String& left, const String& right);

/** @relates String
** @brief Overload of < operator to compare two UTF-32 strings
** @param left  Left operand (a string)
** @param right Right operand (a string)
** @return True if \a left is alphabetically lesser than \a right
**/
EE_API bool operator <(const String& left, const String& right);

/** @relates String
** @brief Overload of > operator to compare two UTF-32 strings
** @param left  Left operand (a string)
** @param right Right operand (a string)
** @return True if \a left is alphabetically greater than \a right
**/
EE_API bool operator >(const String& left, const String& right);

/** @relates String
** @brief Overload of <= operator to compare two UTF-32 strings
** @param left  Left operand (a string)
** @param right Right operand (a string)
** @return True if \a left is alphabetically lesser or equal than \a right
**/
EE_API bool operator <=(const String& left, const String& right);

/** @relates String
** @brief Overload of >= operator to compare two UTF-32 strings
** @param left  Left operand (a string)
** @param right Right operand (a string)
** @return True if \a left is alphabetically greater or equal than \a right
**/
EE_API bool operator >=(const String& left, const String& right);

/** @relates String
** @brief Overload of binary + operator to concatenate two strings
** @param left  Left operand (a string)
** @param right Right operand (a string)
** @return Concatenated string
**/
EE_API String operator +( const String& left, const String& right );

}

#endif

/**
@class EE::String
EE::String is a utility string class defined mainly for
convenience. It is a Unicode string (implemented using
UTF-32), thus it can store any character in the world
(european, chinese, arabic, hebrew, etc.).
It automatically handles conversions from/to ANSI and
wide strings, so that you can work with standard string
classes and still be compatible with functions taking a
EE::String.
@code
EE::String s;
std::string s1 = s;  // automatically converted to ANSI string
String s2 = s; // automatically converted to wide string
s = "hello";         // automatically converted from ANSI string
s = L"hello";        // automatically converted from wide string
s += 'a';            // automatically converted from ANSI string
s += L'a';           // automatically converted from wide string
@endcode
Conversions involving ANSI strings use the default user locale. However
it is possible to use a custom locale if necessary:
@code
std::locale locale;
EE::String s;
...
std::string s1 = s.toAnsiString(locale);
s = EE::String("hello", locale);
@endcode

EE::String defines the most important functions of the
standard std::string class: removing, random access, iterating,
appending, comparing, etc. However it is a simple class
provided for convenience, and you may have to consider using
a more optimized class if your program requires complex string
handling. The automatic conversion functions will then take
care of converting your string to EE::String whenever EE
requires it.

Please note that EE also defines a low-level, generic
interface for Unicode handling, see the EE::Utf classes.

All credits to Laurent Gomila, i just modified and expanded a little bit the implementation.
*/
