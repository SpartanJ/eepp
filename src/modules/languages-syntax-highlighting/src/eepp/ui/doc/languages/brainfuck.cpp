#include <eepp/ui/doc/languages/brainfuck.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addBrainfuck() {

	return SyntaxDefinitionManager::instance()->add(

		{ "Brainfuck",
		  { "%.bf$" },
		  {
			  { { "%[" }, "operator" },
			  { { "%]" }, "operator" },
			  { { "%-" }, "keyword" },
			  { { "<" }, "type" },
			  { { ">" }, "type" },
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
