#include <eepp/ui/doc/languages/makefile.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addMakefile() {

	SyntaxDefinitionManager::instance()->add(

		{ "Makefile",
		  { "^[Mm]akefile$", "%.mk$", "%.make$", "%.mak$", "^Makefile%.am$", "^Makefile%.in$" },
		  {
			  { { "#.*\n" }, "comment" },
			  { { "[[.]]}" }, "normal" },
			  { { "$[@^<%%?+|*]" }, "keyword2" },
			  { { "$%(", "%)" }, "keyword" },
			  { { "%f[%w_][%d%.]+%f[^%w_]" }, "number" },
			  { { "%..*:" }, "keyword2" },
			  { { ".*:=" }, "function" },
			  { { ".*+=" }, "function" },
			  { { ".*%s=" }, "function" },

		  },
		  {

		  },
		  "#",
		  {}

		} );
}

}}}} // namespace EE::UI::Doc::Language
