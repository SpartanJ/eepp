#include <eepp/ui/doc/languages/jsx.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addJSX() {

	auto& sd =
		SyntaxDefinitionManager::instance()
			->add( { "JSX",
					 { "%.jsx$" },
					 {
						 { { "//.-\n" }, "comment" },
						 { { "/%*", "%*/" }, "comment" },
						 { { "\"", "\"", "\\" }, "string" },
						 { { "'", "'", "\\" }, "string" },
						 { { "`", "`", "\\" }, "string" },
						 { { "/[%+%-%*%^%!%=%&%|%?%:%;%,%(%[%{%<%>%\\%\"].*%f[/]",
							 "/[igmsuyd\n]?[igmsuyd\n]?[igmsuyd\n]?", "\\" },
						   "string" },
						 { { "%f[^<]![%a_][%w%_%-]*" }, "keyword2" },
						 { { "%f[^<][%a_][%w%_%-]*" }, "function" },
						 { { "%f[^<]/[%a_][%w%_%-]*" }, "function" },
						 { { "([%a_-][%w-_]*)(%\?\?)(=)%f[%{%\"]" },
						   { "normal", "keyword", "normal", "operator" } },
						 { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
						 { { "js_number_parser" }, "number", "", SyntaxPatternMatchType::Parser },
						 { { "[%a_][%w_]*%f[(]" }, "function" },
						 { { "[%a_][%w_]*" }, "symbol" },
					 },
					 {
						 { "async", "keyword" },	  { "await", "keyword" },
						 { "break", "keyword" },	  { "case", "keyword" },
						 { "catch", "keyword" },	  { "class", "keyword" },
						 { "const", "keyword" },	  { "continue", "keyword" },
						 { "debugger", "keyword" },	  { "default", "keyword" },
						 { "delete", "keyword" },	  { "do", "keyword" },
						 { "else", "keyword" },		  { "export", "keyword" },
						 { "extends", "keyword" },	  { "finally", "keyword" },
						 { "for", "keyword" },		  { "function", "keyword" },
						 { "get", "keyword" },		  { "if", "keyword" },
						 { "import", "keyword" },	  { "in", "keyword" },
						 { "instanceof", "keyword" }, { "let", "keyword" },
						 { "new", "keyword" },		  { "return", "keyword" },
						 { "set", "keyword" },		  { "static", "keyword" },
						 { "super", "keyword" },	  { "switch", "keyword" },
						 { "throw", "keyword" },	  { "try", "keyword" },
						 { "typeof", "keyword" },	  { "var", "keyword" },
						 { "void", "keyword" },		  { "while", "keyword" },
						 { "with", "keyword" },		  { "yield", "keyword" },
						 { "true", "literal" },		  { "false", "literal" },
						 { "null", "literal" },		  { "undefined", "literal" },
						 { "arguments", "keyword2" }, { "Infinity", "keyword2" },
						 { "NaN", "keyword2" },		  { "this", "keyword2" },
					 },
					 "//" } )
			.setLSPName( "javascriptreact" );

	sd.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );
	return sd;
}

}}}} // namespace EE::UI::Doc::Language
