#include <eepp/ui/doc/languages/covscript.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addCovScript() {

	SyntaxDefinitionManager::instance()->add(

		{ "CovScript",
		  { "%.csc$", "%.csp$", "%.ecs$" },
		  {
			  { { "#.-\n" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "-?%.?%d+" }, "number" },
			  { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
			  { { "(import)%s+(%a+[%w/_]+)" }, { "normal", "keyword", "literal" } },
			  { { "(@[A-Za-z_]%w*)%s*(%:%s*%w+%s*)$" }, { "normal", "keyword", "keyword2" } },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
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
			  { "type", "keyword" },

		  },
		  "//",
		  { "^#!.*[ /]covscript", "^#!.*[ /]cs" }

		} );
}

}}}} // namespace EE::UI::Doc::Language
