#include <eepp/ui/doc/languages/u.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addU() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "Ü",
		  { "%.u$", "%.uh$" },
		  {
			  { { "//.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "\"", "[\"\n]", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "(class|template|namespace|enum)\\s+([A-Za-z]\\w*)" },
				{ "keyword", "keyword", "keyword2" },
				"",
				SyntaxPatternMatchType::RegEx },
			  { { "$%(([%a_]%w*)%)" }, { "operator", "keyword2", "keyword2" } },
			  { { "^%s*(import)%s+([<%\"][%w%d%.%\\%/%_%-]+[>%\"])" },
				{ "keyword", "keyword", "literal" } },
			  { { "cpp_number_parser" }, "number", "", SyntaxPatternMatchType::Parser },
			  { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "ust%:%:[%w_]*" }, "keyword2" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {

			  { "pretty_main", "function" },
			  { "fn", "keyword" },
			  { "op", "keyword" },
			  { "var", "keyword" },
			  { "auto", "keyword" },
			  { "return", "keyword" },
			  { "yield", "keyword" },
			  { "label", "keyword" },
			  { "while", "keyword" },
			  { "loop", "keyword" },
			  { "break", "keyword" },
			  { "continue", "keyword" },
			  { "if", "keyword" },
			  { "static_if", "keyword" },
			  { "enable_if", "keyword" },
			  { "if_coro_advance", "keyword" },
			  { "else", "keyword" },
			  { "move", "keyword" },
			  { "take", "keyword" },
			  { "byval", "keyword" },
			  { "tup", "keyword" },
			  { "struct", "keyword" },
			  { "class", "keyword" },
			  { "final", "keyword" },
			  { "polymorph", "keyword" },
			  { "interface", "keyword" },
			  { "abstract", "keyword" },
			  { "non_sync", "keyword" },
			  { "ordered", "keyword" },
			  { "nodiscard", "keyword" },
			  { "nomangle", "keyword" },
			  { "call_conv", "keyword" },
			  { "virtual", "keyword" },
			  { "override", "keyword" },
			  { "pure", "keyword" },
			  { "generator", "keyword" },
			  { "async", "keyword" },
			  { "await", "keyword" },
			  { "namespace", "keyword" },
			  { "public", "keyword" },
			  { "private", "keyword" },
			  { "protected", "keyword" },
			  { "true", "literal" },
			  { "false", "literal" },
			  { "mut", "keyword" },
			  { "imut", "keyword" },
			  { "constexpr", "keyword" },
			  { "const", "keyword" },
			  { "zero_init", "keyword" },
			  { "this", "keyword" },
			  { "base", "keyword" },
			  { "constructor", "keyword" },
			  { "destructor", "keyword" },
			  { "conversion_constructor", "keyword" },
			  { "static_assert", "keyword" },
			  { "halt", "keyword" },
			  { "safe", "keyword" },
			  { "type", "keyword" },
			  { "typeinfo", "keyword" },
			  { "same_type", "keyword" },
			  { "typeof", "keyword" },
			  { "template", "keyword" },
			  { "enum", "keyword" },
			  { "cast_ref", "keyword" },
			  { "cast_imut", "keyword" },
			  { "alloca", "keyword" },
			  { "as", "keyword" },
			  { "is", "keyword" },
			  { "import", "keyword" },
			  { "export", "keyword" },
			  { "embed", "keyword" },
			  { "default", "keyword" },
			  { "delete", "keyword" },
			  { "for", "keyword" },
			  { "with", "keyword" },
			  { "do", "keyword" },
			  { "switch", "keyword" },
			  { "case", "keyword" },
			  { "typename", "keyword" },
			  { "lambda", "keyword" },
			  { "static", "keyword" },
			  { "package", "keyword" },
			  { "module", "keyword" },
			  { "mixin", "keyword" },
			  { "thread_local", "keyword" },
			  { "unsafe", "keyword" },
			  { "cast_ref_unsafe", "keyword" },
			  { "cast_mut", "keyword" },
			  { "uninitialized", "keyword" },
			  { "void", "keyword2" },
			  { "bool", "keyword2" },
			  { "i8", "keyword2" },
			  { "u8", "keyword2" },
			  { "i16", "keyword2" },
			  { "u16", "keyword2" },
			  { "i32", "keyword2" },
			  { "u32", "keyword2" },
			  { "i64", "keyword2" },
			  { "u64", "keyword2" },
			  { "i128", "keyword2" },
			  { "u128", "keyword2" },
			  { "i256", "keyword2" },
			  { "u256", "keyword2" },
			  { "i512", "keyword2" },
			  { "u512", "keyword2" },
			  { "i1024", "keyword2" },
			  { "u1024", "keyword2" },
			  { "i2048", "keyword2" },
			  { "u2048", "keyword2" },
			  { "i4096", "keyword2" },
			  { "u4096", "keyword2" },
			  { "f16", "keyword2" },
			  { "f32", "keyword2" },
			  { "f80", "keyword2" },
			  { "f64", "keyword2" },
			  { "f128", "keyword2" },
			  { "char8", "keyword2" },
			  { "char16", "keyword2" },
			  { "char32", "keyword2" },
			  { "byte8", "keyword2" },
			  { "byte16", "keyword2" },
			  { "byte32", "keyword2" },
			  { "byte64", "keyword2" },
			  { "byte128", "keyword2" },
			  { "byte256", "keyword2" },
			  { "byte512", "keyword2" },
			  { "byte1024", "keyword2" },
			  { "byte2048", "keyword2" },
			  { "byte4096", "keyword2" },
			  { "size_type", "keyword2" },
			  { "ssize_type", "keyword2" } },
		  "//",
		  {},
		  "u" } );

	sd.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );
}

}}}} // namespace EE::UI::Doc::Language
