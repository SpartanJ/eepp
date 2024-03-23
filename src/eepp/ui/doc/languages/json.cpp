#include <eepp/ui/doc/languages/json.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addJSON() {

	SyntaxDefinitionManager::instance()->add(

		{ "JSON",
		  { "%.json$", "%.cson$", "%.webmanifest" },
		  {
			  { { "(%b\"\")(:)" }, { "normal", "keyword", "operator" } },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "`", "`", "\\" }, "string" },
			  { { "0x[%da-fA-F]+" }, "number" },
			  { { "-?%d+[%d%.eE]*" }, "number" },
			  { { "-?%.?%d+" }, "number" },
			  { { "[%[%]%{%}]" }, "operator" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "false", "literal" },
			  { "true", "literal" },

		  },
		  "//",
		  {}

		} );
}

}}}} // namespace EE::UI::Doc::Language
