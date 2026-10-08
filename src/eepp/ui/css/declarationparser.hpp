#ifndef EE_UI_CSS_DECLARATIONPARSER_HPP
#define EE_UI_CSS_DECLARATIONPARSER_HPP

#include <cctype>
#include <eepp/core/small_vector.hpp>
#include <eepp/core/string.hpp>
#include <eepp/system/functionstring.hpp>
#include <eepp/system/time.hpp>

namespace EE { namespace UI { namespace CSS { namespace DeclarationParser {

template <typename Callback> void forEachCommaItem( std::string_view value, Callback&& callback ) {
	System::FunctionString::forEachParameter( value, [&]( std::string_view item, bool ) {
		callback( item );
		return true;
	} );
}

template <std::size_t InlineCapacity>
SmallVector<std::string_view, InlineCapacity> splitWhitespaceTokens( std::string_view value ) {
	SmallVector<std::string_view, InlineCapacity> tokens;
	std::size_t start = 0;
	int parenthesisDepth = 0;
	for ( std::size_t i = 0; i <= value.size(); ++i ) {
		if ( i < value.size() ) {
			if ( value[i] == '(' )
				++parenthesisDepth;
			else if ( value[i] == ')' && parenthesisDepth > 0 )
				--parenthesisDepth;
		}
		if ( i == value.size() ||
			 ( std::isspace( static_cast<unsigned char>( value[i] ) ) && parenthesisDepth == 0 ) ) {
			if ( i > start )
				tokens.emplace_back( value.substr( start, i - start ) );
			while ( i + 1 < value.size() &&
					std::isspace( static_cast<unsigned char>( value[i + 1] ) ) )
				++i;
			start = i + 1;
		}
	}
	return tokens;
}

inline System::Time parseTime( std::string_view value ) {
	value = String::trim( value, " \t\n\r\f\v" );
	auto lower = []( char character ) {
		return character >= 'A' && character <= 'Z' ? character + ( 'a' - 'A' ) : character;
	};
	bool milliseconds = value.size() >= 2 && lower( value[value.size() - 2] ) == 'm' &&
						lower( value.back() ) == 's';
	bool seconds = !milliseconds && !value.empty() && lower( value.back() ) == 's';
	bool minutes = !milliseconds && !seconds && !value.empty() && lower( value.back() ) == 'm';
	std::size_t numberLength = value.size() - ( milliseconds ? 2 : ( seconds || minutes ? 1 : 0 ) );
	double number = 0;
	const char* numberStart = value.data();
	if ( numberLength > 0 && *numberStart == '+' ) {
		++numberStart;
		--numberLength;
	}
	if ( !String::fromString( number, std::string_view{ numberStart, numberLength } ) )
		return System::Time::Zero;
	if ( milliseconds )
		return System::Milliseconds( number );
	if ( minutes )
		return System::Minutes( number );
	return System::Seconds( number );
}

inline String::HashType lowerHash( std::string_view value ) {
	return String::hashToLower( value.data(), static_cast<Int64>( value.size() ) );
}

inline std::string lowerString( std::string_view value ) {
	std::string result{ value };
	String::toLowerInPlace( result );
	return result;
}

}}}} // namespace EE::UI::CSS::DeclarationParser

#endif
