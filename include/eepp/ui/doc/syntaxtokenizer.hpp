#ifndef EE_UI_DOC_SYNTAXTOKENIZER_HPP
#define EE_UI_DOC_SYNTAXTOKENIZER_HPP

#include <eepp/config.hpp>
#include <eepp/core/string.hpp>
#include <eepp/ui/doc/syntaxdefinition.hpp>
#include <string>

namespace EE { namespace UI { namespace Doc {

struct EE_API SyntaxToken {
	std::string type;
	std::string text;
};

#define SYNTAX_TOKENIZER_STATE_NONE ( 0 )

class EE_API SyntaxTokenizer {
  public:
	static std::pair<std::vector<SyntaxToken>, Uint32> tokenize( const SyntaxDefinition& syntax,
																 const std::string& text,
																 const Uint32& state,
																 const size_t& startIndex = 0 );
};

}}} // namespace EE::UI::Doc

#endif // EE_UI_DOC_SYNTAXTOKENIZER_HPP
