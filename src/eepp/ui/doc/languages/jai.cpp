#include <eepp/ui/doc/languages/odin.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addJai() {

	SyntaxDefinitionManager::instance()->add(

		{ "Jai",
		  { "%.jai$" },
		  {
			  { { "//.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "0b[01_]+" }, "number" },
			  { { "0o[0-7_]+" }, "number" },
			  { { "0[dz][%d_]+" }, "number" },
			  { { "0x[%da-fA-F_]+" }, "number" },
			  { { "-?%d+[%d%._e]*i?" }, "number" },
			  { { "([%a_][%w_]+)(%s+::%s+)(%w+%s+)%f[(]" },
				{ "normal", "function", "operator", "keyword" } },
			  { { "([%a_][%w_]+)(%s+::%s+)%f[(]" }, { "normal", "function", "operator" } },
			  { { "([%a_][%w_]+)(%s+::%s+)(struct%s+)%f[{]" },
				{ "normal", "keyword2", "operator", "keyword" } },
			  { { "([%a_][%w_]+)(%s+::%s+)(enum%s+)(%w+%s)%f[{]" },
				{ "normal", "keyword2", "operator", "keyword", "keyword2" } },
			  { { "[<>~=+-*/]=" }, "operator" },
			  { { "[%+%-=/%*%^%%<>!~|&:]" }, "operator" },
			  { { "%$[%a_][%w_]*" }, "operator" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "[#@][%a_][%w_]*" }, "keyword2" },
			  { { "[#@]%b()" }, "keyword2" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "u8", "keyword2" },		  { "u16", "keyword2" },	 { "u32", "keyword2" },
			  { "u64", "keyword2" },	  { "s8", "keyword2" },		 { "s16", "keyword2" },
			  { "s32", "keyword2" },	  { "s64", "keyword2" },	 { "int", "keyword2" },
			  { "float", "keyword2" },	  { "float32", "keyword2" }, { "float64", "keyword2" },
			  { "Any", "keyword2" },	  { "array", "keyword2" },	 { "bool", "keyword2" },
			  { "enum", "keyword" },	  { "null", "keyword2" },	 { "pointer", "keyword2" },
			  { "procedure", "keyword" }, { "string", "keyword2" },	 { "struct", "keyword" },
			  { "void", "keyword2" },	  { "complex", "keyword2" }, { "const", "keyword" },
			  { "union", "keyword" },	  { "defer", "keyword" },	 { "do", "keyword" },
			  { "for", "keyword" },		  { "macro", "keyword" },	 { "remove", "keyword" },
			  { "bit_set", "keyword" },	  { "switch", "keyword" },	 { "if", "keyword" },
			  { "case", "keyword" },	  { "foreign", "keyword" },	 { "new", "keyword" },
			  { "inline", "keyword" },	  { "import", "keyword" },	 { "false", "literal" },
			  { "assert", "keyword2" },	  { "return", "keyword" },	 { "true", "literal" },
			  { "when", "keyword" },	  { "using", "keyword" },	 { "continue", "keyword" },
			  { "in", "keyword" },		  { "break", "keyword" },	 { "else", "keyword" },
			  { "while", "keyword" },	  { "count", "keyword" },	 { "delete", "keyword" },

		  },
		  "//",
		  {}

		} );
}

}}}} // namespace EE::UI::Doc::Language
