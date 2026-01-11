#include <eepp/ui/doc/languages/covscript.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addCovScript() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "CovScript",
		  { "%.csc$", "%.csp$", "%.ecs$" },
		  {
			  { { "#.-\n" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "(import)%s+([%a_][%w/_]+)(%s+as%s+)([%a_][%w_]*)" },
				{ "normal", "keyword", "literal", "keyword", "type" } },
			  { { "(import)%s+([%a_][%w/_]+)" }, { "keyword", "keyword", "literal" } },
			  { { "(class)%s+([%a_][%w_]+)(%s+extends%s+)([%a_][%w_]+)" },
				{ "keyword", "keyword", "type", "keyword", "type" } },
			  { { "(class)%s+([%a_][%w_]+)" }, { "keyword", "keyword", "type" } },
			  { { "(struct)%s+([%a_][%w_]+)" }, { "keyword", "keyword", "type" } },
			  { { "(package)%s+([%a_][%w_]+)" }, { "keyword", "keyword", "literal" } },
			  { { "(using)%s+([%a_][%w_]+)" }, { "keyword", "keyword", "literal" } },
			  { { "common_number_parser" }, "number", "", SyntaxPatternMatchType::Parser },
			  { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
			  { { "(@[A-Za-z_]%w*)%s*(%:%s*%w+%s*)$" }, { "normal", "keyword", "type" } },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "@[%a_][%w_]*" }, "symbol" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {

			  { "new", "keyword" },		   { "continue", "keyword" },	{ "default", "keyword" },
			  { "finalize", "keyword" },   { "this", "keyword" },		{ "loop", "keyword" },
			  { "foreach", "keyword" },	   { "namespace", "keyword" },	{ "duplicate", "keyword" },
			  { "end", "keyword" },		   { "swap", "keyword" },		{ "extends", "keyword" },
			  { "range", "keyword" },	   { "false", "literal" },		{ "for", "keyword" },
			  { "while", "keyword" },	   { "null", "literal" },		{ "function", "keyword" },
			  { "else", "keyword" },	   { "global", "keyword" },		{ "struct", "keyword" },
			  { "block", "keyword" },	   { "break", "keyword" },		{ "is", "keyword" },
			  { "or", "keyword" },		   { "until", "keyword" },		{ "if", "keyword" },
			  { "to_string", "keyword" },  { "in", "keyword" },			{ "constant", "keyword" },
			  { "and", "keyword" },		   { "equal", "keyword" },		{ "switch", "keyword" },
			  { "using", "keyword" },	   { "to_integer", "keyword" }, { "typeid", "keyword" },
			  { "return", "keyword" },	   { "var", "keyword" },		{ "import", "keyword" },
			  { "try", "keyword" },		   { "as", "keyword" },			{ "class", "keyword" },
			  { "gcnew", "keyword" },	   { "link", "keyword" },		{ "throw", "keyword" },
			  { "self", "keyword" },	   { "catch", "keyword" },		{ "true", "literal" },
			  { "package", "keyword" },	   { "override", "keyword" },	{ "not", "keyword" },
			  { "initialize", "keyword" }, { "local", "keyword" },		{ "move", "keyword" },
			  { "case", "keyword" },	   { "clone", "keyword" },		{ "do", "keyword" },
			  { "type", "keyword" },	   { "@begin", "keyword" },		{ "@end", "keyword" },
			  { "array", "type" },		   { "string", "type" },		{ "hash_map", "type" },

		  },
		  "//",
		  { "^#!.*[ /]covscript", "^#!.*[ /]cs" }

		} );

	sd.setFoldRangeType( FoldRangeType::Indentation );
	return sd;
}

}}}} // namespace EE::UI::Doc::Language
