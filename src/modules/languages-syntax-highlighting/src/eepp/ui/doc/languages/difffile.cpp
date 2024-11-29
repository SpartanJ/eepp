#include <eepp/ui/doc/languages/difffile.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addDiff() {

	SyntaxDefinitionManager::instance()->add(

		{ "Diff File",
		  { "%.diff$", "%.patch$" },
		  {
			  { { "^%+%+%+%s.-\n" }, "keyword" },
			  { { "^%-%-%-%s.-\n" }, "keyword" },
			  { { "^diff%s.-\n" }, "string" },
			  { { "^index%s.-\n" }, "comment" },
			  { { "^@@.-\n" }, "number" },
			  { { "^%+.-\n" }, "function" },
			  { { "^%-.-\n" }, "keyword2" },

		  },
		  {

		  },
		  "",
		  {},
		  "diff" } );
}

}}}} // namespace EE::UI::Doc::Language
