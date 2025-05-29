#include <eepp/ui/doc/languages/graphql.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addGraphQL() {

	return SyntaxDefinitionManager::instance()->add(

		{ "GraphQL",
		  { "%.graphql$", "%.gql$" },
		  {
			  { { "\"\"\"", "\"\"\"" }, "comment" },
			  { { "#.*" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "-?%.?%d+" }, "number" },
			  { { "%s*[@]%s*[%a_][%w_]*" }, "function" },
			  { { "!" }, "operator" },
			  { { "%s*=%s*" }, "operator" },
			  { { "%s*%$[%a_][%w_]*:*" }, "literal" },
			  { { "(query%s*)([%a_][%w_]*[(])" }, { "normal", "keyword", "function" } },
			  { { "(mutation%s*)([%a_][%w_]*[(])" }, { "normal", "keyword", "function" } },
			  { { "(:%s*%[*)([%a_,%s][%w_,%s]*)(%]*)([!]*)" },
				{ "normal", "symbol", "literal", "symbol", "operator" } },

		  },
		  {
			  { "false", "literal" },
			  { "on", "keyword" },
			  { "true", "literal" },
			  { "scalar", "keyword" },
			  { "implements", "keyword" },
			  { "extend", "keyword2" },
			  { "enum", "keyword" },
			  { "schema", "keyword" },
			  { "mutation", "keyword" },
			  { "query", "keyword" },
			  { "directive", "keyword" },
			  { "union", "keyword" },
			  { "input", "keyword" },
			  { "interface", "keyword" },
			  { "extends", "keyword" },
			  { "fragment", "keyword" },
			  { "type", "keyword" },

		  },
		  "#",
		  {}

		} );
}

}}}} // namespace EE::UI::Doc::Language
