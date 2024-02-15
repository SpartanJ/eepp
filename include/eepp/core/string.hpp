#ifndef EE_STRING_HPP
#define EE_STRING_HPP

#include <cstdlib>
#include <cstring>
#include <eepp/config.hpp>
#include <eepp/core/utf.hpp>
#include <locale>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace EE {

template <typename T> struct FormatArg {
	static const T& get( const T& arg ) { return arg; }
};

template <> struct FormatArg<std::string> {
	static const char* get( const std::string& arg ) { return arg.c_str(); }
};

template <> struct FormatArg<std::string_view> {
	static const char* get( const std::string_view& arg ) { return arg.data(); }
};

/*
** The class was modified to fit EEPP own needs. This is not the original implementation from SFML2.
** Functions and methods are the same that in std::string to facilitate portability.
** Also added a lot of utilities for string manipulation
**/

/** Utility string class that automatically handles conversions between types and encodings **/
class EE_API String {
  public:
	typedef char32_t StringBaseType;
	typedef std::basic_string<StringBaseType> StringType;
	typedef StringType::iterator Iterator;							 //! Iterator type
	typedef StringType::const_iterator ConstIterator;				 //! Constant iterator type
	typedef StringType::reverse_iterator ReverseIterator;			 //! Reverse Iterator type
	typedef StringType::const_reverse_iterator ConstReverseIterator; //! Constant iterator type
	typedef Uint32 HashType;
	typedef std::basic_string_view<StringBaseType> View;

	static const std::size_t InvalidPos; ///< Represents an invalid position in the string

	/** Boyer–Moore–Horspool fast string search. */
	class EE_API BMH {
	  public:
		typedef std::vector<size_t> OccTable;

		static const OccTable createOccTable( const unsigned char* needle, size_t needleLength );

		/** @returns haystackLength if not found, otherwise the position */
		static size_t search( const unsigned char* haystack, size_t haystackLength,
							  const unsigned char* needle, const size_t needleLength,
							  const OccTable& occ );

		/** @returns -1 if not found otherwise the position */
		static Int64 find( const std::string& haystack, const std::string& needle,
						   const size_t& haystackOffset, const OccTable& occ );

		/** @returns -1 if not found otherwise the position */
		static Int64 find( const std::string& haystack, const std::string& needle,
						   const size_t& haystackOffset = 0 );
	};

	/** @return string hash */
	static constexpr HashType hash( const char* str ) {
		//! djb2
		if ( NULL != str ) {
			Uint32 hash = 5381;
			Int32 c = 0;

			while ( ( c = *str++ ) )
				hash = ( ( hash << 5 ) + hash ) + c;

			return hash;
		}

		return 0;
	}

	static constexpr String::HashType hash( const char* str, Int64 len ) {
		String::HashType hash = 5381;
		while ( --len >= 0 )
			hash = ( ( hash << 5 ) + hash ) + *str++;
		return hash;
	}

	/** Escape string sequence */
	static String escape( const String& str );

	/** Unescape string sequence */
	static String unescape( const String& str );

	/** @return string hash */
	static String::HashType hash( const std::string& str );

	/** @return string hash. Note: String::hash( std::string( "text" ) ) is != to String::hash(
	 * String( "text" ) ) */
	static String::HashType hash( const String& str );

	/** @return If the value passed is a character */
	static bool isCharacter( const int& value );

	/** @return If the value passed is a number */
	static bool isNumber( const int& value, bool AllowDot = false );

	/** @return If the string represents a number. */
	static bool isNumber( const std::string& value, bool AllowDot = false );

	/** @return If the value passed is a letter */
	static bool isLetter( const int& value );

	/** @return If the value passed is a letter or a number */
	static bool isAlphaNum( const int& value );

	/** @return If the string is a representation of a hexa number */
	static bool isHexNotation( const std::string& value, const std::string& withPrefix = "" );

	/** @return If the needle substring, found starting at startPos is a whole-word. */
	static bool isWholeWord( const std::string& haystack, const std::string& needle,
							 const Int64& startPos );

	/** @return If the needle substring, found starting at startPos is a whole-word. */
	static bool isWholeWord( const String& haystack, const String& needle, const Int64& startPos );

	/** Split a String and hold it on a vector */
	static std::vector<String> split( const String& str, const StringBaseType& delim = '\n',
									  const bool& pushEmptyString = false,
									  const bool& keepDelim = false );

	/** Split a string and hold it on a vector */
	static std::vector<std::string> split( const std::string& str, const Int8& delim = '\n',
										   const bool& pushEmptyString = false,
										   const bool& keepDelim = false );

	/** Split a string and hold it on a vector */
	static std::vector<std::string_view> split( const std::string_view& str,
												const Int8& delim = '\n',
												const bool& pushEmptyString = false );

	/** Split a string and hold it on a vector. This function is meant to be used for code
	 * splitting, detects functions, arrays, braces and quotes for the splitting. */
	static std::vector<std::string> split( const std::string& str, const std::string& delims,
										   const std::string& delimsPreserve = "",
										   const std::string& quote = "\"",
										   const bool& removeQuotes = false );

	/** Joins a string vector into a single string */
	static std::string join( const std::vector<std::string>& strArray, const Int8& joinchar = ' ',
							 const bool& appendLastJoinChar = false );

	/** Joins a string vector into a single string */
	static String join( const std::vector<String>& strArray, const Int8& joinchar = ' ',
						const bool& appendLastJoinChar = false );

	/** Removes the trailing prefix. */
	static std::string lTrim( const std::string& str, char character = ' ' );

	/** Removes the trailing suffix. */
	static std::string rTrim( const std::string& str, char character );

	/** Removes all spaces ( or the specified character ) on the string */
	static std::string trim( const std::string& str, char character = ' ' );

	/** Removes the trailing prefix. */
	static std::string_view lTrim( const std::string_view& str, char character = ' ' );

	/** Removes the trailing suffix. */
	static std::string_view rTrim( const std::string_view& str, char character );

	/** Removes all spaces ( or the specified character ) on the string */
	static std::string_view trim( const std::string_view& str, char character = ' ' );

	/** Removes all spaces ( or the specified character ) on the string */
	static void trimInPlace( std::string& str, char character = ' ' );

	/** Removes the trailing prefix. */
	static String lTrim( const String& str, char character = ' ' );

	/** Removes the trailing suffix. */
	static String rTrim( const String& str, char character = ' ' );

	/** Removes all spaces ( or the specified character ) on the string */
	static String trim( const String& str, char character = ' ' );

	/** Removes all spaces ( or the specified character ) on the string */
	static void trimInPlace( String& str, char character = ' ' );

	/** Convert the string into upper case string */
	static void toUpperInPlace( std::string& str );

	/** Convert a string to lower case */
	static std::string toUpper( std::string str );

	/** Convert the reference of a string into lower case string */
	static void toLowerInPlace( std::string& str );

	/** Capitalizes the reference of a string */
	static void capitalizeInPlace( std::string& str );

	/** Convert a string to lower case */
	static std::string toLower( std::string str );

	/** Catitalize a string */
	static std::string capitalize( std::string str );

	/** Convert a string to lower case */
	static String toUpper( const String& str );

	/** Convert a string to lower case */
	static String toLower( const String& str );

	/** Capitalizes a string */
	static String capitalize( const String& str );

	/** Convert the string to an std::vector<Uint8> */
	static std::vector<Uint8> stringToUint8( const std::string& str );

	/** Convert the std::vector<Uint8> to an string */
	static std::string Uint8ToString( const std::vector<Uint8>& v );

	/** Insert a char into String on pos (added this function to avoid a bug on String) */
	static void insertChar( String& str, const unsigned int& pos, const StringBaseType& tchar );

	/** Copy a string to another
	 * @param Dst Destination String
	 * @param Src Source String
	 * @param DstSize Destination Size
	 */
	static void strCopy( char* Dst, const char* Src, unsigned int DstSize );

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

	/** Compare two strings from its beginning.
	 * @param haystack The string to search in.
	 * @param needle The searched string.
	 * @return true if string starts with the substring
	 */
	static bool startsWith( const char* haystack, const char* needle );

	/** Compare two strings from its end.
	 * @param haystack The string to search in.
	 * @param needle The searched string.
	 * @return true if string starts with the substring
	 */
	static bool endsWith( const std::string& haystack, const std::string& needle );

	/** Compare two strings from its end.
	 * @param haystack The string to search in.
	 * @param needle The searched string.
	 * @return true if string starts with the substring
	 */
	static bool endsWith( const String& haystack, const String& needle );

	/** @return True if a string contains a substring.
	 * @param haystack The string to search in.
	 * @param needle The searched string.
	 */
	static bool contains( const std::string& haystack, const std::string& needle );

	/** @return True if a string contains a substring.
	 * @param haystack The string to search in.
	 * @param needle The searched string.
	 */
	static bool contains( const String& haystack, const String& needle );

	static int fuzzyMatch( const std::string& string, const std::string& pattern,
						   bool allowUneven = false, bool permissive = false );

	/** Replace all occurrences of the search string with the replacement string. */
	static void replaceAll( std::string& target, const std::string& that, const std::string& with );

	/** Replace all occurrences of the search string with the replacement string. */
	static void replaceAll( String& target, const String& that, const String& with );

	/** Replace the first ocurrence of the search string with the replacement string. */
	static void replace( std::string& target, const std::string& that, const std::string& with );

	/** Replace the first ocurrence of the search string with the replacement string. */
	static void replace( String& target, const String& that, const String& with );

	/** Removes the numbers at the end of the string */
	static std::string removeNumbersAtEnd( std::string txt );

	/** Removes the trailing 0 and . in a string number */
	static std::string_view numberClean( std::string_view strNumber );

	/** Removes the trailing 0 and . in a string number */
	static std::string numberClean( const std::string& strNumber );

	/** Removes the trailing 0 and . in a string number */
	static void numberCleanInPlace( std::string& strNumber );

	/** Searchs the position of the corresponding close bracket in a string. */
	static std::size_t findCloseBracket( const std::string& string, std::size_t startOffset,
										 char openBracket, char closeBracket );

	/** Having a string of values separated by a delimiter, returns the corresponding index of the
	 * searched value */
	static int valueIndex( const std::string& val, const std::string& strings, int defValue = -1,
						   char delim = ';' );

	/** Creates a random string using the dictionary characters. */
	static std::string randString(
		size_t len,
		std::string dictionary = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz" );

	/** Converts from any basic type to std::string */
	template <class T> static std::string toString( const T& i ) {
		std::ostringstream ss;
		ss << std::fixed << i;
		return ss.str();
	}

	static std::string fromFloat( const Float& value, const std::string& append = "",
								  const std::string& prepend = "" );

	static std::string fromDouble( const double& value, const std::string& append = "",
								   const std::string& prepend = "" );

	/** Converts from a string to type */
	template <class T>
	static bool fromString( T& t, const std::string& s,
							std::ios_base& ( *f )( std::ios_base& ) = std::dec ) {
		std::istringstream iss( s );
		return !( iss >> f >> t ).fail();
	}

	/** Converts from a String to type */
	template <class T>
	static bool fromString( T& t, const String& s,
							std::ios_base& ( *f )( std::ios_base& ) = std::dec ) {
		std::istringstream iss( s.toUtf8() );
		return !( iss >> f >> t ).fail();
	}

	template <typename... Args>
	static std::string format( std::string_view format, Args&&... args ) {
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"
#elif defined( __GNUC__ )
#pragma GCC diagnostic ignored "-Wformat-security"
#endif
		int size =
			std::snprintf( nullptr, 0, format.data(),
						   FormatArg<std::decay_t<Args>>::get( std::forward<Args>( args ) )... ) +
			1;
		std::string result( size, 0 );
		if ( size > 0 ) {
			std::snprintf( &result[0], size, format.data(),
						   FormatArg<std::decay_t<Args>>::get( std::forward<Args>( args ) )... );
			result.resize( size - 1 );
		}
#ifdef __clang__
#pragma clang diagnostic pop
#elif defined( __GNUC__ )
#pragma GCC diagnostic pop
#endif
		return result;
	}

	/** Format a char buffer */
	static void formatBuffer( char* Buffer, int BufferSize, const char* format, ... );

	/** @brief Construct from an UTF-8 string to UTF-32 according
	** @param utf8String UTF-8 string to convert
	**/
	static String fromUtf8( const std::string& utf8String );

	/** @return The number of codepoints of the utf8 string. */
	static size_t utf8Length( const std::string& utf8String );

	/** @brief Construct from an UTF-8 string to UTF-32 according
	** @param utf8String UTF-8 string to convert
	**/
	static String fromUtf8( const std::string_view& utf8String );

	/** @return The number of codepoints of the utf8 string. */
	static size_t utf8Length( const std::string_view& utf8String );

	/** @return The next character in a utf8 null terminated string */
	static Uint32 utf8Next( char*& utf8String );

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
	String( const char* utf8String );

	/** @brief Construct from an from a sub-string C-style UTF-8 string to UTF-32
	** @param uf8String UTF-8 string to convert
	**/
	String( const char* utf8String, const size_t& utf8StringSize );

	/** @brief Construct from an UTF-8 string to UTF-32 according
	** @param utf8String UTF-8 string to convert
	**/
	String( const std::string& utf8String );

	/** @brief Construct from an UTF-8 string to UTF-32 according
	** @param utf8String UTF-8 string to convert
	**/
	String( const std::string_view& utf8String );

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

	static String fromWide( const wchar_t* wideString );
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

	/** @brief Copy constructor
	** @param str Instance to copy
	**/
	String( const String::View& str );

	/** @brief Create a new String from a UTF-8 encoded string
	**  @param begin Forward iterator to the begining of the UTF-8 sequence
	**  @param end   Forward iterator to the end of the UTF-8 sequence
	**  @return A String containing the source string
	**  @see FromUtf16, FromUtf32 */
	template <typename T> static String fromUtf8( T begin, T end ) {
		String string;
		Utf8::toUtf32( begin, end, std::back_inserter( string.mString ) );
		return string;
	}

	/** @brief Create a new String from a UTF-16 encoded string
	**  @param begin Forward iterator to the begining of the UTF-16 sequence
	**  @param end   Forward iterator to the end of the UTF-16 sequence
	**  @return A String containing the source string
	**  @see FromUtf8, FromUtf32 */
	template <typename T> static String fromUtf16( T begin, T end ) {
		String string;
		Utf16::toUtf32( begin, end, std::back_inserter( string.mString ) );
		return string;
	}

	/** @brief Create a new String from a UTF-32 encoded string
	**  @param begin Forward iterator to the begining of the UTF-32 sequence
	**  @param end   Forward iterator to the end of the UTF-32 sequence
	**  @return A String containing the source string
	**  @see FromUtf8, FromUtf32 */
	template <typename T> static String fromUtf32( T begin, T end ) {
		String string;
		Utf32::toUtf32( begin, end, std::back_inserter( string.mString ) );
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
	HashType getHash() const;

	/** @brief Overload of assignment operator
	** @param right Instance to assign
	** @return Reference to self
	**/
	String& operator=( const String& right );

	String& operator=( String&& right );

	String& operator=( const StringBaseType& right );

	/** @brief Overload of += operator to append an UTF-32 string
	** @param right String to append
	** @return Reference to self
	**/
	String& operator+=( const String& right );

	String& operator+=( const StringBaseType& right );

	/** @brief Overload of [] operator to access a character by its position
	** This function provides read-only access to characters.
	** Note: this function doesn't throw if \a index is out of range.
	** @param index Index of the character to get
	** @return Character at position \a index
	**/
	const StringBaseType& operator[]( std::size_t index ) const;

	/** @brief Overload of [] operator to access a character by its position
	** This function provides read and write access to characters.
	** Note: this function doesn't throw if \a index is out of range.
	** @param index Index of the character to get
	** @return Reference to the character at position \a index
	**/

	StringBaseType& operator[]( std::size_t index );

	/** @brief Get character in string
	** Performs a range check, throwing an exception of type out_of_range in case that pos is not an
	*actual position in the string.
	** @return The character at position pos in the string.
	*/
	const StringBaseType& at( std::size_t index ) const;

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
	void erase( std::size_t position, std::size_t count = 1 );

	/** @brief Insert one or more characters into the string
	** This function inserts the characters of \a str
	** into the string, starting from \a position.
	** @param position Position of insertion
	** @param str      Characters to insert
	**/
	String& insert( std::size_t position, const String& str );

	String& insert( size_t pos1, const char* s, size_t n );

	String& insert( std::size_t pos1, const String& str, std::size_t pos2, std::size_t n );

	String& insert( std::size_t pos1, const char* s );

	String& insert( std::size_t pos1, size_t n, const String::StringBaseType& c );

	Iterator insert( Iterator p, const String::StringBaseType& c );

	void insert( Iterator p, std::size_t n, const StringBaseType& c );

	template <class InputIterator>
	void insert( Iterator p, InputIterator first, InputIterator last ) {
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

	std::size_t find( const char* s, std::size_t pos, std::size_t n ) const;

	std::size_t find( const char* s, std::size_t pos = 0 ) const;

	std::size_t find( const String::StringBaseType& c, std::size_t pos = 0 ) const;

	/** @brief Get a pointer to the C-style array of characters
	** This functions provides a read-only access to a
	** null-terminated C-style representation of the string.
	** The returned pointer is temporary and is meant only for
	** immediate use, thus it is not recommended to store it.
	** @return Read-only pointer to the array of characters
	**/
	const StringBaseType* c_str() const;

	/** @brief Get string data
	** Notice that no terminating null character is appended (see member c_str for such a
	*functionality).
	** The returned array points to an internal location which should not be modified directly in
	*the program.
	** Its contents are guaranteed to remain unchanged only until the next call to a non-constant
	*member function of the string object.
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
	void resize( std::size_t n, StringBaseType c );

	/** @brief Resize String */
	void resize( std::size_t n );

	/** @return Maximum size of string */
	std::size_t max_size() const;

	/** @brief Request a change in capacity */
	void reserve( size_t res_arg = 0 );

	/** @return Size of allocated storage */
	std::size_t capacity() const;

	/** @brief Append character to string */
	void push_back( StringBaseType c );

	/** @brief Swap contents with another string */
	void swap( String& str );

	String& assign( const String& str );

	String& assign( const String& str, std::size_t pos, std::size_t n );

	String& assign( const char* s );

	String& assign( std::size_t n, StringBaseType c );

	template <class InputIterator> String& assign( InputIterator first, InputIterator last ) {
		mString.assign( first, last );
		return *this;
	}

	String& append( const String& str );

	String& append( const String& str, std::size_t pos, std::size_t n );

	String& append( const char* s );

	String& append( std::size_t n, char c );

	String& append( std::size_t n, StringBaseType c );

	template <class InputIterator> String& append( InputIterator first, InputIterator last ) {
		mString.append( first, last );
		return *this;
	}

	String& replace( std::size_t pos1, std::size_t n1, const String& str );

	String& replace( Iterator i1, Iterator i2, const String& str );

	String& replace( std::size_t pos1, std::size_t n1, const String& str, std::size_t pos2,
					 std::size_t n2 );

	String& replace( std::size_t pos1, std::size_t n1, const char* s, std::size_t n2 );

	String& replace( Iterator i1, Iterator i2, const char* s, std::size_t n2 );

	String& replace( std::size_t pos1, std::size_t n1, const char* s );

	String& replace( Iterator i1, Iterator i2, const char* s );

	String& replace( std::size_t pos1, std::size_t n1, std::size_t n2, StringBaseType c );

	String& replace( Iterator i1, Iterator i2, std::size_t n2, StringBaseType c );

	template <class InputIterator>
	String& replace( Iterator i1, Iterator i2, InputIterator j1, InputIterator j2 ) {
		mString.replace( i1, i2, j1, j2 );
		return *this;
	}

	std::size_t rfind( const String& str, std::size_t pos = StringType::npos ) const;

	std::size_t rfind( const char* s, std::size_t pos = StringType::npos ) const;

	std::size_t rfind( const StringBaseType& c, std::size_t pos = StringType::npos ) const;

	String substr( std::size_t pos = 0, std::size_t n = StringType::npos ) const;

	std::size_t copy( StringBaseType* s, std::size_t n, std::size_t pos = 0 ) const;

	int compare( const String& str ) const;

	int compare( const char* s ) const;

	int compare( std::size_t pos1, std::size_t n1, const String& str ) const;

	int compare( std::size_t pos1, std::size_t n1, const char* s ) const;

	int compare( std::size_t pos1, std::size_t n1, const String& str, std::size_t pos2,
				 std::size_t n2 ) const;

	int compare( std::size_t pos1, std::size_t n1, const char* s, std::size_t n2 ) const;

	std::size_t find_first_of( const String& str, std::size_t pos = 0 ) const;

	std::size_t find_first_of( const char* s, std::size_t pos = 0 ) const;

	std::size_t find_first_of( StringBaseType c, std::size_t pos = 0 ) const;

	std::size_t find_last_of( const String& str, std::size_t pos = StringType::npos ) const;

	std::size_t find_last_of( const char* s, std::size_t pos = StringType::npos ) const;

	std::size_t find_last_of( StringBaseType c, std::size_t pos = StringType::npos ) const;

	std::size_t find_first_not_of( const String& str, std::size_t pos = 0 ) const;

	std::size_t find_first_not_of( const char* s, std::size_t pos = 0 ) const;

	std::size_t find_first_not_of( StringBaseType c, std::size_t pos = 0 ) const;

	std::size_t find_last_not_of( const String& str, std::size_t pos = StringType::npos ) const;

	std::size_t find_last_not_of( const char* s, std::size_t pos = StringType::npos ) const;

	std::size_t find_last_not_of( StringBaseType c, std::size_t pos = StringType::npos ) const;

	size_t countChar( StringBaseType c ) const;

	String& padLeft( unsigned int minDigits, StringBaseType padChar );

	String& toLower();

	String& toUpper();

	String& capitalize();

	String& escape();

	String& unescape();

	StringBaseType lastChar() const;

	std::vector<String> split( const StringBaseType& delim = '\n',
							   const bool& pushEmptyString = false,
							   const bool& keepDelim = false ) const;

	static std::string getFirstLine( const std::string& string );

	String getFirstLine();

	/** Replace all occurrences of the search string with the replacement string. */
	void replaceAll( const String& that, const String& with );

	void pop_back();

	const StringBaseType& front() const;

	const StringBaseType& back() const;

	String& trim( char character = ' ' );

	String& lTrim( char character = ' ' );

	String& rTrim( char character = ' ' );

	/** @return True if a string contains a substring.
	 * @param needle The searched string.
	 */
	bool contains( const String& needle );

	String::View view() const;

  private:
	friend EE_API bool operator==( const String& left, const String& right );
	friend EE_API bool operator<( const String& left, const String& right );

	StringType mString; ///< Internal string of UTF-32 characters
};

/** @relates String
** @brief Overload of == operator to compare two UTF-32 strings
** @param left  Left operand (a string)
** @param right Right operand (a string)
** @return True if both strings are equal
**/
EE_API bool operator==( const String& left, const String& right );

/** @relates String
** @brief Overload of != operator to compare two UTF-32 strings
** @param left  Left operand (a string)
** @param right Right operand (a string)
** @return True if both strings are different
**/
EE_API bool operator!=( const String& left, const String& right );

/** @relates String
** @brief Overload of < operator to compare two UTF-32 strings
** @param left  Left operand (a string)
** @param right Right operand (a string)
** @return True if \a left is alphabetically lesser than \a right
**/
EE_API bool operator<( const String& left, const String& right );

/** @relates String
** @brief Overload of > operator to compare two UTF-32 strings
** @param left  Left operand (a string)
** @param right Right operand (a string)
** @return True if \a left is alphabetically greater than \a right
**/
EE_API bool operator>( const String& left, const String& right );

/** @relates String
** @brief Overload of <= operator to compare two UTF-32 strings
** @param left  Left operand (a string)
** @param right Right operand (a string)
** @return True if \a left is alphabetically lesser or equal than \a right
**/
EE_API bool operator<=( const String& left, const String& right );

/** @relates String
** @brief Overload of >= operator to compare two UTF-32 strings
** @param left  Left operand (a string)
** @param right Right operand (a string)
** @return True if \a left is alphabetically greater or equal than \a right
**/
EE_API bool operator>=( const String& left, const String& right );

/** @relates String
** @brief Overload of binary + operator to concatenate two strings
** @param left  Left operand (a string)
** @param right Right operand (a string)
** @return Concatenated string
**/
EE_API String operator+( const String& left, const String& right );

} // namespace EE

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
