#include <eepp/ui/doc/languages/jule.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addJule() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "Jule",
		  { "%.jule$" },
		  {
			  { { "//.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "\"", "[\"\n]", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "`", "`", "\\" }, "string" },
			  { { "^%s*#[%a_][%w_]*" }, "keyword" },
			  { { "0[oO][0-7]+_?[0-7]+" }, "number" },
			  { { "-?0x[%x_]+" }, "number" },
			  { { "-?%d+_%d" }, "number" },
			  { { "-?0b[01]+_?[01]+" }, "number" },
			  { { "-?%d+[%d%.eE]*f?" }, "number" },
			  { { "-?%.?%d+f?" }, "number" },
			  { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
			  { { "(%w+)::([%a_][%w_]*)%(" }, { "normal", "keyword2", "function" } },
			  { { "(%w+)::([%a_][%w_]*)" }, { "normal", "keyword2", "keyword2" } },
			  { { "(struct)%s+([%a_][%w_]*)" }, { "normal", "keyword", "keyword2" } },
			  { { "(impl)%s+([%a_][%w_]*)" }, { "normal", "keyword", "keyword2" } },
			  { { "(enum)%s+([%a_][%w_]*)" }, { "normal", "keyword", "keyword2" } },
			  { { "(trait)%s+([%a_][%w_]*)" }, { "normal", "keyword", "keyword2" } },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "([%a_][%w_]+)%[([%a_][%w_]*)%]%f[(]" }, { "normal", "function", "keyword2" } },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "int", "keyword2" },	 { "str", "keyword2" },	  { "impl", "keyword" },
			  { "continue", "keyword" }, { "i16", "keyword2" },	  { "bool", "keyword2" },
			  { "uintptr", "keyword2" }, { "integ", "keyword" },  { "fall", "keyword" },
			  { "i8", "keyword2" },		 { "u32", "keyword2" },	  { "any", "keyword2" },
			  { "match", "keyword" },	 { "i64", "keyword2" },	  { "false", "literal" },
			  { "for", "keyword" },		 { "goto", "keyword" },	  { "const", "keyword" },
			  { "u64", "keyword2" },	 { "select", "keyword" }, { "unsafe", "keyword" },
			  { "else", "keyword" },	 { "f64", "keyword2" },	  { "enum", "keyword" },
			  { "cpp", "keyword" },		 { "struct", "keyword" }, { "break", "keyword" },
			  { "use", "keyword" },		 { "if", "keyword" },	  { "let", "keyword" },
			  { "in", "keyword" },		 { "f32", "keyword2" },	  { "i32", "keyword2" },
			  { "map", "keyword2" },	 { "trait", "keyword" },  { "mut", "keyword" },
			  { "error", "keyword" },	 { "u8", "keyword2" },	  { "byte", "keyword2" },
			  { "static", "keyword" },	 { "defer", "keyword" },  { "co", "keyword" },
			  { "uint", "keyword2" },	 { "self", "keyword" },	  { "true", "literal" },
			  { "chan", "keyword" },	 { "nil", "literal" },	  { "rune", "keyword2" },
			  { "ret", "keyword" },		 { "fn", "keyword" },	  { "u16", "keyword2" },
			  { "type", "keyword" },

		  },
		  "//",
		  {}

		} );

	sd.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );
}
}}}} // namespace EE::UI::Doc::Language
