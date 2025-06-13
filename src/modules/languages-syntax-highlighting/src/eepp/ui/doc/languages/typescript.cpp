#include <eepp/system/parsermatcher.hpp>
#include <eepp/ui/doc/languages/typescript.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addTypeScript() {

	SyntaxDefinition& ts = SyntaxDefinitionManager::instance()->add(

		{ "TypeScript",
		  { "%.ts$", "%.d%.ts$" },
		  {
			  { { "//.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "`", "`", "\\" }, "string" },
			  { { "js_regex" }, "string", "", SyntaxPatternMatchType::Parser },
			  { { "js_number_parser" }, "number", "", SyntaxPatternMatchType::Parser },
			  { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
			  { { "(interface%s)([%a_][%w_]*)" }, { "normal", "keyword", "type" } },
			  { { "(type%s)([%a_][%w_]*)" }, { "normal", "keyword", "type" } },
			  { { "(if|for|while|switch|catch|with|typeof|delete|void|super|await|function)\\s*(?="
				  "\\()" },
				{ "normal", "keyword", "keyword" },
				"",
				SyntaxPatternMatchType::RegEx },
			  { { "[%a_][%w_$]*%f[(]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "new", "keyword" },		   { "continue", "keyword" },
			  { "default", "keyword" },	   { "set", "keyword" },
			  { "this", "type" },		   { "namespace", "keyword" },
			  { "any", "type" },		   { "of", "keyword" },
			  { "typeof", "keyword" },	   { "extends", "keyword" },
			  { "with", "keyword" },	   { "undefined", "literal" },
			  { "false", "literal" },	   { "for", "keyword" },
			  { "while", "keyword" },	   { "const", "keyword" },
			  { "get", "keyword" },		   { "from", "keyword" },
			  { "null", "literal" },	   { "function", "keyword" },
			  { "else", "keyword" },	   { "constructor", "function" },
			  { "enum", "keyword" },	   { "async", "keyword" },
			  { "break", "keyword" },	   { "declare", "keyword" },
			  { "arguments", "type" },	   { "string", "type" },
			  { "if", "keyword" },		   { "await", "keyword" },
			  { "let", "keyword" },		   { "in", "keyword" },
			  { "public", "keyword" },	   { "debugger", "keyword" },
			  { "private", "keyword" },	   { "finally", "keyword" },
			  { "abstract", "keyword" },   { "number", "type" },
			  { "switch", "keyword" },	   { "delete", "keyword" },
			  { "return", "keyword" },	   { "var", "keyword" },
			  { "import", "keyword" },	   { "try", "keyword" },
			  { "as", "keyword" },		   { "class", "keyword" },
			  { "void", "keyword" },	   { "unknown", "type" },
			  { "yield", "keyword" },	   { "static", "keyword" },
			  { "super", "keyword" },	   { "module", "keyword" },
			  { "boolean", "type" },	   { "throw", "keyword" },
			  { "catch", "keyword" },	   { "true", "literal" },
			  { "implements", "keyword" }, { "symbol", "type" },
			  { "instanceof", "keyword" }, { "package", "keyword" },
			  { "require", "keyword" },	   { "Infinity", "type" },
			  { "readonly", "keyword" },   { "case", "keyword" },
			  { "export", "keyword" },	   { "interface", "keyword" },
			  { "protected", "keyword" },  { "do", "keyword" },
			  { "type", "type" },		   { "is", "keyword" },

		  },
		  "//",
		  {}

		} );

	ts.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );
	ts.addAlternativeName( "ts" );

	SyntaxDefinitionManager::instance()
		->add( { "TSX",
				 { "%.tsx$" },
				 {
					 { { "//.-\n" }, "comment" },
					 { { "/%*", "%*/" }, "comment" },
					 { { "\"", "\"", "\\" }, "string" },
					 { { "'", "'", "\\" }, "string" },
					 { { "`", "`", "\\" }, "string" },
					 { { "js_regex" }, "string", "", SyntaxPatternMatchType::Parser },
					 { { "%f[^<]![%a_][%w%_%-]*" }, "type" },
					 { { "%f[^<][%a_][%w%_%-]*" }, "function" },
					 { { "%f[^<]/[%a_][%w%_%-]*" }, "function" },
					 { { "([%a_-][%w-_]*)(%\?\?)(=)%f[%{%\"]" },
					   { "normal", "keyword", "normal", "operator" } },
					 { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
					 { { "js_number_parser" }, "number", "", SyntaxPatternMatchType::Parser },
					 { { "(interface%s)([%a_][%w_]*)" }, { "normal", "keyword", "type" } },
					 { { "(type%s)([%a_][%w_]*)" }, { "normal", "keyword", "type" } },
					 { { "[%a_][%w_$]*%f[(]" }, "function" },
					 { { "[%a_][%w_]*" }, "symbol" },
				 },
				 {},
				 "//" } )
		.setSymbols( ts.getSymbols(), ts.getSymbolNames() )
		.setLSPName( "typescriptreact" )
		.setFoldRangeType( FoldRangeType::Braces )
		.setFoldBraces( { { '{', '}' } } );

	return ts;
}

}}}} // namespace EE::UI::Doc::Language
