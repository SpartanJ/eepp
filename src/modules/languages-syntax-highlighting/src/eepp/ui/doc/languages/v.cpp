#include <eepp/ui/doc/languages/v.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addV() {

	return SyntaxDefinitionManager::instance()
		->add( { "V",
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
					 { { "%$%s?[%a_][%w_]*" }, "type" },
					 { { "%@%s?[%a_][%w_]*" }, "type" },

				 },
				 {
					 { "thread", "type" },		{ "struct", "keyword" },
					 { "string", "type" },		{ "go", "keyword" },
					 { "i128", "type" },		{ "bool", "type" },
					 { "import", "keyword" },	{ "as", "keyword" },
					 { "goto", "keyword" },		{ "union", "keyword" },
					 { "u32", "type" },			{ "const", "keyword" },
					 { "f32", "type" },			{ "u16", "type" },
					 { "unsafe", "keyword" },	{ "voidptr", "type" },
					 { "i64", "type" },			{ "typeof", "keyword" },
					 { "if", "keyword" },		{ "fn", "keyword" },
					 { "continue", "keyword" }, { "char", "type" },
					 { "atomic", "keyword" },	{ "isreftype", "keyword" },
					 { "u64", "type" },			{ "true", "literal" },
					 { "u128", "type" },		{ "u8", "type" },
					 { "chan", "type" },		{ "assert", "keyword" },
					 { "i16", "type" },			{ "byte", "type" },
					 { "type", "keyword" },		{ "__offsetof", "keyword" },
					 { "for", "keyword" },		{ "i8", "type" },
					 { "int", "type" },			{ "is", "keyword" },
					 { "defer", "keyword" },	{ "interface", "keyword" },
					 { "false", "literal" },	{ "or", "keyword" },
					 { "pub", "keyword" },		{ "lock", "keyword" },
					 { "map", "type" },			{ "module", "keyword" },
					 { "rune", "type" },		{ "shared", "keyword" },
					 { "mut", "keyword" },		{ "match", "keyword" },
					 { "static", "keyword" },	{ "asm", "keyword" },
					 { "none", "literal" },		{ "return", "keyword" },
					 { "in", "keyword" },		{ "else", "keyword" },
					 { "break", "keyword" },	{ "rlock", "keyword" },
					 { "select", "keyword" },	{ "enum", "keyword" },
					 { "f64", "type" },			{ "sizeof", "keyword" },

				 },

				 "//",
				 {}

		} )
		.setExtensionPriority( true )
		.setFoldRangeType( FoldRangeType::Braces )
		.setFoldBraces( { { '{', '}' } } );
}

}}}} // namespace EE::UI::Doc::Language
