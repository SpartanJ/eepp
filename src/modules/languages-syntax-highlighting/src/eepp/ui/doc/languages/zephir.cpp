#include <eepp/ui/doc/languages/zephir.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addZephir() {

	SyntaxDefinitionManager::instance()
		->add(

			{ "Zephir",
			  { "%.zep$" },
			  {
				  { { "//.-\n" }, "comment" },
				  { { "/%*", "%*/" }, "comment" },
				  { { "\"", "\"", "\\" }, "string" },
				  { { "'", "'", "\\" }, "string" },
				  { { "/[%+%-%*%^%!%=%&%|%?%:%;%,%(%[%{%<%>%\\%\"].*%f[/]",
					  "/[igmsuyd\n]?[igmsuyd\n]?[igmsuyd\n]?", "\\" },
					"string" },
				  { { "common_number_parser" }, "number", "", SyntaxPatternMatchType::Parser },
				  { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
				  { { "[%a_][%w_]*%f[(]" }, "function" },
				  { { "%$[%a][%w_]*" }, "keyword3" },
				  { { "[%a_][%w_]*" }, "symbol" },

			  },
			  {
				  { "internal", "keyword" },   { "int", "keyword2" },
				  { "new", "keyword" },		   { "continue", "keyword" },
				  { "default", "keyword" },	   { "isset", "keyword" },
				  { "char", "keyword2" },	   { "likely", "keyword" },
				  { "_REQUEST", "keyword2" },  { "bool", "keyword2" },
				  { "this", "keyword" },	   { "loop", "keyword" },
				  { "iterator", "keyword2" },  { "long", "keyword2" },
				  { "Array", "keyword2" },	   { "namespace", "keyword" },
				  { "short", "keyword2" },	   { "typeof", "keyword" },
				  { "unset", "keyword" },	   { "PHP_VERSION", "keyword2" },
				  { "extends", "keyword" },	   { "match", "keyword" },
				  { "range", "keyword2" },	   { "with", "keyword" },
				  { "undefined", "literal" },  { "false", "literal" },
				  { "for", "keyword" },		   { "goto", "keyword" },
				  { "while", "keyword" },	   { "const", "keyword" },
				  { "null", "literal" },	   { "function", "keyword" },
				  { "else", "keyword" },	   { "final", "keyword" },
				  { "enum", "keyword" },	   { "_COOKIE", "keyword2" },
				  { "break", "keyword" },	   { "use", "keyword" },
				  { "string", "keyword" },	   { "if", "keyword" },
				  { "let", "keyword" },		   { "in", "keyword" },
				  { "native", "keyword" },	   { "public", "keyword" },
				  { "unsigned", "keyword2" },  { "throws", "keyword" },
				  { "private", "keyword" },	   { "finally", "keyword" },
				  { "abstract", "keyword" },   { "volatile", "keyword" },
				  { "switch", "keyword" },	   { "double", "keyword2" },
				  { "_SERVER", "keyword2" },   { "delete", "keyword" },
				  { "unlikely", "keyword" },   { "ulong", "keyword2" },
				  { "return", "keyword" },	   { "var", "keyword" },
				  { "import", "keyword" },	   { "uchar", "keyword2" },
				  { "try", "keyword" },		   { "class", "keyword" },
				  { "void", "keyword" },	   { "static", "keyword" },
				  { "inline", "keyword" },	   { "echo", "keyword" },
				  { "reverse", "keyword" },	   { "boolean", "keyword2" },
				  { "_POST", "keyword2" },	   { "throw", "keyword" },
				  { "self", "keyword2" },	   { "catch", "keyword" },
				  { "true", "literal" },	   { "implements", "keyword" },
				  { "stdClass", "keyword2" },  { "Boolean", "keyword2" },
				  { "instanceof", "keyword" }, { "_SESSION", "keyword2" },
				  { "require", "keyword" },	   { "transient", "keyword" },
				  { "Date", "keyword2" },	   { "float", "keyword2" },
				  { "PHP_EOL", "keyword2" },   { "fetch", "keyword" },
				  { "case", "keyword" },	   { "_GET", "keyword2" },
				  { "export", "keyword" },	   { "interface", "keyword" },
				  { "protected", "keyword" },  { "do", "keyword" },
				  { "window", "keyword2" },	   { "empty", "keyword" },

			  },
			  "//",
			  {}

			} )
		.setFoldRangeType( FoldRangeType::Braces )
		.setFoldBraces( { { '}', '{' } } );
	;
}

}}}} // namespace EE::UI::Doc::Language
