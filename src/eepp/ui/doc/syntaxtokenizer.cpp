#include <eepp/system/log.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/system/regex.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>
#include <eepp/ui/doc/syntaxtokenizer.hpp>
#include <variant>

using namespace EE::System;

namespace EE { namespace UI { namespace Doc {

// This tokenizer was a direct conversion to C++ from the lite (https://github.com/rxi/lite)
// tokenizer. This allows eepp to support the same color schemes and syntax definitions from
// lite. Making much easier to implement a complete code editor. Currently some improvements
// has been made. It's still compatible with lite tokenizer.

// Max token size limits the length of each token for cases where there's a single token for a very
// large line. This will help the editor to cull the rendering only for the visible tokens
#define MAX_TOKEN_SIZE ( 512 )

static int isInMultiByteCodePoint( const char* text, const size_t& textSize, const size_t& pos ) {
	// current char is a multybyte codepoint
	if ( ( text[pos] & 0xC0 ) == 0x80 ) {
		int nextCodePoint = 1;
		while ( pos + nextCodePoint < textSize ) {
			// search the start of the next codepoint
			if ( ( text[pos + nextCodePoint] & 0xC0 ) != 0x80 )
				return nextCodePoint;
			++nextCodePoint;
		}
	}
	return 0;
}

template <typename T>
static void pushToken( std::vector<T>& tokens, const SyntaxStyleType& type,
					   const std::string_view& text ) {
	if ( !tokens.empty() && ( tokens[tokens.size() - 1].type == type ) ) {
		size_t tpos = tokens.size() - 1;
		tokens[tpos].type = type;
		if constexpr ( std::is_same_v<T, SyntaxTokenComplete> )
			tokens[tpos].text += text;
		tokens[tpos].len += String::utf8Length( text );
	} else {
		if ( text.size() > MAX_TOKEN_SIZE ) {
			size_t textSize = text.size();
			size_t pos = 0;

			while ( textSize > 0 ) {
				size_t chunkSize = textSize > MAX_TOKEN_SIZE ? MAX_TOKEN_SIZE : textSize;
				int multiByteCodePointPos = 0;
				if ( ( multiByteCodePointPos = isInMultiByteCodePoint( text.data(), text.size(),
																	   pos + chunkSize ) ) > 0 ) {
					chunkSize = eemin( textSize, chunkSize + multiByteCodePointPos );
				}
				std::string_view substr = text.substr( pos, chunkSize );
				SyntaxStyleType len = String::utf8Length( substr );
				if constexpr ( std::is_same_v<T, SyntaxTokenComplete> ) {
					tokens.push_back( { type, std::string{ substr }, len } );
				} else if constexpr ( std::is_same_v<T, SyntaxTokenPosition> ) {
					SyntaxStyleType tpos = tokens.empty() ? 0
														  : tokens[tokens.size() - 1].pos +
																tokens[tokens.size() - 1].len;
					tokens.push_back( { type, tpos, len } );
				} else {
					tokens.push_back( { type, len } );
				}
				textSize -= chunkSize;
				pos += chunkSize;
			}
		} else {
			if constexpr ( std::is_same_v<T, SyntaxTokenComplete> ) {
				tokens.push_back( { type, std::string{ text },
									static_cast<SyntaxTokenLen>( String::utf8Length( text ) ) } );
			} else if constexpr ( std::is_same_v<T, SyntaxTokenPosition> ) {
				SyntaxStyleType tpos =
					tokens.empty() ? 0
								   : tokens[tokens.size() - 1].pos + tokens[tokens.size() - 1].len;
				tokens.push_back(
					{ type, tpos, static_cast<SyntaxTokenLen>( String::utf8Length( text ) ) } );
			} else {
				tokens.push_back(
					{ type, static_cast<SyntaxTokenLen>( String::utf8Length( text ) ) } );
			}
		}
	}
}

static bool isScaped( const std::string& text, const size_t& startIndex,
					  const std::string& escapeStr ) {
	char escapeByte = escapeStr.empty() ? '\\' : escapeStr[0];
	int count = 0;
	for ( int i = startIndex - 1; i >= 0; i-- ) {
		if ( text[i] != escapeByte )
			break;
		count++;
	}
	return count % 2 == 1;
}

static std::pair<int, int> findNonEscaped( const std::string& text, const std::string& pattern,
										   int offset, const std::string& escapeStr,
										   bool isRegEx ) {
	eeASSERT( !pattern.empty() );
	if ( pattern.empty() )
		return std::make_pair( -1, -1 );
	std::variant<RegEx, LuaPattern> wordsVar =
		isRegEx ? std::variant<RegEx, LuaPattern>( RegEx( pattern ) )
				: std::variant<RegEx, LuaPattern>( LuaPattern( pattern ) );
	PatternMatcher& words =
		std::visit( []( auto& patternType ) -> PatternMatcher& { return patternType; }, wordsVar );
	int start, end;
	while ( words.find( text, start, end, offset ) ) {
		if ( !escapeStr.empty() && isScaped( text, start, escapeStr ) ) {
			offset = end;
		} else {
			return std::make_pair( start, end );
		}
	}
	return std::make_pair( -1, -1 );
}

SyntaxStateRestored SyntaxTokenizer::retrieveSyntaxState( const SyntaxDefinition& syntax,
														  const SyntaxState& state ) {
	SyntaxStateRestored syntaxState{ &syntax, nullptr, state.state[0], 0 };
	if ( state.state[0] > 0 &&
		 ( state.state[1] > 0 ||
		   ( state.state[0] < syntaxState.currentSyntax->getPatterns().size() &&
			 syntaxState.currentSyntax->getPatterns()[state.state[0] - 1].hasSyntax() ) ) ) {
		for ( size_t i = 0; i <= 2; ++i ) {
			Uint32 target = state.state[i];
			if ( target != SYNTAX_TOKENIZER_STATE_NONE ) {
				if ( target < syntaxState.currentSyntax->getPatterns().size() &&
					 syntaxState.currentSyntax->getPatterns()[target - 1].hasSyntax() ) {
					syntaxState.subsyntaxInfo =
						&syntaxState.currentSyntax->getPatterns()[target - 1];
					Uint32 langIndex = state.langStack[i];
					syntaxState.currentSyntax =
						langIndex != 0
							? &SyntaxDefinitionManager::instance()->getByLanguageIndex( langIndex )
							: &SyntaxDefinitionManager::instance()->getByLanguageName(
								  syntaxState.subsyntaxInfo->syntax );
					syntaxState.currentPatternIdx = SYNTAX_TOKENIZER_STATE_NONE;
					syntaxState.currentLevel++;
				} else {
					syntaxState.currentPatternIdx = target;
				}
			} else {
				break;
			}
		}
	}
	return syntaxState;
}

static inline void setSubsyntaxPatternIdx( SyntaxStateRestored& curState, SyntaxState& retState,
										   const Uint32& patternIndex ) {
	curState.currentPatternIdx = patternIndex;
	retState.state[curState.currentLevel] = patternIndex;
};

static inline void pushSubsyntax( SyntaxStateRestored& curState, SyntaxState& retState,
								  const SyntaxPattern& enteringSubsyntax,
								  const Uint32& patternIndex, const std::string& patternStr ) {
	if ( curState.currentLevel == MAX_SUB_SYNTAXS - 1 )
		return;
	setSubsyntaxPatternIdx( curState, retState, patternIndex );
	curState.subsyntaxInfo = &enteringSubsyntax;
	curState.currentSyntax = &SyntaxDefinitionManager::instance()->getByLanguageName(
		curState.subsyntaxInfo->dynSyntax
			? curState.subsyntaxInfo->dynSyntax( enteringSubsyntax, patternStr )
			: curState.subsyntaxInfo->syntax );
	retState.langStack[curState.currentLevel] = curState.currentSyntax->getLanguageIndex();
	curState.currentLevel++;
	setSubsyntaxPatternIdx( curState, retState, SYNTAX_TOKENIZER_STATE_NONE );
};

static inline void popSubsyntax( SyntaxStateRestored& curState, SyntaxState& retState,
								 const SyntaxDefinition& syntax ) {
	setSubsyntaxPatternIdx( curState, retState, SYNTAX_TOKENIZER_STATE_NONE );
	retState.langStack[curState.currentLevel] = 0;
	curState.currentLevel--;
	setSubsyntaxPatternIdx( curState, retState, SYNTAX_TOKENIZER_STATE_NONE );
	curState = SyntaxTokenizer::retrieveSyntaxState( syntax, retState );
};

template <typename T>
static inline std::pair<std::vector<T>, SyntaxState>
_tokenize( const SyntaxDefinition& syntax, const std::string& text, const SyntaxState& state,
		   const size_t& startIndex, bool skipSubSyntaxSeparator ) {
	std::vector<T> tokens;

	if ( syntax.getPatterns().empty() ) {
		pushToken( tokens, SyntaxStyleTypes::Normal, text );
		return std::make_pair( std::move( tokens ), SyntaxState{} );
	}

	PatternMatcher::Range matches[12];
	int start, end;
	size_t numMatches;
	size_t i = startIndex;
	SyntaxState retState = state;
	SyntaxStateRestored curState = SyntaxTokenizer::retrieveSyntaxState( syntax, state );

	size_t size = text.size();
	std::string patternStr;
	std::string patternText;

	while ( i < size ) {
		if ( curState.currentPatternIdx != SYNTAX_TOKENIZER_STATE_NONE ) {
			const SyntaxPattern& pattern =
				curState.currentSyntax->getPatterns()[curState.currentPatternIdx - 1];
			std::pair<int, int> range = findNonEscaped(
				text, pattern.patterns[1], i,
				pattern.patterns.size() >= 3 ? pattern.patterns[2] : "", pattern.isRegEx );

			bool skip = false;

			if ( curState.subsyntaxInfo != nullptr ) {
				std::pair<int, int> rangeSubsyntax =
					findNonEscaped( text, curState.subsyntaxInfo->patterns[1], i,
									curState.subsyntaxInfo->patterns.size() >= 3
										? curState.subsyntaxInfo->patterns[2]
										: "",
									pattern.isRegEx );

				if ( rangeSubsyntax.first != -1 &&
					 ( range.first == -1 || rangeSubsyntax.first < range.first ) ) {
					if ( !skipSubSyntaxSeparator ) {
						pushToken( tokens, curState.subsyntaxInfo->types[0],
								   text.substr( i, rangeSubsyntax.second - i ) );
					}
					popSubsyntax( curState, retState, syntax );
					i = rangeSubsyntax.second;
					skip = true;
				}
			}

			if ( !skip ) {
				if ( range.first != -1 ) {
					if ( range.second > range.first && pattern.types.size() >= 3 ) {
						pushToken( tokens, pattern.types[0], text.substr( i, range.first - i ) );
						pushToken( tokens, pattern.types[pattern.types.size() - 1],
								   text.substr( range.first, range.second - range.first ) );
					} else {
						pushToken( tokens, pattern.types[0], text.substr( i, range.second - i ) );
					}
					setSubsyntaxPatternIdx( curState, retState, SYNTAX_TOKENIZER_STATE_NONE );
					i = range.second;
				} else {
					pushToken( tokens, pattern.types[0], text.substr( i ) );
					break;
				}
			}
		}

		if ( curState.subsyntaxInfo != nullptr ) {
			std::pair<int, int> rangeSubsyntax = findNonEscaped(
				text, "^" + curState.subsyntaxInfo->patterns[1], i,
				curState.subsyntaxInfo->patterns.size() >= 3 ? curState.subsyntaxInfo->patterns[2]
															 : "",
				curState.subsyntaxInfo->isRegEx );

			if ( rangeSubsyntax.first != -1 ) {
				if ( !skipSubSyntaxSeparator ) {
					pushToken( tokens, curState.subsyntaxInfo->types[0],
							   text.substr( i, rangeSubsyntax.second - i ) );
				}
				popSubsyntax( curState, retState, syntax );
				i = rangeSubsyntax.second;
			}
		}

		bool matched = false;
		size_t patternsCount = curState.currentSyntax->getPatterns().size();

		for ( size_t patternIndex = 0; patternIndex < patternsCount; patternIndex++ ) {
			const SyntaxPattern& pattern = curState.currentSyntax->getPatterns()[patternIndex];
			if ( i != 0 && pattern.patterns[0][0] == '^' )
				continue;
			patternStr =
				pattern.patterns[0][0] == '^' ? pattern.patterns[0] : "^" + pattern.patterns[0];
			std::variant<RegEx, LuaPattern> wordsVar =
				pattern.isRegEx ? std::variant<RegEx, LuaPattern>( RegEx( patternStr ) )
								: std::variant<RegEx, LuaPattern>( LuaPattern( patternStr ) );
			PatternMatcher& words = std::visit(
				[]( auto& patternType ) -> PatternMatcher& { return patternType; }, wordsVar );
			if ( !words.isValid() ) // Skip invalid patterns
				continue;
			if ( words.matches( text, matches, i ) && ( numMatches = words.getNumMatches() ) > 0 ) {
				if ( numMatches > 1 ) {
					int patternMatchStart = matches[0].start;
					int patternMatchEnd = matches[0].end;
					std::string patternFullText(
						text.substr( patternMatchStart, patternMatchEnd - patternMatchStart ) );
					auto patternType = pattern.types[0];
					int lastStart = patternMatchStart;
					int lastEnd = patternMatchEnd;

					for ( size_t curMatch = 1; curMatch < numMatches; curMatch++ ) {
						start = matches[curMatch].start;
						end = matches[curMatch].end;
						if ( start == end && matches[curMatch - 1].end == start )
							continue;
						if ( pattern.patterns.size() >= 3 && i > 0 &&
							 text[i - 1] == pattern.patterns[2][0] )
							continue;
						Uint8 lead = ( 0xff & ( text[start] ) );
						if ( !( lead < 0x80 ) ) {
							char* strStart = const_cast<char*>( text.c_str() + start );
							char* strEnd = strStart;
							String::utf8Next( strEnd );
							end = start + ( strEnd - strStart );
						}
						if ( curMatch == 1 && start > lastStart ) {
							pushToken(
								tokens, patternType,
								text.substr( patternMatchStart, start - patternMatchStart ) );
						} else if ( start > lastEnd ) {
							pushToken( tokens, patternType,
									   text.substr( lastEnd, start - lastEnd ) );
						}

						patternText = text.substr( start, end - start );
						SyntaxStyleType type = curState.currentSyntax->getSymbol( patternText );
						if ( !skipSubSyntaxSeparator || !pattern.hasSyntax() ) {
							pushToken( tokens,
									   type == SyntaxStyleEmpty()
										   ? ( curMatch < pattern.types.size()
												   ? pattern.types[curMatch]
												   : pattern.types[0] )
										   : type,
									   patternText );
						}

						if ( pattern.hasSyntax() ) {
							pushSubsyntax( curState, retState, pattern, patternIndex + 1,
										   patternStr );
						} else if ( pattern.patterns.size() > 1 ) {
							setSubsyntaxPatternIdx( curState, retState, patternIndex + 1 );
						}

						i = end;

						if ( curMatch == numMatches - 1 && end < patternMatchEnd ) {
							pushToken( tokens, patternType,
									   text.substr( end, patternMatchEnd - end ) );
							i = patternMatchEnd;
						}

						matched = true;
						lastStart = start;
						lastEnd = end;
					}
					break;
				} else {
					for ( size_t curMatch = 0; curMatch < numMatches; curMatch++ ) {
						start = matches[curMatch].start;
						end = matches[curMatch].end;
						if ( pattern.patterns.size() >= 3 && i > 0 &&
							 text[i - 1] == pattern.patterns[2][0] )
							continue;
						Uint8 lead = ( 0xff & ( text[start] ) );
						if ( !( lead < 0x80 ) ) {
							char* strStart = const_cast<char*>( text.c_str() + start );
							char* strEnd = strStart;
							String::utf8Next( strEnd );
							end = start + ( strEnd - strStart );
						}
						patternText = text.substr( start, end - start );
						SyntaxStyleType type = curState.currentSyntax->getSymbol( patternText );
						if ( !skipSubSyntaxSeparator || !pattern.hasSyntax() ) {
							pushToken( tokens,
									   type == SyntaxStyleEmpty()
										   ? ( curMatch < pattern.types.size()
												   ? pattern.types[curMatch]
												   : pattern.types[0] )
										   : type,
									   patternText );
						}
						if ( pattern.hasSyntax() ) {
							pushSubsyntax( curState, retState, pattern, patternIndex + 1,
										   patternText );
						} else if ( pattern.patterns.size() > 1 ) {
							setSubsyntaxPatternIdx( curState, retState, patternIndex + 1 );
						}
						i = end;
						matched = true;
					}
					break;
				}
			}
		}

		if ( !matched && i < text.size() ) {
			char* strStart = const_cast<char*>( text.c_str() + i );
			char* strEnd = strStart;
			String::utf8Next( strEnd );
			int dist = strEnd - strStart;
			if ( dist > 0 ) {
				pushToken( tokens, SyntaxStyleTypes::Normal, text.substr( i, dist ) );
				i += dist;
			} else {
				Log::error( "Error parsing \"%s\" using syntax: %s", text.c_str(),
							syntax.getLSPName().c_str() );
				break;
			}
		}
	}

	return std::make_pair( std::move( tokens ), retState );
}

std::pair<std::vector<SyntaxToken>, SyntaxState>
SyntaxTokenizer::tokenize( const SyntaxDefinition& syntax, const std::string& text,
						   const SyntaxState& state, const size_t& startIndex,
						   bool skipSubSyntaxSeparator ) {
	return _tokenize<SyntaxToken>( syntax, text, state, startIndex, skipSubSyntaxSeparator );
}

std::pair<std::vector<SyntaxTokenPosition>, SyntaxState>
SyntaxTokenizer::tokenizePosition( const SyntaxDefinition& syntax, const std::string& text,
								   const SyntaxState& state, const size_t& startIndex,
								   bool skipSubSyntaxSeparator ) {
	return _tokenize<SyntaxTokenPosition>( syntax, text, state, startIndex,
										   skipSubSyntaxSeparator );
}

std::pair<std::vector<SyntaxTokenComplete>, SyntaxState>
SyntaxTokenizer::tokenizeComplete( const SyntaxDefinition& syntax, const std::string& text,
								   const SyntaxState& state, const size_t& startIndex,
								   bool skipSubSyntaxSeparator ) {
	return _tokenize<SyntaxTokenComplete>( syntax, text, state, startIndex,
										   skipSubSyntaxSeparator );
}

Text& SyntaxTokenizer::tokenizeText( const SyntaxDefinition& syntax,
									 const SyntaxColorScheme& colorScheme, Text& text,
									 const size_t& startIndex, const size_t& endIndex,
									 bool skipSubSyntaxSeparator, const std::string& trimChars ) {

	auto tokens = SyntaxTokenizer::tokenizeComplete( syntax, text.getString(), SyntaxState{},
													 startIndex, skipSubSyntaxSeparator )
					  .first;

	if ( skipSubSyntaxSeparator || !trimChars.empty() ) {
		String txt;
		size_t c = 0;
		for ( auto& token : tokens ) {
			if ( c == 0 ) {
				auto f = token.text.find_first_not_of( trimChars );
				if ( f == std::string::npos ) {
					token.text.clear();
					token.len = 0;
				} else if ( f > 0 ) {
					token.text = token.text.substr( f );
					token.len = String::utf8Length( token.text );
				}
			} else if ( c == tokens.size() - 1 ) {
				auto f = token.text.find_last_not_of( trimChars );
				if ( f == std::string::npos ) {
					token.text.clear();
					token.len = 0;
				} else if ( f >= 0 && f + 1 <= token.text.size() ) {
					token.text = token.text.substr( 0, f + 1 );
					token.len = String::utf8Length( token.text );
				}
			}
			if ( !token.text.empty() )
				txt += token.text;
			++c;
		}
		text.setString( txt );
	}

	size_t start = startIndex;
	for ( const auto& token : tokens ) {
		if ( start < endIndex ) {
			if ( token.len > 0 )
				text.setFillColor( colorScheme.getSyntaxStyle( token.type ).color, start,
								   std::min( start + token.len, endIndex ) );
			start += token.len;
		} else {
			break;
		}
	}

	return text;
}

}}} // namespace EE::UI::Doc
