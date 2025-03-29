#include <eepp/ui/doc/languages/v.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addV() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "V",
		  { "%.v$", "%.vsh$" },
		  {
			  { { "//.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "`", "`", "\\" }, "string" },
			  { { "r'", "'" }, "string" },
			  { { "r\"", "\"" }, "string" },
			  { { "0x[%da-fA-F_]+" }, "number" },
			  { { "0b[01_]+" }, "number" },
			  { { "00[01234567_]+" }, "number" },
			  { { "-?%.?%d+" }, "number" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "[%+%-%*%/%%%~%&%|%^%!%=]" }, "operator" },
			  { { "%:%=" }, "operator" },
			  { { "%.%.%.?" }, "operator" },
			  { { "[%a_][%w_]*" }, "symbol" },
			  { { "%$%s?[%a_][%w_]*" }, "keyword2" },
			  { { "%@%s?[%a_][%w_]*" }, "keyword2" },

		  },
		  {
			  { "thread", "keyword2" },	   { "struct", "keyword" }, { "string", "keyword2" },
			  { "go", "keyword" },		   { "i128", "keyword2" },	{ "bool", "keyword2" },
			  { "import", "keyword" },	   { "as", "keyword" },		{ "goto", "keyword" },
			  { "union", "keyword" },	   { "u32", "keyword2" },	{ "const", "keyword" },
			  { "f32", "keyword2" },	   { "u16", "keyword2" },	{ "unsafe", "keyword" },
			  { "voidptr", "keyword2" },   { "i64", "keyword2" },	{ "typeof", "keyword" },
			  { "if", "keyword" },		   { "fn", "keyword" },		{ "continue", "keyword" },
			  { "char", "keyword2" },	   { "atomic", "keyword" }, { "isreftype", "keyword" },
			  { "u64", "keyword2" },	   { "true", "literal" },	{ "u128", "keyword2" },
			  { "u8", "keyword2" },		   { "chan", "keyword2" },	{ "assert", "keyword" },
			  { "i16", "keyword2" },	   { "byte", "keyword2" },	{ "type", "keyword" },
			  { "__offsetof", "keyword" }, { "for", "keyword" },	{ "i8", "keyword2" },
			  { "int", "keyword2" },	   { "is", "keyword" },		{ "defer", "keyword" },
			  { "interface", "keyword" },  { "false", "literal" },	{ "or", "keyword" },
			  { "pub", "keyword" },		   { "lock", "keyword" },	{ "map", "keyword2" },
			  { "module", "keyword" },	   { "rune", "keyword2" },	{ "shared", "keyword" },
			  { "mut", "keyword" },		   { "match", "keyword" },	{ "static", "keyword" },
			  { "asm", "keyword" },		   { "none", "literal" },	{ "return", "keyword" },
			  { "in", "keyword" },		   { "else", "keyword" },	{ "break", "keyword" },
			  { "rlock", "keyword" },	   { "select", "keyword" }, { "enum", "keyword" },
			  { "f64", "keyword2" },	   { "sizeof", "keyword" },

		  },
		  "//",
		  {}

		} ).setExtensionPriority( true );

	sd.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );
}

}}}} // namespace EE::UI::Doc::Language
