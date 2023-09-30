#ifndef EE_UI_DOC_SYNTAXTOKENIZER_HPP
#define EE_UI_DOC_SYNTAXTOKENIZER_HPP

#include <eepp/config.hpp>
#include <eepp/core/string.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/ui/doc/syntaxcolorscheme.hpp>
#include <eepp/ui/doc/syntaxdefinition.hpp>
#include <memory>
#include <string>

using namespace EE::Graphics;

namespace EE { namespace UI { namespace Doc {

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

struct SyntaxState {
	const SyntaxDefinition* currentSyntax{ nullptr };
	const SyntaxPattern* subsyntaxInfo{ nullptr };
	Uint32 currentPatternIdx{ 0 };
	Uint32 currentLevel{ 0 };
};

class EE_API SyntaxTokenizer {
  public:
	static std::pair<std::vector<SyntaxToken>, Uint32>
	tokenize( const SyntaxDefinition& syntax, const std::string& text, const Uint32& state,
			  const size_t& startIndex = 0, bool skipSubSyntaxSeparator = false );

	static std::pair<std::vector<SyntaxTokenPosition>, Uint32>
	tokenizePosition( const SyntaxDefinition& syntax, const std::string& text, const Uint32& state,
					  const size_t& startIndex = 0, bool skipSubSyntaxSeparator = false );

	static std::pair<std::vector<SyntaxTokenComplete>, Uint32>
	tokenizeComplete( const SyntaxDefinition& syntax, const std::string& text, const Uint32& state,
					  const size_t& startIndex = 0, bool skipSubSyntaxSeparator = false );

	static Text& tokenizeText( const SyntaxDefinition& syntax, const SyntaxColorScheme& colorScheme,
							   Text& text, const size_t& startIndex = 0,
							   const size_t& endIndex = 0xFFFFFFFF,
							   bool skipSubSyntaxSeparator = false,
							   const std::string& trimChars = "" );

	static SyntaxState retrieveSyntaxState( const SyntaxDefinition& syntax, const Uint32& state );
};

}}} // namespace EE::UI::Doc

#endif // EE_UI_DOC_SYNTAXTOKENIZER_HPP
