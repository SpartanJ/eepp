#include <eepp/ui/doc/languages/x86assembly.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addRust() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "Rust",
		  { "%.rs$" },
		  {
			  { { "//.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "r#\"", "\"#", "\\" }, "string" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'.'" }, "string" },
			  { { "'[%a_][%a%d_]*" }, "type" },
			  { { "0[oO_][0-7]+" }, "number" },
			  { { "-?0x[%x_]+" }, "number" },
			  { { "-?%d+_%d" }, "number" },
			  { { "-?%d+[%d%.eE]*f?" }, "number" },
			  { { "-?%.?%d+f?" }, "number" },
			  { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
			  { { "[%a_][%w_]*!%f[%[(]" }, "function" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "while", "keyword" },  { "for", "keyword" },	{ "u16", "type" },
			  { "i16", "type" },	   { "usize", "type" },		{ "continue", "keyword" },
			  { "char", "type" },	   { "i64", "type" },		{ "extern", "keyword" },
			  { "fn", "keyword" },	   { "if", "keyword" },		{ "u8", "type" },
			  { "f32", "type" },	   { "await", "keyword" },	{ "let", "keyword" },
			  { "return", "keyword" }, { "Self", "keyword" },	{ "Result", "literal" },
			  { "false", "literal" },  { "dyn", "keyword" },	{ "pub", "keyword" },
			  { "i128", "type" },	   { "as", "keyword" },		{ "Some", "literal" },
			  { "enum", "keyword" },   { "f64", "type" },		{ "None", "literal" },
			  { "async", "keyword" },  { "mut", "keyword" },	{ "i32", "type" },
			  { "move", "keyword" },   { "f128", "type" },		{ "crate", "keyword" },
			  { "i8", "type" },		   { "str", "type" },		{ "impl", "keyword" },
			  { "in", "keyword" },	   { "break", "keyword" },	{ "else", "keyword" },
			  { "isize", "type" },	   { "String", "type" },	{ "type", "keyword" },
			  { "loop", "keyword" },   { "static", "keyword" }, { "match", "keyword" },
			  { "Option", "literal" }, { "bool", "type" },		{ "mod", "keyword" },
			  { "ref", "keyword" },	   { "super", "keyword" },	{ "self", "keyword" },
			  { "trait", "keyword" },  { "struct", "keyword" }, { "true", "literal" },
			  { "const", "keyword" },  { "u32", "type" },		{ "u64", "type" },
			  { "use", "keyword" },	   { "unsafe", "keyword" }, { "where", "keyword" },

		  },
		  "//",
		  {}

		} );

	sd.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );
	sd.setBlockComment( { "/*", "*/" } );
	return sd;
}

}}}} // namespace EE::UI::Doc::Language
