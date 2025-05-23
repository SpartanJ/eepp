#ifndef EE_UI_DOC_SYNTAXTOKENIZER_HPP
#define EE_UI_DOC_SYNTAXTOKENIZER_HPP

#include <eepp/config.hpp>
#include <eepp/core/string.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/ui/doc/syntaxcolorscheme.hpp>
#include <string>

using namespace EE::Graphics;

namespace EE { namespace UI { namespace Doc {

class SyntaxDefinition;
struct SyntaxPattern;

using SyntaxTokenLen = Uint32;

struct EE_API SyntaxToken {
	SyntaxStyleType type;
	SyntaxTokenLen len{ 0 };

	SyntaxToken( SyntaxStyleType type, SyntaxTokenLen len ) : type( type ), len( len ) {}
};

struct EE_API SyntaxTokenPosition {
	SyntaxStyleType type;
	SyntaxTokenLen pos{ 0 };
	SyntaxTokenLen len{ 0 };

	SyntaxTokenPosition( SyntaxStyleType type, SyntaxTokenLen pos, SyntaxTokenLen len ) :
		type( type ), pos( pos ), len( len ) {}
};

struct EE_API SyntaxTokenComplete {
	std::string text;
	SyntaxStyleType type;
	SyntaxTokenLen len{ 0 };

	SyntaxTokenComplete( SyntaxStyleType type, const std::string& text, SyntaxTokenLen len ) :
		text( text ), type( type ), len( len ) {}
};

#define SYNTAX_TOKENIZER_STATE_NONE ( 0 )

struct SyntaxStateType {
	Uint8 state{ SYNTAX_TOKENIZER_STATE_NONE };
	Uint8 repositoryIdx{ SYNTAX_TOKENIZER_STATE_NONE };
};

struct SyntaxStateRestored {
	const SyntaxDefinition* currentSyntax{ nullptr };
	const SyntaxPattern* subsyntaxInfo{ nullptr };
	SyntaxStateType currentPatternIdx{};
	Uint32 currentLevel{ 0 };
};

#define MAX_SUB_SYNTAXS 8

struct SyntaxState {
	// 16 bits per pattern - max 8 sub-languages - max 254 patterns per language or repository
	SyntaxStateType state[MAX_SUB_SYNTAXS]{};

	// 16 bits per language (language index) - max 8 sub-languages - max 65k languages
	Uint16 langStack[MAX_SUB_SYNTAXS]{};

	bool operator==( const SyntaxState& other ) const {
		return memcmp( this, &other, sizeof( SyntaxState ) ) == 0;
	}

	bool operator!=( const SyntaxState& other ) const { return !( *this == other ); }
};

class EE_API SyntaxTokenizer {
  public:
	static std::pair<std::vector<SyntaxToken>, SyntaxState>
	tokenize( const SyntaxDefinition& syntax, const std::string& text, const SyntaxState& state,
			  const size_t& startIndex = 0, bool skipSubSyntaxSeparator = false );

	static std::pair<std::vector<SyntaxTokenPosition>, SyntaxState>
	tokenizePosition( const SyntaxDefinition& syntax, const std::string& text,
					  const SyntaxState& state, const size_t& startIndex = 0,
					  bool skipSubSyntaxSeparator = false );

	static std::pair<std::vector<SyntaxTokenComplete>, SyntaxState>
	tokenizeComplete( const SyntaxDefinition& syntax, const std::string& text,
					  const SyntaxState& state, const size_t& startIndex = 0,
					  bool skipSubSyntaxSeparator = false );

	static Text& tokenizeText( const SyntaxDefinition& syntax, const SyntaxColorScheme& colorScheme,
							   Text& text, const size_t& startIndex = 0,
							   const size_t& endIndex = 0xFFFFFFFF,
							   bool skipSubSyntaxSeparator = false,
							   const std::string& trimChars = "" );

	static SyntaxStateRestored retrieveSyntaxState( const SyntaxDefinition& syntax,
													const SyntaxState& state );
};

}}} // namespace EE::UI::Doc

#endif // EE_UI_DOC_SYNTAXTOKENIZER_HPP
