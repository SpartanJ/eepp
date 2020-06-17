#include <eepp/system/luapatternmatcher.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>
#include <eepp/ui/doc/syntaxtokenizer.hpp>

using namespace EE::System;

namespace EE { namespace UI { namespace Doc {

// This tokenizer is a direct conversion to C++ from the lite (https://github.com/rxi/lite)
// tokenizer. This allows eepp to support the same color schemes and syntax definitions from
// lite. Making much easier to implement a complete code editor.

static bool allSpaces( const std::string& str ) {
	for ( auto& chr : str )
		if ( ' ' != chr )
			return false;
	return true;
}

static void pushToken( std::vector<SyntaxToken>& tokens, const std::string& type,
					   const std::string& text ) {
	if ( !tokens.empty() && ( tokens[tokens.size() - 1].type == type ||
							  allSpaces( tokens[tokens.size() - 1].text ) ) ) {
		tokens[tokens.size() - 1].type = type;
		tokens[tokens.size() - 1].text += text;
	} else {
		tokens.push_back( {type, text} );
	}
}

bool isScaped( const std::string& text, const size_t& startIndex, const std::string& escapeStr ) {
	char escapeByte = escapeStr.empty() ? '\\' : escapeStr[0];
	int count = 0;
	for ( int i = startIndex < 1 ? 0 : startIndex - 1; i >= 0; i-- ) {
		if ( text[i] != escapeByte )
			break;
		count++;
	}
	return count % 2 == 1;
}

std::pair<int, int> findNonEscaped( const std::string& text, const std::string& pattern, int offset,
									const std::string& escapeStr ) {
	while ( true ) {
		LuaPatternMatcher words( pattern );
		int start, end;
		if ( words.find( text, offset, start, end ) ) {
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

std::pair<std::vector<SyntaxToken>, int> SyntaxTokenizer::tokenize( const SyntaxDefinition& syntax,
																	const std::string& text,
																	const int& state ) {
	std::vector<SyntaxToken> tokens;
	if ( syntax.getPatterns().empty() ) {
		pushToken( tokens, "normal", text );
		return std::make_pair( tokens, SYNTAX_TOKENIZER_STATE_NONE );
	}

	size_t i = 0;
	int retState = state;

	while ( i < text.size() ) {
		if ( retState != SYNTAX_TOKENIZER_STATE_NONE ) {
			const SyntaxPattern& pattern = syntax.getPatterns()[retState];
			std::pair<int, int> range =
				findNonEscaped( text, pattern.patterns[1], i,
								pattern.patterns.size() >= 3 ? pattern.patterns[2] : "" );
			if ( range.first != -1 ) {
				pushToken( tokens, pattern.type, text.substr( i, range.second - i ) );
				retState = SYNTAX_TOKENIZER_STATE_NONE;
				i = range.second;
			} else {
				pushToken( tokens, pattern.type, text.substr( i ) );
				break;
			}
		}

		bool matched = false;

		for ( size_t patternIndex = 0; patternIndex < syntax.getPatterns().size();
			  patternIndex++ ) {
			const SyntaxPattern& pattern = syntax.getPatterns()[patternIndex];
			if ( i != 0 && pattern.patterns[0][0] == '^' )
				continue;
			const std::string& patternStr(
				pattern.patterns[0][0] == '^' ? pattern.patterns[0] : "^" + pattern.patterns[0] );
			LuaPatternMatcher words( patternStr );
			int start, end = 0;
			if ( words.find( text, i, start, end ) ) {
				std::string patternText( text.substr( start, end - start ) );
				std::string type = syntax.getSymbol( patternText );
				pushToken( tokens, type.empty() ? pattern.type : type, patternText );
				if ( pattern.patterns.size() > 1 ) {
					retState = patternIndex;
				}
				i = end;
				matched = true;
				break;
			}
		}

		if ( !matched ) {
			pushToken( tokens, "normal", text.substr( i, 1 ) );
			i += 1;
		}
	}

	return std::make_pair( tokens, retState );
}

}}} // namespace EE::UI::Doc
