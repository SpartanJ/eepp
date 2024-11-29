#include <eepp/ui/doc/languages/typescript.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addTypeScript() {

	SyntaxDefinition& ts = SyntaxDefinitionManager::instance()->add(

		{ "TypeScript",
		  { "%.ts$", "%.d.ts$" },
		  {
			  { { "//.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "`", "`", "\\" }, "string" },
			  { { "/[%+%-%*%^%!%=%&%|%?%:%;%,%(%[%{%<%>%\\].*%f[/]",
				  "/[igmsuyd\n]?[igmsuyd\n]?[igmsuyd\n]?", "\\" },
				"string" },
			  { { "0x[%da-fA-F]+" }, "number" },
			  { { "-?%d+[%d%.eE]*" }, "number" },
			  { { "-?%.?%d+" }, "number" },
			  { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
			  { { "(interface%s)([%a_][%w_]*)" }, { "normal", "keyword", "keyword2" } },
			  { { "(type%s)([%a_][%w_]*)" }, { "normal", "keyword", "keyword2" } },
			  { { "[%a_][%w_$]*%f[(]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "new", "keyword" },		   { "continue", "keyword" },
			  { "default", "keyword" },	   { "set", "keyword" },
			  { "this", "keyword2" },	   { "namespace", "keyword" },
			  { "any", "keyword2" },	   { "of", "keyword" },
			  { "typeof", "keyword" },	   { "extends", "keyword" },
			  { "with", "keyword" },	   { "undefined", "literal" },
			  { "false", "literal" },	   { "for", "keyword" },
			  { "while", "keyword" },	   { "const", "keyword" },
			  { "get", "keyword" },		   { "from", "keyword" },
			  { "null", "literal" },	   { "function", "keyword" },
			  { "else", "keyword" },	   { "constructor", "function" },
			  { "enum", "keyword" },	   { "async", "keyword" },
			  { "break", "keyword" },	   { "declare", "keyword" },
			  { "arguments", "keyword2" }, { "string", "keyword2" },
			  { "if", "keyword" },		   { "await", "keyword" },
			  { "let", "keyword" },		   { "in", "keyword" },
			  { "public", "keyword" },	   { "debugger", "keyword" },
			  { "private", "keyword" },	   { "finally", "keyword" },
			  { "abstract", "keyword" },   { "number", "keyword2" },
			  { "switch", "keyword" },	   { "delete", "keyword" },
			  { "return", "keyword" },	   { "var", "keyword" },
			  { "import", "keyword" },	   { "try", "keyword" },
			  { "as", "keyword" },		   { "class", "keyword" },
			  { "void", "keyword" },	   { "unknown", "keyword2" },
			  { "yield", "keyword" },	   { "static", "keyword" },
			  { "super", "keyword" },	   { "module", "keyword" },
			  { "boolean", "keyword2" },   { "throw", "keyword" },
			  { "catch", "keyword" },	   { "true", "literal" },
			  { "implements", "keyword" }, { "symbol", "keyword2" },
			  { "instanceof", "keyword" }, { "package", "keyword" },
			  { "require", "keyword" },	   { "Infinity", "keyword2" },
			  { "readonly", "keyword" },   { "case", "keyword" },
			  { "export", "keyword" },	   { "interface", "keyword" },
			  { "protected", "keyword" },  { "do", "keyword" },
			  { "type", "keyword2" },

		  },
		  "//",
		  {}

		} );

	ts.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );

	SyntaxDefinitionManager::instance()
		->add( { "TSX",
				 { "%.tsx$" },
				 {
					 { { "//.-\n" }, "comment" },
					 { { "/%*", "%*/" }, "comment" },
					 { { "\"", "\"", "\\" }, "string" },
					 { { "'", "'", "\\" }, "string" },
					 { { "`", "`", "\\" }, "string" },
					 { { "/[%+%-%*%^%!%=%&%|%?%:%;%,%(%[%{%<%>%\\].*%f[/]",
						 "/[igmsuyd\n]?[igmsuyd\n]?[igmsuyd\n]?", "\\" },
					   "string" },
					 { { "%f[^<]![%a_][%w%_%-]*" }, "keyword2" },
					 { { "%f[^<][%a_][%w%_%-]*" }, "function" },
					 { { "%f[^<]/[%a_][%w%_%-]*" }, "function" },
					 { { "([%a_-][%w-_]*)(%\?\?)(=)%f[%{%\"]" },
					   { "normal", "keyword", "normal", "operator" } },
					 { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
					 { { "0x[%da-fA-F]+" }, "number" },
					 { { "-?%d+[%d%.eE]*" }, "number" },
					 { { "-?%.?%d+" }, "number" },
					 { { "(interface%s)([%a_][%w_]*)" }, { "normal", "keyword", "keyword2" } },
					 { { "(type%s)([%a_][%w_]*)" }, { "normal", "keyword", "keyword2" } },
					 { { "[%a_][%w_$]*%f[(]" }, "function" },
					 { { "[%a_][%w_]*" }, "symbol" },
				 },
				 {},
				 "//" } )
		.setSymbols( ts.getSymbols() )
		.setLSPName( "typescriptreact" )
		.setFoldRangeType( FoldRangeType::Braces )
		.setFoldBraces( { { '{', '}' } } );
	;
}

}}}} // namespace EE::UI::Doc::Language
