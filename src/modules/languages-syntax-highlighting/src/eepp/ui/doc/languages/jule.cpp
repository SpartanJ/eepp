#include <eepp/ui/doc/languages/jule.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addJule() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "Jule",
		  { "%.jule$" },
		  {
			  { { "//.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "\"", "[\"\n]", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "`", "`", "\\" }, "string" },
			  { { "#[%a_][%w_]*" }, "keyword" },
			  { { "0[oO][0-7]+_?[0-7]+" }, "number" },
			  { { "-?0x[%x_]+" }, "number" },
			  { { "-?%d+_%d" }, "number" },
			  { { "-?0b[01]+_?[01]+" }, "number" },
			  { { "-?%d+[%d%.eE]*f?" }, "number" },
			  { { "-?%.?%d+f?" }, "number" },
			  { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
			  { { "(%w+)::([%a_][%w_]*)%(" }, { "normal", "type", "function" } },
			  { { "(%w+)::([%a_][%w_]*)" }, { "normal", "type", "type" } },
			  { { "(struct)%s+([%a_][%w_]*)" }, { "normal", "keyword", "type" } },
			  { { "(impl)%s+([%a_][%w_]*)" }, { "normal", "keyword", "type" } },
			  { { "(enum)%s+([%a_][%w_]*)" }, { "normal", "keyword", "type" } },
			  { { "(trait)%s+([%a_][%w_]*)" }, { "normal", "keyword", "type" } },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "([%a_][%w_]+)%[([%a_][%w_]*)%]%f[(]" }, { "normal", "function", "type" } },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "int", "type" },		 { "str", "type" },		  { "impl", "keyword" },
			  { "continue", "keyword" }, { "i16", "type" },		  { "bool", "type" },
			  { "uintptr", "type" },	 { "integ", "keyword" },  { "fall", "keyword" },
			  { "i8", "type" },			 { "u32", "type" },		  { "any", "type" },
			  { "match", "keyword" },	 { "i64", "type" },		  { "false", "literal" },
			  { "for", "keyword" },		 { "goto", "keyword" },	  { "const", "keyword" },
			  { "u64", "type" },		 { "select", "keyword" }, { "unsafe", "keyword" },
			  { "else", "keyword" },	 { "f64", "type" },		  { "enum", "keyword" },
			  { "cpp", "keyword" },		 { "struct", "keyword" }, { "break", "keyword" },
			  { "use", "keyword" },		 { "if", "keyword" },	  { "let", "keyword" },
			  { "in", "keyword" },		 { "f32", "type" },		  { "i32", "type" },
			  { "map", "type" },		 { "trait", "keyword" },  { "mut", "keyword" },
			  { "error", "keyword" },	 { "u8", "type" },		  { "byte", "type" },
			  { "static", "keyword" },	 { "defer", "keyword" },  { "co", "keyword" },
			  { "uint", "type" },		 { "self", "keyword" },	  { "true", "literal" },
			  { "chan", "keyword" },	 { "nil", "literal" },	  { "rune", "type" },
			  { "ret", "keyword" },		 { "fn", "keyword" },	  { "u16", "type" },
			  { "type", "keyword" },

		  },
		  "//",
		  {}

		} );

	sd.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );
	sd.setBlockComment( { "/*", "*/" } );
	return sd;
}
}}}} // namespace EE::UI::Doc::Language
