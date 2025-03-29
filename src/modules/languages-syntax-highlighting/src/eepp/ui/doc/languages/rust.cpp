#include <eepp/ui/doc/languages/x86assembly.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addRust() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "Rust",
		  { "%.rs$" },
		  {
			  { { "//.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "r#\"", "\"#", "\\" }, "string" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'.'" }, "string" },
			  { { "'[%a_][%a%d_]*" }, "keyword2" },
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
			  { "while", "keyword" },  { "for", "keyword" },	 { "u16", "keyword2" },
			  { "i16", "keyword2" },   { "usize", "keyword2" },	 { "continue", "keyword" },
			  { "char", "keyword2" },  { "i64", "keyword2" },	 { "extern", "keyword" },
			  { "fn", "keyword" },	   { "if", "keyword" },		 { "u8", "keyword2" },
			  { "f32", "keyword2" },   { "await", "keyword" },	 { "let", "keyword" },
			  { "return", "keyword" }, { "Self", "keyword" },	 { "Result", "literal" },
			  { "false", "literal" },  { "dyn", "keyword" },	 { "pub", "keyword" },
			  { "i128", "keyword2" },  { "as", "keyword" },		 { "Some", "literal" },
			  { "enum", "keyword" },   { "f64", "keyword2" },	 { "None", "literal" },
			  { "async", "keyword" },  { "mut", "keyword" },	 { "i32", "keyword2" },
			  { "move", "keyword" },   { "f128", "keyword2" },	 { "crate", "keyword" },
			  { "i8", "keyword2" },	   { "str", "keyword2" },	 { "impl", "keyword" },
			  { "in", "keyword" },	   { "break", "keyword" },	 { "else", "keyword" },
			  { "isize", "keyword2" }, { "String", "keyword2" }, { "type", "keyword" },
			  { "loop", "keyword" },   { "static", "keyword" },	 { "match", "keyword" },
			  { "Option", "literal" }, { "bool", "keyword2" },	 { "mod", "keyword" },
			  { "ref", "keyword" },	   { "super", "keyword" },	 { "self", "keyword" },
			  { "trait", "keyword" },  { "struct", "keyword" },	 { "true", "literal" },
			  { "const", "keyword" },  { "u32", "keyword2" },	 { "u64", "keyword2" },
			  { "use", "keyword" },	   { "unsafe", "keyword" },	 { "where", "keyword" },

		  },
		  "//",
		  {}

		} );

	sd.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );
}

}}}} // namespace EE::UI::Doc::Language
