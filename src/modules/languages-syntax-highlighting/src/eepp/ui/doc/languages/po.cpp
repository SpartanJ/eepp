#include <eepp/ui/doc/languages/po.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addPO() {

	return SyntaxDefinitionManager::instance()->add(

		{ "PO",
		  { "%.po$", "%.pot$" },
		  {
			  { { "#", "\n" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "[%[%]]" }, "operator" },
			  { { "%d+" }, "number" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "msgctxt", "keyword" },
			  { "msgid", "keyword" },
			  { "msgstr", "keyword" },
			  { "msgid_plural", "keyword" },

		  },
		  "#",
		  {}

		} );
}

}}}} // namespace EE::UI::Doc::Language
