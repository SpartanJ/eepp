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

#define SYNTAX_TOKENIZER_STATE_NONE ( -1 )

class EE_API SyntaxTokenizer {
  public:
	std::pair<std::vector<SyntaxToken>, int> static tokenize( const SyntaxDefinition& syntax,
															  const std::string& text,
															  const int& state,
															  const size_t& startIndex = 0 );
};

}}} // namespace EE::UI::Doc

#endif // EE_UI_DOC_SYNTAXTOKENIZER_HPP
