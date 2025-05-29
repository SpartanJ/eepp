#include <eepp/ui/doc/languages/blueprint.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addBlueprint() {

	return SyntaxDefinitionManager::instance()->add(

		{ "Blueprint",
		  { "%.blp$" },
		  {
			  { { "//.*" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "'", "'", "\\" }, "string" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "%.?%d+" }, "number" },
			  { { "%[.*%]" }, "literal" },
			  { { "%$" }, "operator" },
			  { { "(=>%s*%$)(.*)(%(%))" }, { "normal", "operator", "function", "normal" } },
			  { { "([%w-_]+)(%s*:)" }, { "normal", "keyword", "normal" } },
			  { { "([%w_-%.]+%s*)({)" }, { "normal", "keyword2", "normal" } },
			  { { "([%w_-%.]+%s*)([%w_-]+%s*{)" }, { "normal", "keyword2", "normal" } },
			  { { "[%w-_]+" }, "symbol" },

		  },
		  {
			  { "no-sync-create", "keyword" },
			  { "section", "keyword" },
			  { "bind", "keyword" },
			  { "setters", "keyword2" },
			  { "default", "keyword" },
			  { "suggested", "keyword" },
			  { "using", "keyword" },
			  { "strings", "keyword2" },
			  { "swapped", "keyword" },
			  { "accessibility", "keyword2" },
			  { "responses", "keyword2" },
			  { "mime-types", "keyword2" },
			  { "after", "keyword" },
			  { "menu", "keyword" },
			  { "mark", "keyword2" },
			  { "destructive", "keyword" },
			  { "submenu", "keyword" },
			  { "suffixes", "keyword2" },
			  { "patterns", "keyword2" },
			  { "item", "keyword2" },
			  { "false", "literal" },
			  { "condition", "keyword2" },
			  { "null", "literal" },
			  { "bidirectional", "keyword" },
			  { "inverted", "keyword" },
			  { "true", "literal" },
			  { "bind-property", "keyword" },
			  { "marks", "keyword2" },
			  { "C_", "operator" },
			  { "sync-create", "keyword" },
			  { "template", "keyword" },
			  { "widgets", "keyword2" },
			  { "_", "operator" },
			  { "styles", "keyword2" },
			  { "items", "keyword2" },
			  { "layout", "keyword2" },
			  { "disabled", "keyword" },

		  },
		  "//",
		  {}

		} );
}

}}}} // namespace EE::UI::Doc::Language
