#include <eepp/ui/doc/languages/brainfuck.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addBrainfuck() {

	SyntaxDefinitionManager::instance()->add(

		{ "Brainfuck",
		  { "%.bf$" },
		  {
			  { { "%[" }, "operator" },
			  { { "%]" }, "operator" },
			  { { "%-" }, "keyword" },
			  { { "<" }, "keyword2" },
			  { { ">" }, "keyword2" },
			  { { "+" }, "string" },
			  { { "," }, "literal" },
			  { { "%." }, "string" },
			  { { "[^%-%.<>%+,%[%]]+" }, "comment" },

		  },
		  {

		  },
		  "",
		  {}

		} );
}

}}}} // namespace EE::UI::Doc::Language
