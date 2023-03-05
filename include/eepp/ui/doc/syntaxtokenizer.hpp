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

struct EE_API SyntaxToken {
	std::string type;
	std::unique_ptr<std::string> text; // text is optional and must be requested explicitly
	size_t len{ 0 };

	SyntaxToken( const std::string& _type, const std::string& _text, const size_t _len ) :
		type( _type ),
		text( _text.empty() ? nullptr : std::make_unique<std::string>( _text ) ),
		len( _len ) {}
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
			  const size_t& startIndex = 0, bool skipSubSyntaxSeparator = false,
			  bool allocateText = false );

	static Text& tokenizeText( const SyntaxDefinition& syntax, const SyntaxColorScheme& colorScheme,
							   Text& text, const size_t& startIndex = 0,
							   const size_t& endIndex = 0xFFFFFFFF,
							   bool skipSubSyntaxSeparator = false,
							   const std::string& trimChars = "" );

	static SyntaxState retrieveSyntaxState( const SyntaxDefinition& syntax, const Uint32& state );
};

}}} // namespace EE::UI::Doc

#endif // EE_UI_DOC_SYNTAXTOKENIZER_HPP
