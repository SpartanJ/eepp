#include <eepp/ui/doc/languages/javascript.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addJavaScript() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "JavaScript",
		  { "%.js$" },
		  {
			  { { "//.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "`", "`", "\\" }, "string" },
			  { { "/[%+%-%*%^%!%=%&%|%?%:%;%,%(%[%{%<%>%\\%\"].*%f[/]",
				  "/[igmsuyd\n]?[igmsuyd\n]?[igmsuyd\n]?", "\\" },
				"string" },
			  { { "js_number_parser" }, "number", "", SyntaxPatternMatchType::Parser },
			  { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
			  { { "([%w_][%w_]+)%.([%w_][%w%d_]*)%s*(=)%s*(function)" },
				{ "normal", "keyword2", "function", "operator", "keyword" } },
			  { { "([%w_][%w_]+)%.([%w_][%w%d_]*)%s*(=)%s*(async%s*function)" },
				{ "normal", "keyword2", "function", "operator", "keyword" } },
			  { { "([%w_][%w_]+)%.([%w_][%w%d_]*)%s*(=)%s*%f[(]" },
				{ "normal", "keyword2", "function", "operator" } },
			  { { "([%w_][%w_]+)%.([%w_][%w%d_]*)%s*(=)%s*(async)%s*%f[(]" },
				{ "normal", "keyword2", "function", "operator", "function" } },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "new", "keyword" },		   { "continue", "keyword" },  { "default", "keyword" },
			  { "set", "keyword" },		   { "this", "keyword2" },	   { "Array", "keyword2" },
			  { "namespace", "keyword" },  { "any", "keyword" },	   { "of", "keyword" },
			  { "typeof", "keyword" },	   { "extends", "keyword" },   { "with", "keyword" },
			  { "undefined", "literal" },  { "false", "literal" },	   { "for", "keyword" },
			  { "while", "keyword" },	   { "const", "keyword" },	   { "get", "keyword" },
			  { "from", "keyword" },	   { "null", "literal" },	   { "function", "keyword" },
			  { "else", "keyword" },	   { "enum", "keyword" },	   { "async", "keyword" },
			  { "break", "keyword" },	   { "declare", "keyword" },   { "arguments", "keyword2" },
			  { "if", "keyword" },		   { "await", "keyword" },	   { "let", "keyword" },
			  { "in", "keyword" },		   { "public", "keyword" },	   { "debugger", "keyword" },
			  { "private", "keyword" },	   { "NaN", "keyword2" },	   { "finally", "keyword" },
			  { "switch", "keyword" },	   { "delete", "keyword" },	   { "return", "keyword" },
			  { "var", "keyword" },		   { "import", "keyword" },	   { "try", "keyword" },
			  { "class", "keyword" },	   { "void", "keyword" },	   { "yield", "keyword" },
			  { "static", "keyword" },	   { "super", "keyword" },	   { "throw", "keyword" },
			  { "catch", "keyword" },	   { "true", "literal" },	   { "implements", "keyword" },
			  { "instanceof", "keyword" }, { "Infinity", "keyword2" }, { "case", "keyword" },
			  { "export", "keyword" },	   { "protected", "keyword" }, { "do", "keyword" },

		  },
		  "//",
		  {}

		} );

	sd.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );
}

}}}} // namespace EE::UI::Doc::Language
