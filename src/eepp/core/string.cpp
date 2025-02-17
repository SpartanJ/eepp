#include <eepp/core/string.hpp>
#include <eepp/core/utf.hpp>
// #include <eepp/core/simd.hpp>

#include <thirdparty/fast_float/include/fast_float/fast_float.h>
#include <thirdparty/utf8cpp/utf8.h>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <climits>
#include <cstdarg>
#include <iostream>
#include <iterator>
#include <limits>
#include <random>

#if ( __GNUC__ >= 11 || ( __clang__ && __clang_major__ >= 12 ) ) && \
	EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
#define STD_SUPPORTS_FIXED_TO_CHARS
#endif

#ifndef STD_SUPPORTS_FIXED_TO_CHARS
#include <sstream>
#endif

namespace EE {

template <typename T> static bool _fromString( T& t, const std::string& s, int base = 10 ) {
	const char* begin = s.data();
	const char* end = s.data() + s.size();

	if constexpr ( std::is_integral_v<T> && std::is_signed_v<T> ) {
		long long value = 0;
		auto result = std::from_chars( begin, end, value, base );
		if ( result.ec == std::errc{} && result.ptr == end &&
			 value >= std::numeric_limits<T>::min() && value <= std::numeric_limits<T>::max() ) {
			t = static_cast<T>( value );
			return true;
		}
		return false;
	} else if constexpr ( std::is_integral_v<T> && std::is_unsigned_v<T> ) {
		unsigned long long value = 0;
		auto result = std::from_chars( begin, end, value, base );
		if ( result.ec == std::errc{} && result.ptr == end &&
			 value <= std::numeric_limits<T>::max() ) {
			t = static_cast<T>( value );
			return true;
		}
		return false;
	} else if constexpr ( (std::is_same_v<T, float> || std::is_same_v<T, double>)) {
		auto result = fast_float::from_chars( begin, end, t );
		bool res = result.ec == std::errc{} && result.ptr == end;
		return res;
	} else {
		T value;
		auto result = std::from_chars( begin, end, value, base );
		if ( result.ec == std::errc{} && result.ptr == end ) {
			t = static_cast<T>( value );
			return true;
		}
	}
}

template <class T> static std::string _toString( const T& value, size_t digitsAfterComma = 2 ) {
	char buffer[32];
	if constexpr ( (std::is_same_v<T, float> || std::is_same_v<T, double>)) {
#ifdef STD_SUPPORTS_FIXED_TO_CHARS
		auto [ptr, ec] = std::to_chars( buffer, buffer + sizeof( buffer ), value,
										std::chars_format::fixed, digitsAfterComma );
		return ec == std::errc() ? std::string( buffer, ptr ) : std::string{};
#else
		std::ostringstream ss;
		ss << std::fixed << value;
		return ss.str();
#endif
	} else {
		auto [ptr, ec] = std::to_chars( buffer, buffer + sizeof( buffer ), value );
		return ec == std::errc() ? std::string( buffer, ptr ) : std::string{};
	}
}

bool String::fromString( Int8& t, const std::string& s, int base ) {
	return _fromString<>( t, s, base );
}

bool String::fromString( Int16& t, const std::string& s, int base ) {
	return _fromString<>( t, s, base );
}

bool String::fromString( Int32& t, const std::string& s, int base ) {
	return _fromString<>( t, s, base );
}

bool String::fromString( Int64& t, const std::string& s, int base ) {
	return _fromString<>( t, s, base );
}

bool String::fromString( Uint8& t, const std::string& s, int base ) {
	return _fromString<>( t, s, base );
}

bool String::fromString( Uint16& t, const std::string& s, int base ) {
	return _fromString<>( t, s, base );
}

bool String::fromString( Uint32& t, const std::string& s, int base ) {
	return _fromString<>( t, s, base );
}

bool String::fromString( Uint64& t, const std::string& s, int base ) {
	return _fromString<>( t, s, base );
}

bool String::fromString( float& t, const std::string& s ) {
	return _fromString<>( t, s );
}

bool String::fromString( double& t, const std::string& s ) {
	return _fromString<>( t, s );
}

bool String::fromString( Int8& t, const String& s, int base ) {
	return _fromString<>( t, s, base );
}

bool String::fromString( Int16& t, const String& s, int base ) {
	return _fromString<>( t, s, base );
}

bool String::fromString( Int32& t, const String& s, int base ) {
	return _fromString<>( t, s, base );
}

bool String::fromString( Int64& t, const String& s, int base ) {
	return _fromString<>( t, s, base );
}

bool String::fromString( Uint8& t, const String& s, int base ) {
	return _fromString<>( t, s, base );
}

bool String::fromString( Uint16& t, const String& s, int base ) {
	return _fromString<>( t, s, base );
}

bool String::fromString( Uint32& t, const String& s, int base ) {
	return _fromString<>( t, s, base );
}

bool String::fromString( Uint64& t, const String& s, int base ) {
	return _fromString<>( t, s, base );
}

bool String::fromString( float& t, const String& s ) {
	return _fromString<>( t, s );
}

bool String::fromString( double& t, const String& s ) {
	return _fromString<>( t, s );
}

std::string String::toString( const Int8& i ) {
	return _toString<>( i );
}

std::string String::toString( const Int16& i ) {
	return _toString<>( i );
}

std::string String::toString( const Int32& i ) {
	return _toString<>( i );
}

std::string String::toString( const Int64& i ) {
	return _toString<>( i );
}

std::string String::toString( const Uint8& i ) {
	return _toString<>( i );
}

std::string String::toString( const Uint16& i ) {
	return _toString<>( i );
}

std::string String::toString( const Uint32& i ) {
	return _toString<>( i );
}

std::string String::toString( const Uint64& i ) {
	return _toString<>( i );
}

std::string String::toString( const float& i ) {
	return _toString<>( i );
}

std::string String::toString( const double& i ) {
	return _toString<>( i );
}

std::string String::fromFloat( const Float& value, const std::string& append,
							   const std::string& prepend, size_t digitsAfterComma ) {
	std::string val( _toString( value, digitsAfterComma ) );
	numberCleanInPlace( val );
	return prepend + val + append;
}

std::string String::fromDouble( const double& value, const std::string& append,
								const std::string& prepend, size_t digitsAfterComma ) {
	std::string val( _toString( value, digitsAfterComma ) );
	numberCleanInPlace( val );
	return prepend + val + append;
}

const std::size_t String::InvalidPos = StringType::npos;

// Author:      Robert A. van Engelen, engelen@genivia.com
// Date:        August 5, 2019
// License:     The Code Project Open License (CPOL)
//              https://www.codeproject.com/info/cpol10.aspx

#define DOTGLOB 1
#define PATHSEP '/'
#define CASE( c, caseInsensitive ) ( caseInsensitive ? std::tolower( c ) : ( c ) )

bool String::globMatch( const std::string_view& text, const std::string_view& glob,
						bool caseInsensitive ) {
	size_t i = 0;
	size_t j = 0;
	size_t n = text.size();
	size_t m = glob.size();
	size_t text1_backup = std::string_view::npos;
	size_t glob1_backup = std::string_view::npos;
	size_t text2_backup = std::string_view::npos;
	size_t glob2_backup = std::string_view::npos;
	bool nodot = !DOTGLOB;
	// match pathname if glob contains a / otherwise match the basename
	if ( j + 1 < m && glob[j] == '/' ) {
		// if pathname starts with ./ then ignore these pairs
		while ( i + 1 < n && text[i] == '.' && text[i + 1] == PATHSEP )
			i += 2;
		// if pathname starts with a / then ignore it
		if ( i < n && text[i] == PATHSEP )
			i++;
		j++;
	} else if ( glob.find( '/' ) == std::string_view::npos ) {
		size_t sep = text.rfind( PATHSEP );
		if ( sep != std::string_view::npos )
			i = sep + 1;
	}
	while ( i < n ) {
		if ( j < m ) {
			switch ( glob[j] ) {
				case '*':
					// match anything except . after /
					if ( nodot && text[i] == '.' )
						break;
					if ( ++j < m && glob[j] == '*' ) {
						// trailing ** match everything after /
						if ( ++j >= m )
							return true;
						// ** followed by a / match zero or more directories
						if ( glob[j] != '/' )
							return false;
						// new **-loop, discard *-loop
						text1_backup = std::string_view::npos;
						glob1_backup = std::string_view::npos;
						text2_backup = i;
						glob2_backup = ++j;
						continue;
					}
					// trailing * matches everything except /
					text1_backup = i;
					glob1_backup = j;
					continue;
				case '?':
					// match anything except . after /
					if ( nodot && text[i] == '.' )
						break;
					// match any character except /
					if ( text[i] == PATHSEP )
						break;
					i++;
					j++;
					continue;
				case '[': {
					// match anything except . after /
					if ( nodot && text[i] == '.' )
						break;
					// match any character in [...] except /
					if ( text[i] == PATHSEP )
						break;
					int lastchr;
					bool matched = false;
					bool reverse = j + 1 < m && ( glob[j + 1] == '^' || glob[j + 1] == '!' );
					// inverted character class
					if ( reverse )
						j++;
					// match character class
					for ( lastchr = 256; ++j < m && glob[j] != ']';
						  lastchr = CASE( glob[j], caseInsensitive ) )
						if ( lastchr < 256 && glob[j] == '-' && j + 1 < m && glob[j + 1] != ']'
								 ? CASE( text[i], caseInsensitive ) <=
										   CASE( glob[++j], caseInsensitive ) &&
									   CASE( text[i], caseInsensitive ) >= lastchr
								 : CASE( text[i], caseInsensitive ) ==
									   CASE( glob[j], caseInsensitive ) )
							matched = true;
					if ( matched == reverse )
						break;
					i++;
					if ( j < m )
						j++;
					continue;
				}
				case '\\':
					// literal match \-escaped character
					if ( j + 1 < m )
						j++;
					// FALLTHROUGH
				default:
					// match the current non-NUL character
					if ( CASE( glob[j], caseInsensitive ) != CASE( text[i], caseInsensitive ) &&
						 !( glob[j] == '/' && text[i] == PATHSEP ) )
						break;
					// do not match a . with *, ? [] after /
					nodot = !DOTGLOB && glob[j] == '/';
					i++;
					j++;
					continue;
			}
		}
		if ( glob1_backup != std::string_view::npos && text[text1_backup] != PATHSEP ) {
			// *-loop: backtrack to the last * but do not jump over /
			i = ++text1_backup;
			j = glob1_backup;
			continue;
		}
		if ( glob2_backup != std::string_view::npos ) {
			// **-loop: backtrack to the last **
			i = ++text2_backup;
			j = glob2_backup;
			continue;
		}
		return false;
	}
	// ignore trailing stars
	while ( j < m && glob[j] == '*' )
		j++;
	// at end of text means success if nothing else is left to match
	return j >= m;
}

bool String::globMatch( const std::string_view& text, const std::vector<std::string>& globs,
						bool caseInsensitive ) {
	for ( const auto& glob : globs ) {
		if ( globMatch( text, glob, caseInsensitive ) )
			return true;
	}
	return false;
}

/*
 * Originally written by Joel Yliluoma <joel.yliluoma@iki.fi>
 * http://en.wikipedia.org/wiki/Talk:Boyer%E2%80%93Moore_string_search_algorithm#Common_functions
 *
 * Copyright (c) 2008 Joel Yliluoma
 * Copyright (c) 2010 Hongli Lai
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/* This function creates an occ table to be used by the search algorithms. */
/* It only needs to be created once per a needle to search. */
const String::BMH::OccTable String::BMH::createOccTable( const unsigned char* needle,
														 size_t needleLength ) {
	OccTable occ(
		UCHAR_MAX + 1,
		needleLength ); // initialize a table of UCHAR_MAX+1 elements to value needle_length

	/* Populate it with the analysis of the needle */
	/* But ignoring the last letter */
	if ( needleLength >= 1 ) {
		const size_t needleLengthMinus1 = needleLength - 1;
		for ( size_t a = 0; a < needleLengthMinus1; ++a )
			occ[needle[a]] = needleLengthMinus1 - a;
	}
	return occ;
}

/* A Boyer-Moore-Horspool search algorithm. */
/* If it finds the needle, it returns an offset to haystack from which
 * the needle was found. Otherwise, it returns haystack_length.
 */
size_t String::BMH::search( const unsigned char* haystack, size_t haystackLength,
							const unsigned char* needle, const size_t needleLength,
							const OccTable& occ ) {
	if ( needleLength > haystackLength )
		return haystackLength;
	if ( needleLength == 1 ) {
		const unsigned char* result =
			(const unsigned char*)std::memchr( haystack, *needle, haystackLength );
		return result ? size_t( result - haystack ) : haystackLength;
	}

	const size_t needleLengthMinus1 = needleLength - 1;

	const unsigned char lastNeedleChar = needle[needleLengthMinus1];

	size_t haystackPosition = 0;
	while ( haystackPosition <= haystackLength - needleLength ) {
		const unsigned char occChar = haystack[haystackPosition + needleLengthMinus1];

		// The author modified this part. Original algorithm matches needle right-to-left.
		// This code calls memcmp() (usually matches left-to-right) after matching the last
		// character, thereby incorporating some ideas from
		// "Tuning the Boyer-Moore-Horspool String Searching Algorithm"
		// by Timo Raita, 1992.
		if ( lastNeedleChar == occChar &&
			 std::memcmp( needle, haystack + haystackPosition, needleLengthMinus1 ) == 0 ) {
			return haystackPosition;
		}

		haystackPosition += occ[occChar];
	}
	return haystackLength;
}

Int64 String::BMH::find( const std::string& haystack, const std::string& needle,
						 const size_t& haystackOffset, const OccTable& occ ) {
	size_t result = search( (const unsigned char*)haystack.c_str() + haystackOffset,
							haystack.size() - haystackOffset, (const unsigned char*)needle.c_str(),
							needle.size(), occ );
	if ( result == haystack.size() - haystackOffset ) {
		return -1;
	} else {
		return (Int64)haystackOffset + result;
	}
}

Int64 String::BMH::find( const std::string& haystack, const std::string& needle,
						 const size_t& haystackOffset ) {
	const auto occ = createOccTable( (const unsigned char*)needle.c_str(), needle.size() );
	return find( haystack, needle, haystackOffset, occ );
}

String String::escape( const String& str ) {
	String output;
	for ( size_t i = 0; i < str.size(); i++ ) {
		switch ( str[i] ) {
			case '\r':
				output += "\\r";
				break;
			case '\t':
				output += "\\t";
				break;
			case '\n':
				output += "\\n";
				break;
			case '\a':
				output += "\\a";
				break;
			case '\b':
				output += "\\b";
				break;
			case '\f':
				output += "\\f";
				break;
			case '\v':
				output += "\\v";
				break;
			default: {
				output += str[i];
				break;
			}
		}
	}
	return output;
}

String String::unescape( const String& str ) {
	String output;
	bool lastWasEscape = false;
	bool lastInsertedEscape = false;

	for ( size_t i = 0; i < str.size(); i++ ) {
		lastWasEscape = lastWasEscape && !lastInsertedEscape;
		lastInsertedEscape = false;

		if ( lastWasEscape ) {
			switch ( str[i] ) {
				case '\\':
					output.push_back( '\\' );
					lastInsertedEscape = true;
					break;
				case 'r':
					output.push_back( '\r' );
					break;
				case 't':
					output.push_back( '\t' );
					break;
				case 'n':
					output.push_back( '\n' );
					break;
				case '\'':
					output.push_back( '\'' );
					break;
				case '"':
					output.push_back( '"' );
					break;
				case '?':
					output.push_back( '\?' );
					break;
				case 'a':
					output.push_back( '\a' );
					break;
				case 'b':
					output.push_back( '\b' );
					break;
				case 'f':
					output.push_back( '\f' );
					break;
				case 'v':
					output.push_back( '\v' );
					break;
				case 'x': {
					size_t len = 2;
					if ( i + len < str.size() ) {
						std::string buffer;
						for ( i = i + 1; i < str.size(); i++ )
							if ( std::isxdigit( str[i] ) ) {
								buffer.push_back( str[i] );
							} else {
								break;
							}
						if ( buffer.size() >= len ) {
							Uint32 value;
							if ( String::fromString( value, buffer, 16 ) )
								output.push_back( value );
						}
					}
					break;
				}
				case 'u':
				case 'U': {
					size_t len = str[i] == 'u' ? 4 : 8;
					if ( i + len < str.size() ) {
						size_t to = i + len;
						std::string buffer;
						for ( i = i + 1; i <= to; i++ ) {
							if ( std::isxdigit( str[i] ) ) {
								buffer.push_back( str[i] );
							}
						}
						if ( buffer.size() == len ) {
							Uint32 value;
							if ( String::fromString( value, buffer, 16 ) )
								output.push_back( value );
						}
					}
					break;
				}
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8': {
					size_t to = eemin( (size_t)i + 3, str.size() );
					std::string buffer;
					for ( ; i < to; i++ ) {
						if ( ( str[i] >= '0' && str[i] <= '8' ) ) {
							buffer.push_back( str[i] );
						} else {
							break;
						}
					}
					if ( buffer.size() <= 3 ) {
						Uint32 value;
						if ( String::fromString( value, buffer, 8 ) )
							output.push_back( value );
					}
					break;
				}
				default: {
					output.push_back( '\\' );
					output.push_back( str[i] );
					break;
				}
			}
		} else if ( str[i] != '\\' ) {
			output.push_back( str[i] );
		}

		lastWasEscape = str[i] == '\\';
	}
	return output;
}

String::HashType String::hash( const std::string& str ) {
	return String::hash( str.c_str() );
}

String::HashType String::hash( const String& str ) {
	return String::hash( reinterpret_cast<const char*>( str.mString.data() ),
						 str.size() * sizeof( String::StringBaseType ) );
}

bool String::isCharacter( const int& value ) {
	return ( value >= 32 && value <= 126 ) || ( value >= 161 && value <= 255 ) || ( value == 9 );
}

bool String::isNumber( const int& value, bool AllowDot ) {
	if ( AllowDot )
		return ( value >= 48 && value <= 57 ) || value == 46;

	return value >= 48 && value <= 57;
}

bool String::isNumber( const std::string& value, bool AllowDot ) {
	for ( auto& val : value ) {
		if ( !isNumber( val, AllowDot ) ) {
			return false;
		}
	}
	return true;
}

bool String::isLetter( const int& value ) {
	return ( ( ( value >= 65 && value <= 90 ) || ( value >= 97 && value <= 122 ) ||
			   ( value >= 192 && value <= 255 ) ) &&
			 ( value != 215 ) && ( value != 247 ) );
}

bool String::isAlphaNum( const int& value ) {
	return isLetter( value ) || isNumber( value );
}

bool String::isHexNotation( const std::string& value, const std::string& withPrefix ) {
	if ( !withPrefix.empty() && !String::startsWith( value, withPrefix ) ) {
		return false;
	}
	return value.find_first_not_of( "0123456789abcdefABCDEF", withPrefix.size() ) ==
		   std::string::npos;
}

std::vector<String> String::split( const String& str, const StringBaseType& delim,
								   const bool& pushEmptyString, const bool& keepDelim ) {
	if ( str.empty() )
		return {};
	std::vector<String> cont;
	std::size_t current, previous = 0;
	current = str.find( delim );
	while ( current != String::InvalidPos ) {
		String substr( str.substr( previous, current - previous ) );
		if ( pushEmptyString || !substr.empty() )
			cont.emplace_back( std::move( substr ) );
		previous = current + 1;
		current = str.find( delim, previous );
	}
	String substr( str.substr( previous, current - previous ) );
	if ( pushEmptyString || !substr.empty() )
		cont.emplace_back( std::move( substr ) );
	if ( keepDelim ) {
		for ( size_t i = 0; i < cont.size(); i++ ) {
			if ( i != cont.size() - 1 ||
				 ( str.lastChar() == delim && cont[cont.size() - 1].lastChar() != delim ) )
				cont[i].push_back( delim );
			if ( cont[cont.size() - 1].empty() )
				cont.pop_back();
		}
	}
	return cont;
}

std::vector<std::string> String::split( const std::string& str, const Int8& delim,
										const bool& pushEmptyString, const bool& keepDelim ) {
	if ( str.empty() )
		return {};
	std::vector<std::string> cont;
	std::size_t current, previous = 0;
	current = str.find( delim );
	while ( current != std::string::npos ) {
		if ( (Int64)current - (Int64)previous < 0 ) {
			std::cerr << "String::split fatal error: current " << current << " previous "
					  << previous << " with str " << str << " delim " << delim
					  << " pushEmptyString " << pushEmptyString << " keepDelim " << keepDelim
					  << "\n";
			return cont;
		}
		std::string substr( str.substr( previous, current - previous ) );
		if ( pushEmptyString || !substr.empty() )
			cont.emplace_back( std::move( substr ) );
		previous = current + 1;
		current = str.find( delim, previous );
	}
	std::string substr( str.substr( previous, current - previous ) );
	if ( pushEmptyString || !substr.empty() )
		cont.emplace_back( std::move( substr ) );
	if ( keepDelim ) {
		for ( size_t i = 0; i < cont.size(); i++ ) {
			if ( i != cont.size() - 1 ||
				 ( !str.empty() && str[str.size() - 1] == delim && !cont.empty() &&
				   !cont[cont.size() - 1].empty() &&
				   cont[cont.size() - 1][cont[cont.size() - 1].size() - 1] != delim ) )
				cont[i].push_back( delim );
			if ( cont[cont.size() - 1].empty() )
				cont.pop_back();
		}
	}
	return cont;
}

std::vector<std::string_view> String::split( const std::string_view& str, const Int8& delim,
											 const bool& pushEmptyString ) {
	if ( str.empty() )
		return {};
	std::vector<std::string_view> cont;
	std::size_t current, previous = 0;
	current = str.find( delim );
	while ( current != std::string::npos ) {
		if ( (Int64)current - (Int64)previous < 0 ) {
			std::cerr << "String::split fatal error: current " << current << " previous "
					  << previous << " with str " << str << " delim " << delim
					  << " pushEmptyString " << pushEmptyString << "\n";
			return cont;
		}
		std::string_view substr( str.substr( previous, current - previous ) );
		if ( pushEmptyString || !substr.empty() )
			cont.emplace_back( std::move( substr ) );
		previous = current + 1;
		current = str.find( delim, previous );
	}
	std::string_view substr( str.substr( previous, current - previous ) );
	if ( pushEmptyString || !substr.empty() )
		cont.emplace_back( std::move( substr ) );
	return cont;
}

std::vector<String> String::split( const StringBaseType& delim, const bool& pushEmptyString,
								   const bool& keepDelim ) const {
	return String::split( *this, delim, pushEmptyString, keepDelim );
}

std::string String::getFirstLine( const std::string& string ) {
	auto pos = string.find_first_of( '\n' );
	if ( pos == std::string::npos )
		return string;
	return string.substr( 0, pos );
}

String String::getFirstLine() {
	auto pos = find_first_of( '\n' );
	if ( pos == String::InvalidPos )
		return *this;
	return substr( 0, pos );
}

std::vector<std::string> String::split( const std::string& str, const std::string& delims,
										const std::string& delimsPreserve, const std::string& quote,
										const bool& removeQuotes ) {
	std::vector<std::string> tokens;
	if ( str.empty() || ( delims.empty() && delimsPreserve.empty() ) ) {
		return tokens;
	}

	std::string allDelims = delims + delimsPreserve + quote;

	std::string::size_type tokenStart = 0;
	std::string::size_type tokenEnd = str.find_first_of( allDelims, tokenStart );
	std::string::size_type tokenLen = 0;
	std::string token;
	while ( true ) {
		bool fromQuote = false;
		while ( tokenEnd != std::string::npos &&
				quote.find_first_of( str[tokenEnd] ) != std::string::npos ) {
			if ( str[tokenEnd] == '(' ) {
				tokenEnd = findCloseBracket( str, tokenEnd, '(', ')' );
			} else if ( str[tokenEnd] == '[' ) {
				tokenEnd = findCloseBracket( str, tokenEnd, '[', ']' );
			} else if ( str[tokenEnd] == '{' ) {
				tokenEnd = findCloseBracket( str, tokenEnd, '{', '}' );
			} else {
				fromQuote = true;
				tokenEnd = str.find_first_of( str[tokenEnd], tokenEnd + 1 );
			}
			if ( tokenEnd != std::string::npos ) {
				tokenEnd = str.find_first_of( allDelims, tokenEnd + 1 );
			}
		}

		if ( tokenEnd == std::string::npos ) {
			tokenLen = std::string::npos;
		} else {
			tokenLen = tokenEnd - tokenStart;
		}

		token = str.substr( tokenStart, tokenLen );
		if ( !token.empty() ) {
			if ( fromQuote && removeQuotes && token.size() > 2 )
				token = token.substr( 1, token.size() - 2 );
			tokens.push_back( token );
		}
		if ( tokenEnd != std::string::npos && !delimsPreserve.empty() &&
			 delimsPreserve.find_first_of( str[tokenEnd] ) != std::string::npos ) {
			tokens.push_back( str.substr( tokenEnd, 1 ) );
		}

		tokenStart = tokenEnd;
		if ( tokenStart == std::string::npos )
			break;
		tokenStart++;
		if ( tokenStart == str.length() )
			break;
		tokenEnd = str.find_first_of( allDelims, tokenStart );
	}

	return tokens;
}

void String::splitCb( std::function<bool( std::string_view )> fnCb, const std::string& str,
					  const std::string& delims, const std::string& delimsPreserve,
					  const std::string& quote, const bool& removeQuotes ) {
	if ( str.empty() || ( delims.empty() && delimsPreserve.empty() ) )
		return;

	std::string allDelims = delims + delimsPreserve + quote;

	std::string::size_type tokenStart = 0;
	std::string::size_type tokenEnd = str.find_first_of( allDelims, tokenStart );
	std::string::size_type tokenLen = 0;
	std::string_view token;
	while ( true ) {
		bool fromQuote = false;
		while ( tokenEnd != std::string::npos &&
				quote.find_first_of( str[tokenEnd] ) != std::string::npos ) {
			if ( str[tokenEnd] == '(' ) {
				tokenEnd = findCloseBracket( str, tokenEnd, '(', ')' );
			} else if ( str[tokenEnd] == '[' ) {
				tokenEnd = findCloseBracket( str, tokenEnd, '[', ']' );
			} else if ( str[tokenEnd] == '{' ) {
				tokenEnd = findCloseBracket( str, tokenEnd, '{', '}' );
			} else {
				fromQuote = true;
				tokenEnd = str.find_first_of( str[tokenEnd], tokenEnd + 1 );
			}
			if ( tokenEnd != std::string::npos ) {
				tokenEnd = str.find_first_of( allDelims, tokenEnd + 1 );
			}
		}

		if ( tokenEnd == std::string::npos ) {
			tokenLen = std::string::npos;
		} else {
			tokenLen = tokenEnd - tokenStart;
		}

		token = std::string_view{ str }.substr( tokenStart, tokenLen );
		if ( !token.empty() ) {
			if ( fromQuote && removeQuotes && token.size() > 2 )
				token = token.substr( 1, token.size() - 2 );
			if ( !fnCb( token ) )
				return;
		}
		if ( tokenEnd != std::string::npos && !delimsPreserve.empty() &&
			 delimsPreserve.find_first_of( str[tokenEnd] ) != std::string::npos ) {
			if ( !fnCb( std::string_view{ str }.substr( tokenEnd, 1 ) ) )
				return;
		}

		tokenStart = tokenEnd;
		if ( tokenStart == std::string::npos )
			break;
		tokenStart++;
		if ( tokenStart == str.length() )
			break;
		tokenEnd = str.find_first_of( allDelims, tokenStart );
	}
}

std::string String::join( const std::vector<std::string>& strArray, const Int8& joinchar,
						  const bool& appendLastJoinChar ) {
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

String String::join( const std::vector<String>& strArray, const Int8& joinchar,
					 const bool& appendLastJoinChar ) {
	size_t s = strArray.size();
	String str;

	if ( s > 0 ) {
		for ( size_t i = 0; i < s; i++ ) {
			str += strArray[i];

			if ( joinchar >= 0 && ( i != s - 1 || appendLastJoinChar ) ) {
				str += joinchar;
			}
		}
	}

	return str;
}

std::string String::join( const std::vector<const char*>& strArray, const Int8& joinchar,
						  const bool& appendLastJoinChar ) {
	size_t s = strArray.size();
	std::string str;

	if ( s > 0 ) {
		for ( size_t i = 0; i < s; i++ ) {
			str += strArray[i];

			if ( joinchar >= 0 && ( i != s - 1 || appendLastJoinChar ) ) {
				str += joinchar;
			}
		}
	}

	return str;
}

std::string String::lTrim( const std::string& str, char character ) {
	std::string::size_type pos1 = str.find_first_not_of( character );
	return ( pos1 == std::string::npos ) ? str : str.substr( pos1 );
}

std::string String::rTrim( const std::string& str, char character ) {
	std::string::size_type pos1 = str.find_last_not_of( character );
	return ( pos1 == std::string::npos ) ? str : str.substr( 0, pos1 + 1 );
}

std::string String::trim( const std::string& str, char character ) {
	std::string::size_type pos1 = str.find_first_not_of( character );
	std::string::size_type pos2 = str.find_last_not_of( character );
	return str.substr( pos1 == std::string::npos ? 0 : pos1,
					   pos2 == std::string::npos ? str.length() - 1 : pos2 - pos1 + 1 );
}

std::string_view String::lTrim( const std::string_view& str, char character ) {
	std::string::size_type pos1 = str.find_first_not_of( character );
	return ( pos1 == std::string::npos ) ? str : str.substr( pos1 );
}

std::string_view String::rTrim( const std::string_view& str, char character ) {
	std::string::size_type pos1 = str.find_last_not_of( character );
	return ( pos1 == std::string::npos ) ? str : str.substr( 0, pos1 + 1 );
}

std::string_view String::trim( const std::string_view& str, char character ) {
	std::string::size_type pos1 = str.find_first_not_of( character );
	std::string::size_type pos2 = str.find_last_not_of( character );
	return str.substr( pos1 == std::string::npos ? 0 : pos1,
					   pos2 == std::string::npos ? str.length() - 1 : pos2 - pos1 + 1 );
}

void String::trimInPlace( std::string& str, char character ) {
	// Trim only if there's something to trim
	if ( !str.empty() && ( str[0] == character || str[str.size() - 1] == character ) )
		str = trim( str, character );
}

String String::lTrim( const String& str, char character ) {
	StringType::size_type pos1 = str.find_first_not_of( character );
	return ( pos1 == String::InvalidPos ) ? str : str.substr( pos1 );
}

String String::rTrim( const String& str, char character ) {
	StringType::size_type pos1 = str.find_last_not_of( character );
	return ( pos1 == String::InvalidPos ) ? str : str.substr( 0, pos1 + 1 );
}

String String::trim( const String& str, char character ) {
	StringType::size_type pos1 = str.find_first_not_of( character );
	StringType::size_type pos2 = str.find_last_not_of( character );
	return str.substr( pos1 == String::InvalidPos ? 0 : pos1,
					   pos2 == String::InvalidPos ? str.length() - 1 : pos2 - pos1 + 1 );
}

void String::trimInPlace( String& str, char character ) {
	str = trim( str, character );
}

void String::toUpperInPlace( std::string& str ) {
	std::transform( str.begin(), str.end(), str.begin(), (int ( * )( int ))std::toupper );
}

std::string String::toUpper( std::string str ) {
	for ( std::string::iterator i = str.begin(); i != str.end(); ++i )
		*i = static_cast<char>( std::toupper( *i ) );
	return str;
}

void String::toLowerInPlace( std::string& str ) {
	std::transform( str.begin(), str.end(), str.begin(), (int ( * )( int ))std::tolower );
}

void String::capitalizeInPlace( std::string& str ) {
	char last = ' ';
	for ( std::string::iterator i = str.begin(); i != str.end(); ++i ) {
		if ( isspace( last ) || last == '.' )
			*i = toupper( *i );
		last = *i;
	}
}

std::string String::toLower( std::string str ) {
	for ( std::string::iterator i = str.begin(); i != str.end(); ++i )
		*i = static_cast<char>( std::tolower( *i ) );
	return str;
}

std::string String::capitalize( std::string str ) {
	char last = ' ';
	for ( std::string::iterator i = str.begin(); i != str.end(); ++i ) {
		if ( isspace( last ) || last == '.' )
			*i = toupper( *i );
		last = *i;
	}
	return str;
}

String String::toUpper( const String& str ) {
	String cpy( str );
	cpy.toUpper();
	return cpy;
}

String String::toLower( const String& str ) {
	String cpy( str );
	cpy.toLower();
	return cpy;
}

String String::capitalize( const String& str ) {
	String cpy( str );
	cpy.capitalize();
	return cpy;
}

String& String::toLower() {
	for ( StringType::iterator i = mString.begin(); i != mString.end(); ++i )
		*i = static_cast<Uint32>( std::tolower( *i ) );
	return *this;
}

String& String::toUpper() {
	for ( StringType::iterator i = mString.begin(); i != mString.end(); ++i )
		*i = static_cast<Uint32>( std::toupper( *i ) );
	return *this;
}

String& String::capitalize() {
	String::StringBaseType last = ' ';
	for ( StringType::iterator i = mString.begin(); i != mString.end(); ++i ) {
		if ( isspace( last ) || last == '.' )
			*i = toupper( *i );
		last = *i;
	}
	return *this;
}

String& String::escape() {
	this->assign( String::escape( *this ) );
	return *this;
}

String& String::unescape() {
	this->assign( String::unescape( *this ) );
	return *this;
}

String::StringBaseType String::lastChar() const {
	return mString.empty() ? std::numeric_limits<StringBaseType>::max()
						   : mString[mString.size() - 1];
}

// Lite (https://github.com/rxi/lite) fuzzy match implementation
template <typename T>
constexpr int tFuzzyMatch( const T* str, const T* ptn, bool allowUneven, bool permissive ) {
	int score = 0;
	int run = 0;
	while ( *str && *ptn ) {
		while ( *str == ' ' )
			str++;
		while ( *ptn == ' ' )
			ptn++;
		if ( std::tolower( *str ) == std::tolower( *ptn ) ) {
			score += run * 10 - ( *str != *ptn );
			run++;
			ptn++;
		} else {
			score -= 10;
			run = 0;
		}
		str++;
	}
	if ( *ptn && !allowUneven )
		return INT_MIN;
	return score - ( permissive ? 0 : strlen( str ) );
}

int String::fuzzyMatch( const std::string& string, const std::string& pattern, bool allowUneven,
						bool permissive ) {
	return tFuzzyMatch<char>( string.c_str(), pattern.c_str(), allowUneven, permissive );
}

std::vector<Uint8> String::stringToUint8( const std::string& str ) {
	return std::vector<Uint8>( str.begin(), str.end() );
}

std::string String::Uint8ToString( const std::vector<Uint8>& v ) {
	return std::string( v.begin(), v.end() );
}

void String::strCopy( char* Dst, const char* Src, unsigned int DstSize ) {
	strncpy( Dst, Src, DstSize );
}

bool String::startsWith( const std::string& haystack, const std::string& needle ) {
	return needle.length() <= haystack.length() &&
		   std::equal( needle.begin(), needle.end(), haystack.begin() );
}

bool String::startsWith( const String& haystack, const String& needle ) {
	return needle.length() <= haystack.length() &&
		   std::equal( needle.begin(), needle.end(), haystack.begin() );
}

bool String::startsWith( const char* haystack, const char* needle ) {
	return strncmp( needle, haystack, strlen( needle ) ) == 0;
}

bool String::startsWith( std::string_view haystack, std::string_view needle ) {
	return needle.length() <= haystack.length() &&
		   std::equal( needle.begin(), needle.end(), haystack.begin() );
}

bool String::endsWith( const std::string& haystack, const std::string& needle ) {
	return needle.length() <= haystack.length() &&
		   haystack.compare( haystack.size() - needle.size(), needle.size(), needle ) == 0;
}

bool String::endsWith( const String& haystack, const String& needle ) {
	return needle.length() <= haystack.length() &&
		   haystack.compare( haystack.size() - needle.size(), needle.size(), needle ) == 0;
}

bool String::contains( const std::string& haystack, const std::string& needle ) {
	return haystack.find( needle ) != std::string::npos;
}

bool String::contains( const String& haystack, const String& needle ) {
	return haystack.find( needle ) != String::InvalidPos;
}

bool String::icontains( const std::string& haystack, const std::string& needle ) {
	return std::search( haystack.begin(), haystack.end(), needle.begin(), needle.end(),
						[]( char ch1, char ch2 ) {
							return std::tolower( ch1 ) == std::tolower( ch2 );
						} ) != haystack.end();
}

bool String::icontains( const String& haystack, const String& needle ) {
	return std::search( haystack.begin(), haystack.end(), needle.begin(), needle.end(),
						[]( String::StringBaseType ch1, String::StringBaseType ch2 ) {
							return std::tolower( ch1 ) == std::tolower( ch2 );
						} ) != haystack.end();
}

void String::replaceAll( std::string& target, const std::string& that, const std::string& with ) {
	std::string::size_type pos = 0;

	while ( ( pos = target.find( that, pos ) ) != std::string::npos ) {
		target.erase( pos, that.length() );
		target.insert( pos, with );
		pos += with.length();
	}
}

void String::replaceAll( String& target, const String& that, const String& with ) {
	std::string::size_type pos = 0;

	while ( ( pos = target.find( that, pos ) ) != String::InvalidPos ) {
		target.erase( pos, that.length() );
		target.insert( pos, with );
		pos += with.length();
	}
}

void String::replaceAll( const String& that, const String& with ) {
	String::replaceAll( *this, that, with );
}

void String::pop_back() {
	mString.pop_back();
}

const String::StringBaseType& String::front() const {
	return mString.front();
}

const String::StringBaseType& String::back() const {
	return mString.back();
}

String& String::trim( char character ) {
	trimInPlace( *this, character );
	return *this;
}

String& String::lTrim( char character ) {
	*this = lTrim( *this, character );
	return *this;
}

String& String::rTrim( char character ) {
	*this = rTrim( *this, character );
	return *this;
}

bool String::contains( const String& needle ) const {
	return String::contains( *this, needle );
}

bool String::isAscii() const {
	auto data = mString.data();
	size_t len = mString.size();
	size_t i = 0;

	// #ifdef EE_STD_SIMD
	// 	using simd_type = simd::native_simd<char32_t>;
	// 	constexpr size_t simd_size = simd_type::size();
	// 	const simd_type ascii_limit = 127;
	// 	for ( ; i + simd_size - 1 < len; i += simd_size ) {
	// 		simd_type chunk;
	// 		chunk.copy_from( &data[i], simd::element_aligned );
	// 		auto mask = chunk > ascii_limit;
	// 		if ( simd::any_of( mask ) )
	// 			return false;
	// 	}
	// #endif

	for ( ; i < len; ++i )
		if ( data[i] > 127 )
			return false;

	return true;
}

String::View String::view() const {
	return String::View( mString );
}

void String::replace( std::string& target, const std::string& that, const std::string& with ) {
	std::size_t start_pos = target.find( that );
	if ( start_pos == std::string::npos )
		return;
	target.replace( start_pos, that.length(), with );
}

void String::replace( String& target, const String& that, const String& with ) {
	std::size_t start_pos = target.find( that );
	if ( start_pos == String::InvalidPos )
		return;
	target.replace( start_pos, that.length(), with );
}

std::string String::removeNumbersAtEnd( std::string txt ) {
	while ( txt.size() && txt[txt.size() - 1] >= '0' && txt[txt.size() - 1] <= '9' ) {
		txt.resize( txt.size() - 1 );
	}
	return txt;
}

std::size_t String::findCloseBracket( const std::string& string, std::size_t startOffset,
									  char openBracket, char closeBracket ) {
	int count = 0;
	size_t len = string.size();
	for ( size_t i = startOffset; i < len; i++ ) {
		if ( string[i] == openBracket ) {
			count++;
		} else if ( string[i] == closeBracket ) {
			count--;
			if ( 0 == count )
				return i;
		}
	}
	return std::string::npos;
}

int String::valueIndex( const std::string& val, const std::string& strings, int defValue,
						char delim ) {
	if ( val.empty() || strings.empty() || !delim ) {
		return defValue;
	}

	int idx = 0;
	std::string::size_type delimStart = 0;
	std::string::size_type delimEnd = strings.find( delim, delimStart );
	std::string::size_type itemLen = 0;
	while ( true ) {
		if ( delimEnd == std::string::npos ) {
			itemLen = strings.length() - delimStart;
		} else {
			itemLen = delimEnd - delimStart;
		}
		if ( itemLen == val.length() ) {
			if ( val == strings.substr( delimStart, itemLen ) ) {
				return idx;
			}
		}
		idx++;
		delimStart = delimEnd;
		if ( delimStart == std::string::npos )
			break;
		delimStart++;
		if ( delimStart == strings.length() )
			break;
		delimEnd = strings.find( delim, delimStart );
	}
	return defValue;
}

std::string String::randString( size_t len, std::string dictionary ) {
	std::random_device rd;
	std::mt19937 generator( rd() );
	std::shuffle( dictionary.begin(), dictionary.end(), generator );
	return dictionary.substr( 0, len );
}

std::string_view String::numberClean( std::string_view strNumber ) {
	while ( strNumber.back() == '0' )
		strNumber.remove_suffix( 1 );
	if ( strNumber.back() == '.' )
		strNumber.remove_suffix( 1 );
	return strNumber;
}

std::string String::numberClean( const std::string& number ) {
	std::string strNumber( number );
	while ( strNumber.back() == '0' )
		strNumber.pop_back();
	if ( strNumber.back() == '.' )
		strNumber.pop_back();
	return strNumber;
}

void String::numberCleanInPlace( std::string& strNumber ) {
	while ( strNumber.back() == '0' )
		strNumber.pop_back();
	if ( strNumber.back() == '.' )
		strNumber.pop_back();
}

void String::insertChar( String& str, const unsigned int& pos, const StringBaseType& tchar ) {
	str.insert( str.begin() + pos, tchar );
}

void String::formatBuffer( char* Buffer, int BufferSize, const char* format, ... ) {
	va_list args;
	va_start( args, format );
#ifdef EE_COMPILER_MSVC
	_vsnprintf_s( Buffer, BufferSize, BufferSize, format, args );
#else
	vsnprintf( Buffer, BufferSize - 1, format, args );
#endif
	va_end( args );
}

String::String() {}

String::String( char ansiChar ) {
	mString += Utf32::decodeAnsi( ansiChar, std::locale() );
}

#ifndef EE_NO_WIDECHAR
String::String( wchar_t wideChar ) {
	mString += Utf32::decodeWide( wideChar );
}
#endif

String::String( StringBaseType utf32Char ) {
	mString += utf32Char;
}

String::String( const char* utf8String ) {
	if ( utf8String ) {
		std::size_t length = strlen( utf8String );

		if ( length > 0 ) {
			mString.reserve( length + 1 );

			Utf8::toUtf32( utf8String, utf8String + length, std::back_inserter( mString ) );
		}
	}
}

String::String( const char* utf8String, const size_t& utf8StringSize ) {
	if ( utf8String && utf8StringSize > 0 ) {
		mString.reserve( utf8StringSize + 1 );

		int skip = 0;
		// Skip BOM
		if ( utf8StringSize >= 3 && (char)0xef == utf8String[0] && (char)0xbb == utf8String[1] &&
			 (char)0xbf == utf8String[2] ) {
			skip = 3;
		}

		Utf8::toUtf32( utf8String + skip, utf8String + utf8StringSize,
					   std::back_inserter( mString ) );
	}
}

String::String( const std::string& utf8String ) {
	mString.reserve( utf8String.length() + 1 );

	int skip = 0;
	// Skip BOM
	if ( utf8String.size() >= 3 && (char)0xef == utf8String[0] && (char)0xbb == utf8String[1] &&
		 (char)0xbf == utf8String[2] ) {
		skip = 3;
	}

	Utf8::toUtf32( utf8String.begin() + skip, utf8String.end(), std::back_inserter( mString ) );
}

String::String( const std::string_view& utf8String ) {
	mString.reserve( utf8String.length() + 1 );

	int skip = 0;
	// Skip BOM
	if ( utf8String.size() >= 3 && (char)0xef == utf8String[0] && (char)0xbb == utf8String[1] &&
		 (char)0xbf == utf8String[2] ) {
		skip = 3;
	}

	Utf8::toUtf32( utf8String.begin() + skip, utf8String.end(), std::back_inserter( mString ) );
}

#ifndef EE_NO_WIDECHAR
String::String( const wchar_t* wideString ) {
	if ( wideString ) {
		std::size_t length = std::wcslen( wideString );
		if ( length > 0 ) {
			mString.reserve( length + 1 );
			Utf32::fromWide( wideString, wideString + length, std::back_inserter( mString ) );
		}
	}
}

String::String( const std::wstring& wideString ) {
	mString.reserve( wideString.length() + 1 );
	Utf32::fromWide( wideString.begin(), wideString.end(), std::back_inserter( mString ) );
}

String String::fromWide( const wchar_t* wideString ) {
	return String( wideString );
}
#endif

String::String( const StringBaseType* utf32String ) {
	if ( utf32String )
		mString = utf32String;
}

String::String( const StringType& utf32String ) : mString( utf32String ) {}

String::String( const String& str ) : mString( str.mString ) {}

String::String( const String::View& utf32String ) : mString( utf32String ) {}

String String::fromUtf16( const char* utf16String, const size_t& utf16StringSize,
						  bool isBigEndian ) {
	int skip = 0;
	// Skip BOM
	if ( utf16StringSize >= 2 ) {
		if ( ( (char)0xFF == utf16String[0] && (char)0xFE == utf16String[1] ) ||
			 ( (char)0xFE == utf16String[0] && (char)0xFF == utf16String[1] ) ) {
			skip = 2;
		}
	}

	String::StringType utf32;
	Utf16::toUtf32( reinterpret_cast<const Uint16*>( utf16String + skip ),
					reinterpret_cast<const Uint16*>( utf16String + utf16StringSize ),
					std::back_inserter( utf32 ), isBigEndian );
	return String( utf32 );
}

String String::fromLatin1( const char* str, const size_t& stringSize ) {
	String::StringType utf32;
	utf32.reserve( stringSize );
	for ( size_t i = 0; i < stringSize; i++ )
		utf32.push_back( static_cast<Uint8>( str[i] ) );
	return String( utf32 );
}

String String::fromUtf8( const std::string& utf8String ) {
	String::StringType utf32;

	// Skip BOM
	int skip = 0;
	if ( utf8String.size() >= 3 && (char)0xef == utf8String[0] && (char)0xbb == utf8String[1] &&
		 (char)0xbf == utf8String[2] ) {
		skip = 3;
	}

	utf32.reserve( utf8String.length() + 1 );

	Utf8::toUtf32( utf8String.begin() + skip, utf8String.end(), std::back_inserter( utf32 ) );

	return String( utf32 );
}

String String::fromUtf8( const std::string_view& utf8String ) {
	String::StringType utf32;

	// Skip BOM
	int skip = 0;
	if ( utf8String.size() >= 3 && (char)0xef == utf8String[0] && (char)0xbb == utf8String[1] &&
		 (char)0xbf == utf8String[2] ) {
		skip = 3;
	}

	utf32.reserve( utf8String.length() + 1 );

	Utf8::toUtf32( utf8String.begin() + skip, utf8String.end(), std::back_inserter( utf32 ) );

	return String( utf32 );
}

#define iscont( p ) ( ( *( p ) & 0xC0 ) == 0x80 )

static inline const char* utf8_next( const char* s, const char* e ) {
	while ( s < e && iscont( s + 1 ) )
		++s;
	return s < e ? s + 1 : e;
}

static inline size_t utf8_length( const char* s, const char* e ) {
	size_t i = 0;
	for ( i = 0; s < e; ++i )
		s = utf8_next( s, e );
	return i;
}

size_t String::utf8Length( const std::string& utf8String ) {
	return utf8_length( utf8String.c_str(), utf8String.c_str() + utf8String.length() );
}

size_t String::utf8Length( const std::string_view& utf8String ) {
	return utf8_length( utf8String.data(), utf8String.data() + utf8String.length() );
}

Uint32 String::utf8Next( char*& utf8String ) {
	return utf8::unchecked::next( utf8String );
}

size_t String::utf8ToCodepointPosition( const std::string_view& utf8Text, size_t utf8Pos ) {
	size_t codepointPos = 0;
	size_t bytePos = 0;

	while ( bytePos < utf8Pos && bytePos < utf8Text.length() ) {
		unsigned char c = utf8Text[bytePos];

		// Skip continuation bytes (10xxxxxx)
		if ( ( c & 0xC0 ) == 0x80 ) {
			bytePos++;
			continue;
		}

		// Count this as one codepoint
		codepointPos++;

		// Move to the next character
		if ( c < 0x80 )
			bytePos += 1; // ASCII
		else if ( c < 0xE0 )
			bytePos += 2; // 2-byte sequence
		else if ( c < 0xF0 )
			bytePos += 3; // 3-byte sequence
		else
			bytePos += 4; // 4-byte sequence

		// Handle potential truncated sequences
		if ( bytePos > utf8Text.length() )
			break;
	}

	// If we're in the middle of a character, return the previous complete character position
	if ( utf8Pos > bytePos )
		codepointPos--;

	return codepointPos;
}

String::operator std::string() const {
	return toUtf8();
}

#ifndef EE_NO_WIDECHAR
std::wstring String::toWideString() const {
	// Prepare the output string
	std::wstring output;
	output.reserve( mString.length() + 1 );

	// Convert
	Utf32::toWide( mString.begin(), mString.end(), std::back_inserter( output ), 0 );

	return output;
}
#endif

std::string String::toUtf8() const {
	// Prepare the output string
	std::string output;
	output.reserve( mString.length() + 1 );

	// Convert
	Utf32::toUtf8( mString.begin(), mString.end(), std::back_inserter( output ) );

	return output;
}

std::basic_string<char16_t> String::toUtf16() const {
	// Prepare the output string
	std::basic_string<char16_t> output;
	output.reserve( mString.length() );

	// Convert
	Utf32::toUtf16( mString.begin(), mString.end(), std::back_inserter( output ) );

	return output;
}

String::HashType String::getHash() const {
	return String::hash( *this );
}

String& String::operator=( const String& right ) {
	mString = right.mString;
	return *this;
}

String& String::operator=( String&& right ) {
	mString = std::move( right.mString );
	return *this;
}

String& String::operator=( const StringBaseType& right ) {
	mString = right;
	return *this;
}

String& String::operator+=( const String& right ) {
	mString += right.mString;
	return *this;
}

String& String::operator+=( const StringBaseType& right ) {
	mString += right;
	return *this;
}

const String::StringBaseType& String::operator[]( std::size_t index ) const {
	return mString[index];
}

String::StringBaseType& String::operator[]( std::size_t index ) {
	return mString[index];
}

const String::StringBaseType& String::at( std::size_t index ) const {
	return mString.at( index );
}

void String::push_back( StringBaseType c ) {
	mString.push_back( c );
}

void String::swap( String& str ) {
	mString.swap( str.mString );
}

void String::clear() {
	mString.clear();
}

std::size_t String::size() const {
	return mString.size();
}

std::size_t String::length() const {
	return mString.length();
}

bool String::empty() const {
	return mString.empty();
}

void String::erase( std::size_t position, std::size_t count ) {
	mString.erase( position, count );
}

String& String::insert( std::size_t position, const String& str ) {
	mString.insert( position, str.mString );
	return *this;
}

String& String::insert( std::size_t pos1, const String& str, std::size_t pos2, std::size_t n ) {
	mString.insert( pos1, str.mString, pos2, n );
	return *this;
}

String& String::insert( size_t pos1, const char* s, size_t n ) {
	String tmp( s );

	mString.insert( pos1, tmp.data(), n );

	return *this;
}

String& String::insert( size_t pos1, size_t n, const StringBaseType& c ) {
	mString.insert( pos1, n, c );
	return *this;
}

String& String::insert( size_t pos1, const char* s ) {
	String tmp( s );

	mString.insert( pos1, tmp.data() );

	return *this;
}

String::Iterator String::insert( Iterator p, const String::StringBaseType& c ) {
	return mString.insert( p, c );
}

void String::insert( Iterator p, size_t n, const String::StringBaseType& c ) {
	mString.insert( p, n, c );
}

const String::StringBaseType* String::c_str() const {
	return mString.c_str();
}

const String::StringBaseType* String::data() const {
	return mString.data();
}

String::Iterator String::begin() {
	return mString.begin();
}

String::ConstIterator String::begin() const {
	return mString.begin();
}

String::Iterator String::end() {
	return mString.end();
}

String::ConstIterator String::end() const {
	return mString.end();
}

String::ReverseIterator String::rbegin() {
	return mString.rbegin();
}

String::ConstReverseIterator String::rbegin() const {
	return mString.rbegin();
}

String::ReverseIterator String::rend() {
	return mString.rend();
}

String::ConstReverseIterator String::rend() const {
	return mString.rend();
}

void String::resize( std::size_t n, StringBaseType c ) {
	mString.resize( n, c );
}

void String::resize( std::size_t n ) {
	mString.resize( n );
}

std::size_t String::max_size() const {
	return mString.max_size();
}

void String::reserve( size_t res_arg ) {
	mString.reserve( res_arg );
}

std::size_t String::capacity() const {
	return mString.capacity();
}

String& String::assign( const String& str ) {
	mString.assign( str.mString );
	return *this;
}

String& String::assign( const String& str, size_t pos, size_t n ) {
	mString.assign( str.mString, pos, n );
	return *this;
}

String& String::assign( const char* s ) {
	String tmp( s );

	mString.assign( tmp.mString );

	return *this;
}

String& String::assign( size_t n, StringBaseType c ) {
	mString.assign( n, c );

	return *this;
}

String& String::append( const String& str ) {
	mString.append( str.mString );

	return *this;
}

String& String::append( const String& str, size_t pos, size_t n ) {
	mString.append( str.mString, pos, n );

	return *this;
}

String& String::append( const char* s ) {
	String tmp( s );

	mString.append( tmp.mString );

	return *this;
}

String& String::append( size_t n, char c ) {
	mString.append( n, c );

	return *this;
}

String& String::append( std::size_t n, StringBaseType c ) {
	mString.append( n, c );

	return *this;
}

String& String::replace( size_t pos1, size_t n1, const String& str ) {
	mString.replace( pos1, n1, str.mString );

	return *this;
}

String& String::replace( Iterator i1, Iterator i2, const String& str ) {
	mString.replace( i1, i2, str.mString );

	return *this;
}

String& String::replace( size_t pos1, size_t n1, const String& str, size_t pos2, size_t n2 ) {
	mString.replace( pos1, n1, str.mString, pos2, n2 );

	return *this;
}

String& String::replace( size_t pos1, size_t n1, const char* s, size_t n2 ) {
	String tmp( s );

	mString.replace( pos1, n1, tmp.data(), n2 );

	return *this;
}

String& String::replace( Iterator i1, Iterator i2, const char* s, size_t n2 ) {
	String tmp( s );

	mString.replace( i1, i2, tmp.data(), n2 );

	return *this;
}

String& String::replace( size_t pos1, size_t n1, const char* s ) {
	String tmp( s );

	mString.replace( pos1, n1, tmp.mString );

	return *this;
}

String& String::replace( Iterator i1, Iterator i2, const char* s ) {
	String tmp( s );

	mString.replace( i1, i2, tmp.mString );

	return *this;
}

String& String::replace( size_t pos1, size_t n1, size_t n2, StringBaseType c ) {
	mString.replace( pos1, n1, n2, (StringBaseType)c );

	return *this;
}

String& String::replace( Iterator i1, Iterator i2, size_t n2, StringBaseType c ) {
	mString.replace( i1, i2, n2, (StringBaseType)c );

	return *this;
}

std::size_t String::find( const String& str, std::size_t start ) const {
	return mString.find( str.mString, start );
}

std::size_t String::find( const char* s, std::size_t pos ) const {
	return find( String( s ), pos );
}

size_t String::find( const StringBaseType& c, std::size_t pos ) const {
	return mString.find( c, pos );
}

std::size_t String::rfind( const String& str, std::size_t pos ) const {
	return mString.rfind( str.mString, pos );
}

std::size_t String::rfind( const char* s, std::size_t pos ) const {
	return rfind( String( s ), pos );
}

std::size_t String::rfind( const StringBaseType& c, std::size_t pos ) const {
	return mString.rfind( c, pos );
}

std::size_t String::copy( StringBaseType* s, std::size_t n, std::size_t pos ) const {
	return mString.copy( s, n, pos );
}

String String::substr( std::size_t pos, std::size_t n ) const {
	return mString.substr( pos, n );
}

int String::compare( const String& str ) const {
	return mString.compare( str.mString );
}

int String::compare( const char* s ) const {
	return compare( String( s ) );
}

int String::compare( std::size_t pos1, std::size_t n1, const String& str ) const {
	return mString.compare( pos1, n1, str.mString );
}

int String::compare( std::size_t pos1, std::size_t n1, const char* s ) const {
	return compare( pos1, n1, String( s ) );
}

int String::compare( std::size_t pos1, std::size_t n1, const String& str, std::size_t pos2,
					 std::size_t n2 ) const {
	return mString.compare( pos1, n1, str.mString, pos2, n2 );
}

int String::compare( std::size_t pos1, std::size_t n1, const char* s, std::size_t n2 ) const {
	return compare( pos1, n1, String( s ), 0, n2 );
}

std::size_t String::find_first_of( const String& str, std::size_t pos ) const {
	return mString.find_first_of( str.mString, pos );
}

std::size_t String::find_first_of( const char* s, std::size_t pos ) const {
	return find_first_of( String( s ), pos );
}

std::size_t String::find_first_of( StringBaseType c, std::size_t pos ) const {
	return mString.find_first_of( c, pos );
}

std::size_t String::find_last_of( const String& str, std::size_t pos ) const {
	return mString.find_last_of( str.mString, pos );
}

std::size_t String::find_last_of( const char* s, std::size_t pos ) const {
	return find_last_of( String( s ), pos );
}

std::size_t String::find_last_of( StringBaseType c, std::size_t pos ) const {
	return mString.find_last_of( c, pos );
}

std::size_t String::find_first_not_of( const String& str, std::size_t pos ) const {
	return mString.find_first_not_of( str.mString, pos );
}

std::size_t String::find_first_not_of( const char* s, std::size_t pos ) const {
	return find_first_not_of( String( s ), pos );
}

std::size_t String::find_first_not_of( StringBaseType c, std::size_t pos ) const {
	return mString.find_first_not_of( c, pos );
}

std::size_t String::find_last_not_of( const String& str, std::size_t pos ) const {
	return mString.find_last_not_of( str.mString, pos );
}

std::size_t String::find_last_not_of( const char* s, std::size_t pos ) const {
	return find_last_not_of( String( s ), pos );
}

std::size_t String::find_last_not_of( StringBaseType c, std::size_t pos ) const {
	return mString.find_last_not_of( c, pos );
}

size_t String::countChar( String::StringBaseType c ) const {
	return std::count( begin(), end(), c );
}

String& String::padLeft( unsigned int minDigits, String::StringBaseType padChar ) {
	if ( mString.length() < minDigits ) {
		mString.insert( mString.begin(), minDigits - mString.length(), padChar );
	}
	return *this;
}

bool operator==( const String& left, const String& right ) {
	return left.mString == right.mString;
}

bool operator!=( const String& left, const String& right ) {
	return !( left == right );
}

bool operator<( const String& left, const String& right ) {
	return left.mString < right.mString;
}

bool operator>( const String& left, const String& right ) {
	return right < left;
}

bool operator<=( const String& left, const String& right ) {
	return !( right < left );
}

bool operator>=( const String& left, const String& right ) {
	return !( left < right );
}

String operator+( const String& left, const String& right ) {
	String string = left;
	string += right;

	return string;
}

bool String::isWholeWord( const std::string& haystack, const std::string& needle,
						  const Int64& startPos ) {
	return ( 0 == startPos || !( std::isalnum( haystack[startPos - 1] ) ) ) &&
		   ( startPos + needle.size() >= haystack.size() ||
			 !( std::isalnum( haystack[startPos + needle.size()] ) ) );
}

bool String::isWholeWord( const String& haystack, const String& needle, const Int64& startPos ) {
	return ( 0 == startPos || !( isAlphaNum( haystack[startPos - 1] ) ) ) &&
		   ( startPos + needle.size() >= haystack.size() ||
			 !( isAlphaNum( haystack[startPos + needle.size()] ) ) );
}

size_t String::toUtf32( std::string_view utf8str, String::StringBaseType* buffer,
						size_t bufferSize ) {
	auto start = utf8str.data();
	auto end = start + utf8str.size();
	size_t pos = 0;
	while ( start < end && pos < bufferSize )
		buffer[pos++] = utf8::unchecked::next( start );
	return pos;
}

void String::readBySeparator( std::string_view buf,
							  std::function<void( std::string_view )> onSepChunkRead, char sep ) {
	auto lastNL = 0;
	auto nextNL = buf.find_first_of( sep );
	while ( nextNL != std::string_view::npos ) {
		onSepChunkRead( buf.substr( lastNL, nextNL - lastNL ) );
		lastNL = nextNL + 1;
		nextNL = buf.find_first_of( sep, nextNL + 1 );
	}
}

size_t String::countLines( std::string_view text ) {
	const char* startPtr = text.data();
	const char* endPtr = text.data() + text.size();
	size_t count = 0;
	if ( startPtr != endPtr ) {
		count = 1 + *startPtr == '\n' ? 1 : 0;
		while ( ++startPtr && startPtr != endPtr )
			count += ( '\n' == *startPtr ) ? 1 : 0;
	}
	return count;
}

void String::readBySeparator( String::View buf, std::function<void( String::View )> onSepChunkRead,
							  String::StringBaseType sep ) {
	auto lastNL = 0;
	auto nextNL = buf.find_first_of( sep );
	while ( nextNL != String::View::npos ) {
		onSepChunkRead( buf.substr( lastNL, nextNL - lastNL ) );
		lastNL = nextNL + 1;
		nextNL = buf.find_first_of( sep, nextNL + 1 );
	}
}

size_t String::countLines( String::View text ) {
	const String::StringBaseType* startPtr = text.data();
	const String::StringBaseType* endPtr = text.data() + text.size();
	size_t count = 0;
	if ( startPtr != endPtr ) {
		count = 1 + *startPtr == '\n' ? 1 : 0;
		while ( ++startPtr && startPtr != endPtr )
			count += ( '\n' == *startPtr ) ? 1 : 0;
	}
	return count;
}

} // namespace EE
