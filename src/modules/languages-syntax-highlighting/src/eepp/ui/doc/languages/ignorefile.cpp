#include <eepp/ui/doc/languages/ignorefile.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addIgnoreFile() {

	return SyntaxDefinitionManager::instance()->add(

		{ ".ignore file",
		  { "%..*ignore$" },
		  {
			  { { "#.*$" }, "comment" },
			  { { "^%!.*$" }, "keyword" },
		  },
		  {},
		  "#" } );
}

}}}} // namespace EE::UI::Doc::Language
