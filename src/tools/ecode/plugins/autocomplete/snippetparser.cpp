#include "snippetparser.hpp"

#include <algorithm>
#include <cctype>
#include <eepp/core/string.hpp>
#include <eepp/system/regex.hpp>
#include <limits>

using namespace EE::System;

namespace ecode {

namespace {

using DefaultsMap = UnorderedMap<Uint32, std::string>;

struct ParseResult {
	SnippetParser::Result result;
	bool closed{ false };
};

class Parser {
  public:
	Parser( std::string_view snippet, const SnippetParser::VariableMap& variables ) :
		mSnippet( snippet ), mVariables( variables ) {}

	SnippetParser::Result parse() {
		size_t pos = 0;
		DefaultsMap defaults;
		auto parsed = parseSequence( pos, false, defaults );
		parsed.result.hasExplicitTabStops = !parsed.result.tabStops.empty();
		Uint32 maxIndex = 0;
		bool hasFinalStop = false;
		for ( const auto& stop : parsed.result.tabStops ) {
			if ( stop.index == 0 && !stop.synthetic ) {
				hasFinalStop = true;
			} else if ( !stop.synthetic )
				maxIndex = eemax( maxIndex, stop.index );
		}
		for ( auto& stop : parsed.result.tabStops )
			if ( stop.synthetic )
				stop.index = maxIndex < std::numeric_limits<Uint32>::max() ? ++maxIndex : maxIndex;
		if ( !hasFinalStop ) {
			const size_t end = parsed.result.codepointLength;
			parsed.result.tabStops.push_back( { 0, end, end } );
		}
		return std::move( parsed.result );
	}

  private:
	std::string_view mSnippet;
	const SnippetParser::VariableMap& mVariables;

	static void appendText( SnippetParser::Result& result, std::string_view text ) {
		result.text.append( text.data(), text.size() );
		result.codepointLength += String::utf8Length( text );
	}

	static void appendResult( SnippetParser::Result& result,
							  const SnippetParser::Result& appended ) {
		const size_t offset = result.codepointLength;
		result.text += appended.text;
		result.codepointLength += appended.codepointLength;
		result.tabStops.reserve( result.tabStops.size() + appended.tabStops.size() );
		for ( const auto& stop : appended.tabStops ) {
			auto translatedStop = stop;
			translatedStop.start += offset;
			translatedStop.end += offset;
			result.tabStops.emplace_back( std::move( translatedStop ) );
		}
	}

	static bool parseIndex( std::string_view snippet, size_t& pos, Uint32& index ) {
		if ( pos >= snippet.size() || snippet[pos] < '0' || snippet[pos] > '9' )
			return false;
		Uint64 value = 0;
		do {
			value = value * 10 + static_cast<Uint64>( snippet[pos] - '0' );
			if ( value > std::numeric_limits<Uint32>::max() )
				return false;
			++pos;
		} while ( pos < snippet.size() && snippet[pos] >= '0' && snippet[pos] <= '9' );
		index = static_cast<Uint32>( value );
		return true;
	}

	static bool parseVariableName( std::string_view snippet, size_t& pos, std::string_view& name ) {
		const size_t start = pos;
		if ( pos >= snippet.size() ||
			 !( snippet[pos] == '_' || ( snippet[pos] >= 'a' && snippet[pos] <= 'z' ) ||
				( snippet[pos] >= 'A' && snippet[pos] <= 'Z' ) ) )
			return false;
		while ( ++pos < snippet.size() &&
				( snippet[pos] == '_' || ( snippet[pos] >= 'a' && snippet[pos] <= 'z' ) ||
				  ( snippet[pos] >= 'A' && snippet[pos] <= 'Z' ) ||
				  ( snippet[pos] >= '0' && snippet[pos] <= '9' ) ) ) {
		}
		name = snippet.substr( start, pos - start );
		return true;
	}

	const std::string* resolveVariable( std::string_view name ) const {
		for ( const auto& variable : mVariables )
			if ( variable.first == name )
				return &variable.second;
		return nullptr;
	}

	static void appendVariable( SnippetParser::Result& result, std::string_view name,
								const std::string* value ) {
		const size_t start = result.codepointLength;
		appendText( result, value ? std::string_view( *value ) : name );
		if ( !value )
			result.tabStops.push_back( { 0, start, result.codepointLength, {}, true } );
	}

	bool parseChoice( size_t& pos, Uint32 index, SnippetParser::Result& result,
					  DefaultsMap& defaults ) {
		std::vector<std::string> choices( 1 );
		while ( pos < mSnippet.size() ) {
			const char ch = mSnippet[pos++];
			if ( ch == '\\' && pos < mSnippet.size() &&
				 ( mSnippet[pos] == ',' || mSnippet[pos] == '|' || mSnippet[pos] == '\\' ) ) {
				choices.back().push_back( mSnippet[pos++] );
			} else if ( ch == ',' ) {
				choices.emplace_back();
			} else if ( ch == '|' && pos < mSnippet.size() && mSnippet[pos] == '}' ) {
				++pos;
				const size_t start = result.codepointLength;
				auto defaultIt = defaults.find( index );
				if ( defaultIt == defaults.end() ) {
					defaults[index] = choices.front();
					appendText( result, choices.front() );
				} else {
					appendText( result, defaultIt->second );
				}
				result.tabStops.push_back(
					{ index, start, result.codepointLength, std::move( choices ) } );
				return true;
			} else {
				choices.back().push_back( ch );
			}
		}
		return false;
	}

	static bool readTransformPart( std::string_view snippet, size_t& pos, std::string& part,
								   char delimiter, bool format ) {
		int braceDepth = 0;
		while ( pos < snippet.size() ) {
			const char ch = snippet[pos++];
			if ( ch == '\\' && pos < snippet.size() ) {
				if ( snippet[pos] == delimiter )
					part.push_back( snippet[pos++] );
				else {
					part.push_back( ch );
					part.push_back( snippet[pos++] );
				}
				continue;
			}
			if ( format && ch == '$' && pos < snippet.size() && snippet[pos] == '{' ) {
				++braceDepth;
				part += "${";
				++pos;
				continue;
			}
			if ( format && ch == '}' && braceDepth > 0 ) {
				--braceDepth;
				part.push_back( ch );
				continue;
			}
			if ( ch == delimiter && braceDepth == 0 )
				return true;
			part.push_back( ch );
		}
		return false;
	}

	static std::string capture( std::string_view value,
								const std::vector<PatternMatcher::Range>& matches, Uint32 index ) {
		if ( index >= matches.size() || matches[index].start < 0 || matches[index].end < 0 )
			return {};
		return std::string( value.substr( matches[index].start, matches[index].length() ) );
	}

	static size_t findUnescaped( std::string_view text, char ch, size_t from = 0 ) {
		for ( size_t pos = from; pos < text.size(); ++pos ) {
			if ( text[pos] == '\\' )
				++pos;
			else if ( text[pos] == ch )
				return pos;
		}
		return std::string_view::npos;
	}

	static std::string unescapeFormatText( std::string_view text ) {
		std::string result;
		result.reserve( text.size() );
		for ( size_t pos = 0; pos < text.size(); ++pos ) {
			if ( text[pos] == '\\' && pos + 1 < text.size() )
				++pos;
			result.push_back( text[pos] );
		}
		return result;
	}

	static std::vector<std::string> caseWords( std::string_view value ) {
		std::vector<std::string> words;
		size_t start = 0;
		for ( size_t pos = 0; pos <= value.size(); ++pos ) {
			const bool atEnd = pos == value.size();
			const Uint8 current = atEnd ? 0 : static_cast<Uint8>( value[pos] );
			const bool separator = !atEnd && current < 0x80 && !std::isalnum( current );
			const bool caseBoundary = !atEnd && pos > start && current < 0x80 &&
									  std::isupper( current ) &&
									  static_cast<Uint8>( value[pos - 1] ) < 0x80 &&
									  std::islower( static_cast<Uint8>( value[pos - 1] ) );
			if ( atEnd || separator || caseBoundary ) {
				if ( pos > start )
					words.emplace_back(
						String::toLower( std::string{ value.substr( start, pos - start ) } ) );
				start = separator ? pos + 1 : pos;
			}
		}
		return words;
	}

	static std::string transformCase( std::string_view value, std::string_view modifier ) {
		auto words = caseWords( value );
		std::string output;
		for ( size_t index = 0; index < words.size(); ++index ) {
			if ( modifier == "/snakecase" && index > 0 )
				output += '_';
			else if ( modifier == "/kebabcase" && index > 0 )
				output += '-';
			if ( modifier == "/pascalcase" || ( modifier == "/camelcase" && index > 0 ) )
				output += String::capitalize( words[index] );
			else
				output += words[index];
		}
		return output;
	}

	static std::string transformFormat( std::string_view format, std::string_view value,
										const std::vector<PatternMatcher::Range>& matches ) {
		std::string output;
		for ( size_t pos = 0; pos < format.size(); ) {
			if ( format[pos] == '\\' && pos + 1 < format.size() ) {
				output.push_back( format[pos + 1] );
				pos += 2;
				continue;
			}
			if ( format[pos] != '$' ) {
				output.push_back( format[pos++] );
				continue;
			}
			const size_t constructStart = pos++;
			const bool braced = pos < format.size() && format[pos] == '{';
			if ( braced )
				++pos;
			Uint32 index = 0;
			if ( !parseIndex( format, pos, index ) ) {
				output.push_back( '$' );
				pos = constructStart + 1;
				continue;
			}
			std::string group = capture( value, matches, index );
			if ( !braced ) {
				output += group;
				continue;
			}
			if ( pos < format.size() && format[pos] == '}' ) {
				++pos;
				output += group;
				continue;
			}
			if ( pos >= format.size() || format[pos++] != ':' ) {
				output.append( format.substr( constructStart, pos - constructStart ) );
				continue;
			}
			const size_t bodyStart = pos;
			while ( pos < format.size() && format[pos] != '}' ) {
				if ( format[pos] == '\\' && pos + 1 < format.size() )
					pos += 2;
				else
					++pos;
			}
			if ( pos >= format.size() ) {
				output.append( format.substr( constructStart ) );
				break;
			}
			const auto body = format.substr( bodyStart, pos++ - bodyStart );
			if ( body == "/upcase" )
				output += String::toUpper( group );
			else if ( body == "/downcase" )
				output += String::toLower( group );
			else if ( body == "/capitalize" )
				output += String::capitalize( group );
			else if ( body == "/camelcase" || body == "/pascalcase" || body == "/snakecase" ||
					  body == "/kebabcase" )
				output += transformCase( group, body );
			else if ( String::startsWith( body, "+" ) )
				output += !group.empty() ? unescapeFormatText( body.substr( 1 ) ) : "";
			else if ( String::startsWith( body, "?" ) ) {
				const size_t separator = findUnescaped( body, ':', 1 );
				output += !group.empty() ? unescapeFormatText( body.substr( 1, separator - 1 ) )
						  : separator != std::string_view::npos
							  ? unescapeFormatText( body.substr( separator + 1 ) )
							  : "";
			} else if ( String::startsWith( body, "-" ) )
				output += group.empty() ? unescapeFormatText( body.substr( 1 ) ) : "";
			else
				output += group.empty() ? unescapeFormatText( body ) : group;
		}
		return output;
	}

	static std::string applyTransform( std::string_view value, const std::string& pattern,
									   const std::string& format, std::string_view options ) {
		Uint32 regexOptions = RegEx::Options::Utf | RegEx::Options::AllowFallback;
		if ( options.find( 'i' ) != std::string_view::npos )
			regexOptions |= RegEx::Options::Caseless;
		if ( options.find( 'm' ) != std::string_view::npos )
			regexOptions |= RegEx::Options::Multiline;
		if ( options.find( 's' ) != std::string_view::npos )
			regexOptions |= RegEx::Options::Dotall;
		RegEx regex( pattern, regexOptions, false );
		if ( !regex.isValid() )
			return std::string( value );
		const bool global = options.find( 'g' ) != std::string_view::npos;
		std::vector<PatternMatcher::Range> matches( regex.getCaptureCount() + 1 );
		std::string output;
		size_t outputOffset = 0;
		size_t searchOffset = 0;
		bool matched = false;
		while ( searchOffset <= value.size() &&
				regex.matches( value.data(), static_cast<int>( searchOffset ), matches.data(),
							   value.size() ) ) {
			const auto& match = matches[0];
			if ( match.start < 0 || match.end < match.start )
				break;
			matched = true;
			output.append(
				value.substr( outputOffset, static_cast<size_t>( match.start ) - outputOffset ) );
			output += transformFormat( format, value, matches );
			outputOffset = static_cast<size_t>( match.end );
			searchOffset = outputOffset;
			if ( !global )
				break;
			if ( match.start == match.end ) {
				if ( searchOffset >= value.size() )
					break;
				char* next = const_cast<char*>( value.data() + searchOffset );
				String::utf8Next( next );
				searchOffset = static_cast<size_t>( next - value.data() );
			}
		}
		if ( !matched )
			return std::string( value );
		output.append( value.substr( outputOffset ) );
		return output;
	}

	bool parseTransform( size_t& pos, std::string_view value, SnippetParser::Result& result ) {
		std::string pattern;
		std::string format;
		if ( !readTransformPart( mSnippet, pos, pattern, '/', false ) ||
			 !readTransformPart( mSnippet, pos, format, '/', true ) )
			return false;
		const size_t optionsStart = pos;
		while ( pos < mSnippet.size() && mSnippet[pos] != '}' )
			++pos;
		if ( pos >= mSnippet.size() )
			return false;
		const auto options = mSnippet.substr( optionsStart, pos - optionsStart );
		++pos;
		appendText( result, applyTransform( value, pattern, format, options ) );
		return true;
	}

	static void appendTabStop( SnippetParser::Result& result, Uint32 index,
							   const DefaultsMap& defaults ) {
		const size_t start = result.codepointLength;
		auto defaultIt = defaults.find( index );
		if ( defaultIt != defaults.end() ) {
			result.text += defaultIt->second;
			result.codepointLength += String::utf8Length( defaultIt->second );
		}
		result.tabStops.push_back( { index, start, result.codepointLength } );
	}

	bool parseDollar( size_t& pos, SnippetParser::Result& result, DefaultsMap& defaults ) {
		const size_t constructStart = pos;
		++pos;
		Uint32 index = 0;
		if ( parseIndex( mSnippet, pos, index ) ) {
			appendTabStop( result, index, defaults );
			return true;
		}
		std::string_view variableName;
		if ( parseVariableName( mSnippet, pos, variableName ) ) {
			appendVariable( result, variableName, resolveVariable( variableName ) );
			return true;
		}
		if ( pos >= mSnippet.size() || mSnippet[pos] != '{' ) {
			pos = constructStart;
			return false;
		}

		++pos;
		if ( parseIndex( mSnippet, pos, index ) ) {
			if ( pos < mSnippet.size() && mSnippet[pos] == '}' ) {
				++pos;
				appendTabStop( result, index, defaults );
				return true;
			}
			if ( pos < mSnippet.size() && mSnippet[pos] == '|' ) {
				++pos;
				DefaultsMap choiceDefaults = defaults;
				if ( parseChoice( pos, index, result, choiceDefaults ) ) {
					defaults = std::move( choiceDefaults );
					return true;
				}
				pos = constructStart;
				return false;
			}
			if ( pos >= mSnippet.size() || mSnippet[pos] != ':' ) {
				pos = constructStart;
				return false;
			}

			++pos;
			DefaultsMap placeholderDefaults = defaults;
			auto placeholder = parseSequence( pos, true, placeholderDefaults );
			if ( !placeholder.closed ) {
				pos = constructStart;
				return false;
			}
			defaults = std::move( placeholderDefaults );
			const size_t start = result.codepointLength;
			auto defaultIt = defaults.find( index );
			if ( defaultIt == defaults.end() ) {
				defaults[index] = placeholder.result.text;
				appendResult( result, placeholder.result );
			} else {
				appendText( result, defaultIt->second );
			}
			result.tabStops.push_back( { index, start, result.codepointLength } );
			return true;
		}

		if ( !parseVariableName( mSnippet, pos, variableName ) ) {
			pos = constructStart;
			return false;
		}
		const std::string* value = resolveVariable( variableName );
		if ( pos < mSnippet.size() && mSnippet[pos] == '}' ) {
			++pos;
			appendVariable( result, variableName, value );
			return true;
		}
		if ( pos < mSnippet.size() && mSnippet[pos] == '/' ) {
			++pos;
			if ( parseTransform( pos, value ? std::string_view( *value ) : std::string_view{},
								 result ) )
				return true;
			pos = constructStart;
			return false;
		}
		if ( pos >= mSnippet.size() || mSnippet[pos] != ':' ) {
			pos = constructStart;
			return false;
		}

		++pos;
		DefaultsMap variableDefaults = defaults;
		auto fallback = parseSequence( pos, true, variableDefaults );
		if ( !fallback.closed ) {
			pos = constructStart;
			return false;
		}
		if ( value ) {
			appendText( result, *value );
		} else {
			defaults = std::move( variableDefaults );
			appendResult( result, fallback.result );
		}
		return true;
	}

	ParseResult parseSequence( size_t& pos, bool stopAtClosingBrace, DefaultsMap& defaults ) {
		ParseResult parsed;
		size_t textStart = pos;
		while ( pos < mSnippet.size() ) {
			const char ch = mSnippet[pos];
			if ( stopAtClosingBrace && ch == '}' ) {
				appendText( parsed.result, mSnippet.substr( textStart, pos - textStart ) );
				++pos;
				parsed.closed = true;
				return parsed;
			}
			if ( ch == '\\' && pos + 1 < mSnippet.size() &&
				 ( mSnippet[pos + 1] == '$' || mSnippet[pos + 1] == '}' ||
				   mSnippet[pos + 1] == '\\' ) ) {
				appendText( parsed.result, mSnippet.substr( textStart, pos - textStart ) );
				appendText( parsed.result, mSnippet.substr( pos + 1, 1 ) );
				pos += 2;
				textStart = pos;
				continue;
			}
			if ( ch == '$' ) {
				appendText( parsed.result, mSnippet.substr( textStart, pos - textStart ) );
				if ( parseDollar( pos, parsed.result, defaults ) ) {
					textStart = pos;
					continue;
				}
				++pos;
				textStart = pos - 1;
				continue;
			}
			++pos;
		}
		appendText( parsed.result, mSnippet.substr( textStart, pos - textStart ) );
		return parsed;
	}
};

} // namespace

SnippetParser::Result SnippetParser::parse( std::string_view snippet,
											const VariableMap& variables ) {
	return Parser( snippet, variables ).parse();
}

} // namespace ecode
