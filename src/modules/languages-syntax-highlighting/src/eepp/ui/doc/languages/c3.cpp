#include <eepp/ui/doc/languages/c3.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addC3() {

	SyntaxDefinitionManager::instance()->add(

		{ "C3",
		  { "%.c3$" },
		  {
			  { { "//.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "<%*", "%*>" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "`", "`" }, "string" },
			  { { "(enum)%s+([%a][%w_]*)" }, { "keyword", "keyword", "keyword2" } },
			  { { "(interface)%s+([%a][%w_]*)" }, { "keyword", "keyword", "keyword2" } },
			  { { "(alias)%s+([%a][%w_]*)" }, { "keyword", "keyword", "keyword2" } },
			  { { "(struct)%s+([%a][%w_]*)" }, { "keyword", "keyword", "keyword2" } },
			  { { "(typedef)%s+([%a][%w_]*)" }, { "keyword", "keyword", "keyword2" } },
			  { { "^%s*(import)%s+([%a][%w_%:,%s]*)%f[@;\n]" },
				{ "normal", "keyword", "literal" } },
			  { { "^%s*(module)%s+([%a][%w_%:%s%{%},]*)%s*%f[@;\n]" },
				{ "normal", "keyword", "literal" } },
			  { { "(fn)%s+([%a][%w_]*%*?%\?\?)%s+([%a][%w_]*)%.([%a_][%w_]*)%f[%(]" },
				{ "normal", "keyword", "keyword2", "keyword2", "function" } },
			  { { "(macro)%s+([%a][%w_]*%*?%\?\?)%s+([%a][%w_]*)%.([%a_][%w_]*)%f[%(]" },
				{ "normal", "keyword", "keyword2", "keyword2", "function" } },
			  { { "(fn)%s+([%a][%w_]*%*?%\?\?)%s+([%a][%w_]*)%f[%(]" },
				{ "normal", "keyword", "keyword2", "function" } },
			  { { "common_number_parser_ob" }, "number", "", SyntaxPatternMatchType::Parser },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "[<>~=+-*/%?]=" }, "operator" },
			  { { "%.%." }, "operator" },
			  { { "(@)([%a][%w_]*)" }, { "normal", "operator", "keyword" } },
			  { { "$[%a_][%w_]*" }, "symbol" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "anyfault", "keyword2" }, { "int", "keyword2" },	  { "continue", "keyword" },
			  { "asm", "keyword" },		  { "BigInt", "keyword2" },	  { "default", "keyword" },
			  { "char", "keyword2" },	  { "isz", "keyword2" },	  { "define", "keyword" },
			  { "bool", "keyword2" },	  { "attribute", "keyword" }, { "faultdef", "keyword" },
			  { "extern", "keyword" },	  { "long", "keyword2" },	  { "foreach", "keyword" },
			  { "String", "keyword2" },	  { "union", "keyword" },	  { "short", "keyword2" },
			  { "float16", "keyword2" },  { "any", "keyword2" },	  { "usz", "keyword2" },
			  { "false", "literal" },	  { "for", "keyword" },		  { "while", "keyword" },
			  { "const", "keyword" },	  { "$if", "keyword" },		  { "float128", "keyword2" },
			  { "null", "literal" },	  { "else", "keyword" },	  { "enum", "keyword" },
			  { "$else", "keyword" },	  { "struct", "keyword" },	  { "break", "keyword" },
			  { "uptr", "keyword2" },	  { "if", "keyword" },		  { "$endif", "keyword" },
			  { "alias", "keyword" },	  { "macro", "keyword" },	  { "fault", "keyword" },
			  { "switch", "keyword" },	  { "nextcase", "keyword" },  { "double", "keyword2" },
			  { "typeid", "keyword2" },	  { "ulong", "keyword2" },	  { "return", "keyword" },
			  { "var", "keyword" },		  { "import", "keyword" },	  { "tlocal", "keyword" },
			  { "try", "keyword" },		  { "int128", "keyword2" },	  { "void", "keyword" },
			  { "byte", "keyword2" },	  { "static", "keyword" },	  { "inline", "keyword" },
			  { "defer", "keyword" },	  { "module", "keyword" },	  { "uint", "keyword2" },
			  { "iptr", "keyword2" },	  { "assert", "keyword" },	  { "typedef", "keyword" },
			  { "def", "keyword" },		  { "catch", "keyword" },	  { "true", "literal" },
			  { "ushort", "keyword2" },	  { "float", "keyword2" },	  { "local", "keyword" },
			  { "foreach_r", "keyword" }, { "fn", "keyword" },		  { "case", "keyword" },
			  { "bitstruct", "keyword" }, { "interface", "keyword" }, { "uint128", "keyword2" },
			  { "distinct", "keyword" },  { "do", "keyword" },

		  },
		  "//",
		  {}

		} );
}

}}}} // namespace EE::UI::Doc::Language
