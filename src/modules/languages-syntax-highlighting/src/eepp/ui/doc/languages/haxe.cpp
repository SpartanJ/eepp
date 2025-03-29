#include <eepp/ui/doc/languages/haxe.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addHaxe() {

	SyntaxDefinitionManager::instance()->add(
		{ "Haxe Compiler Arguments",
		  { "%.hxml$" },
		  {
			  { { "#.*" }, "comment" },
			  { { "%-[%-%w_]*" }, "keyword" },
			  { { "(%.)(%u[%w_]*)" }, { "normal", "normal", "keyword2" } },
		  },
		  {},
		  "#" } );

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

	SyntaxDefinitionManager::instance()->add(
		{ "Haxe",
		  { "%.hx$" },
		  {
			  { { "%~%/", "%/[igmsu]*" }, "keyword2", "HaxeRegularExpressions" },
			  { { "%.%.%." }, "operator" },
			  { { "(%<)(%u[%w_]*)(%>*)" }, { "normal", "operator", "keyword2", "operator" } },
			  { { "(%#%s*[%a_]*)(.*\n)" }, { "normal", "keyword", "normal" } },
			  { { "(import%s+)(%u[%w]*)" }, { "normal", "keyword", "keyword2" } },
			  { { "(import%s+)([%w%.]*%.)(%u[%w]*)" },
				{ "normal", "keyword", "normal", "keyword2" } },
			  { { "(abstract%s+)(%u[%w_]*%s*%()(%s*%u[%w_]*)" },
				{ "normal", "keyword2", "normal", "keyword2" } },
			  { { "(from%s+)(%u[%w_]*%s+)(to%s+)(%u[%w_]*)" },
				{ "normal", "keyword", "keyword2", "keyword", "keyword2" } },
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
			  { { "(:)(%u[%a_][%w_]*)" }, { "normal", "normal", "keyword2" } },
			  { { "@:[%a_][%w_]*%f[(]" }, "keyword" },
			  { { "%$type" }, "keyword" },
		  },
		  {
			  { "using", "keyword2" },	  { "true", "literal" },	 { "trace", "keyword" },
			  { "throw", "keyword" },	  { "typedef", "keyword2" }, { "switch", "keyword" },
			  { "try", "keyword" },		  { "static", "keyword" },	 { "set", "keyword" },
			  { "return", "keyword" },	  { "public", "keyword" },	 { "package", "keyword" },
			  { "do", "keyword" },		  { "default", "keyword" },	 { "new", "keyword" },
			  { "private", "keyword" },	  { "macro", "keyword2" },	 { "cast", "keyword" },
			  { "class", "keyword" },	  { "case", "keyword" },	 { "this", "keyword" },
			  { "continue", "keyword" },  { "else", "keyword" },	 { "extern", "keyword2" },
			  { "break", "keyword" },	  { "extends", "keyword2" }, { "interface", "keyword" },
			  { "abstract", "keyword2" }, { "for", "keyword" },		 { "override", "keyword" },
			  { "function", "keyword2" }, { "never", "keyword" },	 { "get", "keyword" },
			  { "final", "keyword" },	  { "if", "keyword" },		 { "implements", "keyword2" },
			  { "var", "keyword2" },	  { "catch", "keyword" },	 { "import", "keyword" },
			  { "false", "literal" },	  { "in", "keyword" },		 { "while", "keyword" },
			  { "inline", "keyword" },	  { "enum", "keyword" },	 { "null", "literal" },

		  },
		  "//",
		  {} } );
}

}}}} // namespace EE::UI::Doc::Language
