#include <eepp/ui/doc/languages/haxe.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addHaxe() {

	SyntaxDefinitionManager::instance()
		->add( { "Haxe Compiler Arguments",
				 { "%.hxml$" },
				 {
					 { { "#.*" }, "comment" },
					 { { "%-[%-%w_]*" }, "keyword" },
					 { { "(%.)(%u[%w_]*)" }, { "normal", "normal", "type" } },
				 },
				 {},
				 "#" } )
		.setVisible( false );

	SyntaxDefinitionManager::instance()
		->add( { "HaxeStringInterpolation",
				 {},
				 {
					 { { "%${", "}", "\\" }, "keyword", ".hx" },
					 { { "%$", "%s", "\\" }, "keyword", ".hx" },
					 { { "[^ ]" }, "string" },
				 },
				 {} } )
		.setVisible( false );

	SyntaxDefinitionManager::instance()
		->add( { "HaxeRegularExpressions",
				 {},
				 {
					 { { "[%[%]%(%)]" }, "string" },
					 { { "[%.%*%+%?%^%$%|%-]" }, "operator" },
				 },
				 {} } )
		.setVisible( false );

	return SyntaxDefinitionManager::instance()->add(
		{ "Haxe",
		  { "%.hx$" },
		  {
			  { { "%~%/", "%/[igmsu]*" }, "type", "HaxeRegularExpressions" },
			  { { "%.%.%." }, "operator" },
			  { { "(%<)(%u[%w_]*)(%>*)" }, { "normal", "operator", "type", "operator" } },
			  { { "(%#%s*[%a_]*)(.*\n)" }, { "normal", "keyword", "normal" } },
			  { { "(import%s+)(%u[%w]*)" }, { "normal", "keyword", "type" } },
			  { { "(import%s+)([%w%.]*%.)(%u[%w]*)" }, { "normal", "keyword", "normal", "type" } },
			  { { "(abstract%s+)(%u[%w_]*%s*%()(%s*%u[%w_]*)" },
				{ "normal", "type", "normal", "type" } },
			  { { "(from%s+)(%u[%w_]*%s+)(to%s+)(%u[%w_]*)" },
				{ "normal", "keyword", "type", "keyword", "type" } },
			  { { "//.*\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string", "HaxeStringInterpolation" },
			  { { "-?%.?%d+" }, "number" },
			  { { "-?0x%x+" }, "number" },
			  { { "-?%d+%.[%deE]+" }, "number" },
			  { { "-?%d+[%deE]+" }, "number" },
			  { { "[%+%-%.=/%*%^%%<>!~|&]" }, "operator" },
			  { { "([%a_][%w_]*)(%s*%f[(])" }, { "normal", "function", "normal" } },
			  { { "[%a_][%w_]*" }, "symbol" },
			  { { "(:)(%u[%a_][%w_]*)" }, { "normal", "normal", "type" } },
			  { { "@:[%a_][%w_]*%f[(]" }, "keyword" },
			  { { "%$type" }, "keyword" },
		  },
		  {
			  { "using", "type" },		 { "true", "literal" },	   { "trace", "keyword" },
			  { "throw", "keyword" },	 { "typedef", "type" },	   { "switch", "keyword" },
			  { "try", "keyword" },		 { "static", "keyword" },  { "set", "keyword" },
			  { "return", "keyword" },	 { "public", "keyword" },  { "package", "keyword" },
			  { "do", "keyword" },		 { "default", "keyword" }, { "new", "keyword" },
			  { "private", "keyword" },	 { "macro", "type" },	   { "cast", "keyword" },
			  { "class", "keyword" },	 { "case", "keyword" },	   { "this", "keyword" },
			  { "continue", "keyword" }, { "else", "keyword" },	   { "extern", "type" },
			  { "break", "keyword" },	 { "extends", "type" },	   { "interface", "keyword" },
			  { "abstract", "type" },	 { "for", "keyword" },	   { "override", "keyword" },
			  { "function", "type" },	 { "never", "keyword" },   { "get", "keyword" },
			  { "final", "keyword" },	 { "if", "keyword" },	   { "implements", "type" },
			  { "var", "type" },		 { "catch", "keyword" },   { "import", "keyword" },
			  { "false", "literal" },	 { "in", "keyword" },	   { "while", "keyword" },
			  { "inline", "keyword" },	 { "enum", "keyword" },	   { "null", "literal" },

		  },
		  "//",
		  {} } ).setBlockComment( { "/*", "*/" } );
}

}}}} // namespace EE::UI::Doc::Language
