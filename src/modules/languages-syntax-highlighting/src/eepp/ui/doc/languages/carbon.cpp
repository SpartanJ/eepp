#include <eepp/ui/doc/languages/carbon.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addCarbon() {
	// Based in Lite-XL Rohan Vashisht implementation
	// https://github.com/RohanVashisht1234/carbon_syntax_highlighter_lite-xl
	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "Carbon",
		  { "%.carbon$" },
		  {
			  { { "\"", "\"", "\\" }, "string" },
			  { { "\"\"\"", "\"\"\"", "\\" }, "string" },
			  { { "'''", "'''", "\\" }, "string" },
			  { { "//.*" }, "comment" },
			  { { "[!%-/*?:=><+]" }, "operator" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "(packages)(%s+)([%a_][%w_]*)" }, { "normal", "keyword", "normal", "literal" } },
			  { { "(let)(%s+)([%a_][%w_]*)" }, { "normal", "keyword", "normal", "literal" } },
			  { { "(import)(%s+)([%a_][%w_]*)" }, { "normal", "keyword", "normal", "literal" } },
			  { { "(impl)(%s+)([%a_][%w_]*)" }, { "normal", "keyword", "normal", "literal" } },
			  { { "(class)(%s+)([%a_][%w_]*)" }, { "normal", "keyword", "normal", "literal" } },
			  { { "(var)(%s+)([%a_][%w_]*)" }, { "normal", "keyword", "normal", "literal" } },
			  { { "(package)(%s+)([%a_][%w_]*)" }, { "normal", "keyword", "normal", "literal" } },
			  { { "-?%d+[%d%.eE_]*" }, "number" },
			  { { "-?%.?%d+" }, "number" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "impl", "keyword2" },	{ "virtual", "keyword" },  { "f32", "keyword" },
			  { "default", "keyword" }, { "i32", "keyword" },	   { "abstract", "keyword" },
			  { "f8", "keyword" },		{ "extend", "keyword" },   { "i16", "keyword" },
			  { "Int", "keyword" },		{ "auto", "keyword" },	   { "bool", "keyword" },
			  { "Base", "keyword" },	{ "return", "keyword" },   { "f16", "keyword" },
			  { "var", "keyword" },		{ "u8", "keyword" },	   { "import", "keyword" },
			  { "i8", "keyword" },		{ "u32", "keyword" },	   { "String", "keyword" },
			  { "class", "keyword" },	{ "partial", "keyword2" }, { "Self", "keyword" },
			  { "api", "keyword" },		{ "match", "keyword" },	   { "i64", "keyword" },
			  { "for", "keyword" },		{ "while", "keyword" },	   { "u64", "keyword" },
			  { "i256", "keyword" },	{ "i128", "keyword" },	   { "else", "keyword" },
			  { "f128", "keyword" },	{ "f64", "keyword" },	   { "UInt", "keyword" },
			  { "File", "keyword" },	{ "package", "keyword" },  { "template", "keyword2" },
			  { "base", "keyword" },	{ "u128", "keyword" },	   { "fn", "keyword" },
			  { "case", "keyword" },	{ "if", "keyword" },	   { "returned", "keyword" },
			  { "u256", "keyword" },	{ "let", "keyword" },	   { "u16", "keyword" },

		  },
		  "//",
		  {}

		} );

	sd.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );
}

}}}} // namespace EE::UI::Doc::Language
