#include <eepp/system/luapattern.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>
#include <eepp/ui/doc/syntaxtokenizer.hpp>

using namespace EE::System;

namespace EE { namespace UI { namespace Doc {

// This tokenizer is a direct conversion to C++ from the lite (https://github.com/rxi/lite)
// tokenizer. This allows eepp to support the same color schemes and syntax definitions from
// lite. Making much easier to implement a complete code editor.

#define MAX_TOKEN_SIZE ( 512 )

static void pushToken( std::vector<SyntaxToken>& tokens, const std::string& type,
					   const std::string& text ) {
	if ( !tokens.empty() && ( tokens[tokens.size() - 1].type == type ) ) {
		tokens[tokens.size() - 1].type = type;
		tokens[tokens.size() - 1].text += text;
	} else {
		if ( text.size() > MAX_TOKEN_SIZE ) {
			size_t textSize = text.size();
			size_t steps = textSize / MAX_TOKEN_SIZE + 1;

			for ( size_t i = 0; i < steps; ++i ) {
				size_t strSize =
					( i == steps - 1 ) ? textSize - MAX_TOKEN_SIZE * i : MAX_TOKEN_SIZE;
				tokens.push_back( { type, text.substr( i * MAX_TOKEN_SIZE, strSize ) } );
			}
		} else {
			tokens.push_back( { type, text } );
		}
	}
}

bool isScaped( const std::string& text, const size_t& startIndex, const std::string& escapeStr ) {
	char escapeByte = escapeStr.empty() ? '\\' : escapeStr[0];
	int count = 0;
	for ( int i = startIndex - 1; i >= 0; i-- ) {
		if ( text[i] != escapeByte )
			break;
		count++;
	}
	return count % 2 == 1;
}

std::pair<int, int> findNonEscaped( const std::string& text, const std::string& pattern, int offset,
									const std::string& escapeStr ) {
	while ( true ) {
		LuaPattern words( pattern );
		int start, end;
		if ( words.find( text, start, end, offset ) ) {
			if ( !escapeStr.empty() && isScaped( text, start, escapeStr ) ) {
				offset = end;
			} else {
				return std::make_pair( start, end );
			}
		} else {
			return std::make_pair( -1, -1 );
		}
	}
}

SyntaxState SyntaxTokenizer::retrieveSyntaxState( const SyntaxDefinition& syntax,
												  const Uint32& state ) {
	SyntaxState syntaxState{ &syntax, nullptr, state, 0 };
	if ( state > 0 &&
		 ( state > 255 ||
		   ( state < syntaxState.currentSyntax->getPatterns().size() &&
			 !syntaxState.currentSyntax->getPatterns()[state - 1].syntax.empty() ) ) ) {
		for ( size_t i = 0; i <= 2; ++i ) {
			Uint32 target = ( state >> ( 8 * i ) ) & 0xFF;
			if ( target != SYNTAX_TOKENIZER_STATE_NONE ) {
				if ( target < syntaxState.currentSyntax->getPatterns().size() &&
					 !syntaxState.currentSyntax->getPatterns()[target - 1].syntax.empty() ) {
					syntaxState.subsyntaxInfo =
						&syntaxState.currentSyntax->getPatterns()[target - 1];
					syntaxState.currentSyntax =
						&SyntaxDefinitionManager::instance()->getStyleByLanguageName(
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

std::pair<std::vector<SyntaxToken>, Uint32>
SyntaxTokenizer::tokenize( const SyntaxDefinition& syntax, const std::string& text,
						   const Uint32& state, const size_t& startIndex ) {
	std::vector<SyntaxToken> tokens;
	if ( syntax.getPatterns().empty() ) {
		pushToken( tokens, "normal", text );
		return std::make_pair( tokens, SYNTAX_TOKENIZER_STATE_NONE );
	}

	size_t i = startIndex;
	int retState = state;
	SyntaxState curState = retrieveSyntaxState( syntax, state );

	auto setSubsyntaxPatternIdx = [&curState, &retState]( const Uint32& patternIndex ) {
		curState.currentPatternIdx = patternIndex;
		retState &= ~( 0xFF << ( curState.currentLevel * 8 ) );
		retState |= ( patternIndex << ( curState.currentLevel * 8 ) );
	};

	auto pushSubsyntax = [&setSubsyntaxPatternIdx, &curState](
							 const SyntaxPattern& enteringSubsyntax, const Uint32& patternIndex ) {
		setSubsyntaxPatternIdx( patternIndex );
		curState.currentLevel++;
		curState.subsyntaxInfo = &enteringSubsyntax;
		curState.currentSyntax = &SyntaxDefinitionManager::instance()->getStyleByLanguageName(
			curState.subsyntaxInfo->syntax );
		setSubsyntaxPatternIdx( SYNTAX_TOKENIZER_STATE_NONE );
	};

	auto popSubsyntax = [&setSubsyntaxPatternIdx, &curState, &syntax, &retState]() {
		setSubsyntaxPatternIdx( SYNTAX_TOKENIZER_STATE_NONE );
		curState.currentLevel--;
		setSubsyntaxPatternIdx( SYNTAX_TOKENIZER_STATE_NONE );
		curState = retrieveSyntaxState( syntax, retState );
	};

	while ( i < text.size() ) {
		if ( curState.currentPatternIdx != SYNTAX_TOKENIZER_STATE_NONE ) {
			const SyntaxPattern& pattern =
				curState.currentSyntax->getPatterns()[curState.currentPatternIdx - 1];
			std::pair<int, int> range =
				findNonEscaped( text, pattern.patterns[1], i,
								pattern.patterns.size() >= 3 ? pattern.patterns[2] : "" );

			bool skip = false;

			if ( curState.subsyntaxInfo != nullptr ) {
				std::pair<int, int> rangeSubsyntax =
					findNonEscaped( text, curState.subsyntaxInfo->patterns[1], i,
									curState.subsyntaxInfo->patterns.size() >= 3
										? curState.subsyntaxInfo->patterns[2]
										: "" );

				if ( rangeSubsyntax.first != -1 &&
					 ( range.first == -1 || rangeSubsyntax.first < range.first ) ) {
					pushToken( tokens, curState.subsyntaxInfo->type,
							   text.substr( i, rangeSubsyntax.second - i ) );
					popSubsyntax();
					i = rangeSubsyntax.second;
					skip = true;
				}
			}

			if ( !skip ) {
				if ( range.first != -1 ) {
					pushToken( tokens, pattern.type, text.substr( i, range.second - i ) );
					setSubsyntaxPatternIdx( SYNTAX_TOKENIZER_STATE_NONE );
					i = range.second;
				} else {
					pushToken( tokens, pattern.type, text.substr( i ) );
					break;
				}
			}
		}

		if ( curState.subsyntaxInfo != nullptr ) {
			std::pair<int, int> rangeSubsyntax = findNonEscaped(
				text, "^" + curState.subsyntaxInfo->patterns[1], i,
				curState.subsyntaxInfo->patterns.size() >= 3 ? curState.subsyntaxInfo->patterns[2]
															 : "" );

			if ( rangeSubsyntax.first != -1 ) {
				pushToken( tokens, curState.subsyntaxInfo->type,
						   text.substr( i, rangeSubsyntax.second - i ) );
				popSubsyntax();
				i = rangeSubsyntax.second;
			}
		}

		bool matched = false;

		for ( size_t patternIndex = 0; patternIndex < curState.currentSyntax->getPatterns().size();
			  patternIndex++ ) {
			const SyntaxPattern& pattern = curState.currentSyntax->getPatterns()[patternIndex];
			if ( i != 0 && pattern.patterns[0][0] == '^' )
				continue;
			const std::string& patternStr(
				pattern.patterns[0][0] == '^' ? pattern.patterns[0] : "^" + pattern.patterns[0] );
			LuaPattern words( patternStr );
			int start, end = 0;
			if ( words.find( text, start, end, i ) && start != end ) {
				if ( pattern.patterns.size() >= 3 && i > 0 &&
					 text[i - 1] == pattern.patterns[2][0] )
					continue;
				std::string patternText( text.substr( start, end - start ) );
				std::string type = curState.currentSyntax->getSymbol( patternText );
				pushToken( tokens, type.empty() ? pattern.type : type, patternText );
				if ( !pattern.syntax.empty() ) {
					pushSubsyntax( pattern, patternIndex + 1 );
				} else if ( pattern.patterns.size() > 1 ) {
					setSubsyntaxPatternIdx( patternIndex + 1 );
				}
				i = end;
				matched = true;
				break;
			}
		}

		if ( !matched && i < text.size() ) {
			pushToken( tokens, "normal", text.substr( i, 1 ) );
			i += 1;
		}
	}

	return std::make_pair( tokens, retState );
}

}}} // namespace EE::UI::Doc
