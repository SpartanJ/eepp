#include <eepp/ui/doc/languages/zephir.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addZephir() {

	return SyntaxDefinitionManager::instance()
		->add(

			{ "Zephir",
			  { "%.zep$" },
			  {
				  { { "//.-\n" }, "comment" },
				  { { "/%*", "%*/" }, "comment" },
				  { { "\"", "\"", "\\" }, "string" },
				  { { "'", "'", "\\" }, "string" },
				  { { "js_regex" }, "string", "", SyntaxPatternMatchType::Parser },
				  { { "common_number_parser" }, "number", "", SyntaxPatternMatchType::Parser },
				  { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
				  { { "[%a_][%w_]*%f[(]" }, "function" },
				  { { "%$[%a][%w_]*" }, "parameter" },
				  { { "[%a_][%w_]*" }, "symbol" },

			  },
			  {
				  { "internal", "keyword" },  { "int", "type" },		   { "new", "keyword" },
				  { "continue", "keyword" },  { "default", "keyword" },	   { "isset", "keyword" },
				  { "char", "type" },		  { "likely", "keyword" },	   { "_REQUEST", "type" },
				  { "bool", "type" },		  { "this", "keyword" },	   { "loop", "keyword" },
				  { "iterator", "type" },	  { "long", "type" },		   { "Array", "type" },
				  { "namespace", "keyword" }, { "short", "type" },		   { "typeof", "keyword" },
				  { "unset", "keyword" },	  { "PHP_VERSION", "type" },   { "extends", "keyword" },
				  { "match", "keyword" },	  { "range", "type" },		   { "with", "keyword" },
				  { "undefined", "literal" }, { "false", "literal" },	   { "for", "keyword" },
				  { "goto", "keyword" },	  { "while", "keyword" },	   { "const", "keyword" },
				  { "null", "literal" },	  { "function", "keyword" },   { "else", "keyword" },
				  { "final", "keyword" },	  { "enum", "keyword" },	   { "_COOKIE", "type" },
				  { "break", "keyword" },	  { "use", "keyword" },		   { "string", "keyword" },
				  { "if", "keyword" },		  { "let", "keyword" },		   { "in", "keyword" },
				  { "native", "keyword" },	  { "public", "keyword" },	   { "unsigned", "type" },
				  { "throws", "keyword" },	  { "private", "keyword" },	   { "finally", "keyword" },
				  { "abstract", "keyword" },  { "volatile", "keyword" },   { "switch", "keyword" },
				  { "double", "type" },		  { "_SERVER", "type" },	   { "delete", "keyword" },
				  { "unlikely", "keyword" },  { "ulong", "type" },		   { "return", "keyword" },
				  { "var", "keyword" },		  { "import", "keyword" },	   { "uchar", "type" },
				  { "try", "keyword" },		  { "class", "keyword" },	   { "void", "keyword" },
				  { "static", "keyword" },	  { "inline", "keyword" },	   { "echo", "keyword" },
				  { "reverse", "keyword" },	  { "boolean", "type" },	   { "_POST", "type" },
				  { "throw", "keyword" },	  { "self", "type" },		   { "catch", "keyword" },
				  { "true", "literal" },	  { "implements", "keyword" }, { "stdClass", "type" },
				  { "Boolean", "type" },	  { "instanceof", "keyword" }, { "_SESSION", "type" },
				  { "require", "keyword" },	  { "transient", "keyword" },  { "Date", "type" },
				  { "float", "type" },		  { "PHP_EOL", "type" },	   { "fetch", "keyword" },
				  { "case", "keyword" },	  { "_GET", "type" },		   { "export", "keyword" },
				  { "interface", "keyword" }, { "protected", "keyword" },  { "do", "keyword" },
				  { "window", "type" },		  { "empty", "keyword" },

			  },
			  "//",
			  {}

			} )
		.setFoldRangeType( FoldRangeType::Braces )
		.setFoldBraces( { { '}', '{' } } );
	;
}

}}}} // namespace EE::UI::Doc::Language
