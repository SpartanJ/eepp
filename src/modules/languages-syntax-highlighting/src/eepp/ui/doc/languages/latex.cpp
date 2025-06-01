#include <eepp/ui/doc/languages/latex.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addLatex() {

	return SyntaxDefinitionManager::instance()->add(

		{ "LaTeX",
		  { "%.tex$", "%.sty$", "%.ltx$" },
		  {
			  { { "%%", "\n" }, "comment" },
			  { { "&" }, "operator" },
			  { { "\\\\" }, "operator" },
			  { { "%$", "%$" }, "operator" },
			  { { "\\%[", "\\]" }, "operator" },
			  { { "{", "}" }, "keyword" },
			  { { "\\%w*" }, "type" },

		  },
		  {

		  },
		  "%%",
		  {}

		} );
}

}}}} // namespace EE::UI::Doc::Language
