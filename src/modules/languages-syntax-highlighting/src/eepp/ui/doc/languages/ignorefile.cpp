#include <eepp/ui/doc/languages/ignorefile.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addIgnoreFile() {

	SyntaxDefinitionManager::instance()->add(

		{ ".ignore file",
		  { "%..*ignore$" },
		  {
			  { { "^%s*#.*$" }, "comment" },
			  { { "^%!.*$" }, "keyword" },
		  },
		  {},
		  "#" } );
}

}}}} // namespace EE::UI::Doc::Language
