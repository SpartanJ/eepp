#include <eepp/ui/doc/languages/odin.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addJai() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

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
			  { { "([%a_][%w_]+)(%s+::%s+)(%w+%s*)%f[(]" },
				{ "normal", "function", "operator", "keyword" } },
			  { { "([%a_][%w_]+)(%s+::%s+)%f[(]" }, { "normal", "function", "operator" } },
			  { { "([%a_][%w_]+)(%s+::%s+)(struct%s*)%f[{]" },
				{ "normal", "type", "operator", "keyword" } },
			  { { "([%a_][%w_]+)(%s+::%s+)(enum%s*)(%w+%s)%f[{]" },
				{ "normal", "type", "operator", "keyword", "type" } },
			  { { "[<>~=+-*/]=" }, "operator" },
			  { { "[%+%-=/%*%^%%<>!~|&:]" }, "operator" },
			  { { "%$[%a_][%w_]*" }, "operator" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "[#@][%a_][%w_]*" }, "type" },
			  { { "[#@]%b()" }, "type" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "u8", "type" },			  { "u16", "type" },		{ "u32", "type" },
			  { "u64", "type" },		  { "s8", "type" },			{ "s16", "type" },
			  { "s32", "type" },		  { "s64", "type" },		{ "int", "type" },
			  { "float", "type" },		  { "float32", "type" },	{ "float64", "type" },
			  { "Any", "type" },		  { "array", "type" },		{ "bool", "type" },
			  { "enum", "keyword" },	  { "null", "type" },		{ "pointer", "type" },
			  { "procedure", "keyword" }, { "string", "type" },		{ "struct", "keyword" },
			  { "void", "type" },		  { "complex", "type" },	{ "const", "keyword" },
			  { "union", "keyword" },	  { "defer", "keyword" },	{ "do", "keyword" },
			  { "for", "keyword" },		  { "macro", "keyword" },	{ "remove", "keyword" },
			  { "bit_set", "keyword" },	  { "switch", "keyword" },	{ "if", "keyword" },
			  { "case", "keyword" },	  { "foreign", "keyword" }, { "new", "keyword" },
			  { "inline", "keyword" },	  { "import", "keyword" },	{ "false", "literal" },
			  { "assert", "type" },		  { "return", "keyword" },	{ "true", "literal" },
			  { "when", "keyword" },	  { "using", "keyword" },	{ "continue", "keyword" },
			  { "in", "keyword" },		  { "break", "keyword" },	{ "else", "keyword" },
			  { "while", "keyword" },	  { "count", "keyword" },	{ "delete", "keyword" },

		  },
		  "//",
		  {}

		} );

	sd.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );
	sd.setBlockComment( { "/*", "*/" } );
	return sd;
}

}}}} // namespace EE::UI::Doc::Language
