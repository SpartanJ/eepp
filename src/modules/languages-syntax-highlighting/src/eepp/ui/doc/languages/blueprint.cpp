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
			  { { "([%w_-%.]+%s*)({)" }, { "normal", "type", "normal" } },
			  { { "([%w_-%.]+%s*)([%w_-]+%s*{)" }, { "normal", "type", "normal" } },
			  { { "[%w-_]+" }, "symbol" },

		  },
		  {
			  { "no-sync-create", "keyword" },
			  { "section", "keyword" },
			  { "bind", "keyword" },
			  { "setters", "type" },
			  { "default", "keyword" },
			  { "suggested", "keyword" },
			  { "using", "keyword" },
			  { "strings", "type" },
			  { "swapped", "keyword" },
			  { "accessibility", "type" },
			  { "responses", "type" },
			  { "mime-types", "type" },
			  { "after", "keyword" },
			  { "menu", "keyword" },
			  { "mark", "type" },
			  { "destructive", "keyword" },
			  { "submenu", "keyword" },
			  { "suffixes", "type" },
			  { "patterns", "type" },
			  { "item", "type" },
			  { "false", "literal" },
			  { "condition", "type" },
			  { "null", "literal" },
			  { "bidirectional", "keyword" },
			  { "inverted", "keyword" },
			  { "true", "literal" },
			  { "bind-property", "keyword" },
			  { "marks", "type" },
			  { "C_", "operator" },
			  { "sync-create", "keyword" },
			  { "template", "keyword" },
			  { "widgets", "type" },
			  { "_", "operator" },
			  { "styles", "type" },
			  { "items", "type" },
			  { "layout", "type" },
			  { "disabled", "keyword" },

		  },
		  "//",
		  {}

		} );
}

}}}} // namespace EE::UI::Doc::Language
