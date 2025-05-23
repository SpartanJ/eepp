#include <eepp/ui/doc/languages/c3.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addC3() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "C3",
		  { "%.c3t?$" },
		  {
			  { { "//.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "<%*", "%*>" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "`", "`" }, "string" },
			  { { "(enum|interface|alias|struct|typedef)\\s+([A-Za-z][\\w_]*)" },
				{ "keyword", "keyword", "keyword2" },
				"",
				SyntaxPatternMatchType::RegEx },
			  { { "^%s*(import)%s+([%a][%w_%:,%s]*)%f[@;\n]" },
				{ "normal", "keyword", "literal" } },
			  { { "^%s*(module)%s+([%a][%w_%:%s%{%},]*)%s*%f[@;\n]" },
				{ "normal", "keyword", "literal" } },
			  { { "(fn)%s+([%a][%w_]*%*?%\?\?)%s+([%a][%w_]*)%.([%a_][%w_]*)%f[%(]" },
				{ "normal", "keyword", "keyword2", "keyword2", "function" } },
			  { { "(macro)%s+([%a][%w_]*%*?%\?\?)%s+([%a][%w_]*)%.([%a_][%w_]*)%f[%(]" },
				{ "normal", "keyword", "keyword2", "keyword2", "function" } },
			  { { "(fn)%s+([%a][%w_]*%*?%\?\?)%s+([%a][%w_]*)%f[%(]" },
				{ "normal", "keyword", "keyword2", "function" } },
			  { { "common_number_parser_ob" }, "number", "", SyntaxPatternMatchType::Parser },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "[<>~=+-*/%?]=" }, "operator" },
			  { { "%.%." }, "operator" },
			  { { "(#[%a][%w_]*)" }, "keyword3" },
			  { { "(@)([%a][%w_]*)" }, { "normal", "operator", "keyword" } },
			  { { "%$?%$?[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "continue", "keyword" },
			  { "asm", "keyword" },
			  { "default", "keyword" },
			  { "define", "keyword" },
			  { "attribute", "keyword" },
			  { "faultdef", "keyword" },
			  { "extern", "keyword" },
			  { "foreach", "keyword" },
			  { "union", "keyword" },
			  { "false", "literal" },
			  { "for", "keyword" },
			  { "while", "keyword" },
			  { "const", "keyword" },
			  { "null", "literal" },
			  { "else", "keyword" },
			  { "enum", "keyword" },
			  { "struct", "keyword" },
			  { "break", "keyword" },
			  { "if", "keyword" },
			  { "alias", "keyword" },
			  { "macro", "keyword" },
			  { "fault", "keyword" },
			  { "switch", "keyword" },
			  { "nextcase", "keyword" },
			  { "return", "keyword" },
			  { "var", "keyword" },
			  { "import", "keyword" },
			  { "tlocal", "keyword" },
			  { "try", "keyword" },
			  { "void", "keyword" },
			  { "static", "keyword" },
			  { "inline", "keyword" },
			  { "defer", "keyword" },
			  { "module", "keyword" },
			  { "assert", "keyword" },
			  { "typedef", "keyword" },
			  { "def", "keyword" },
			  { "catch", "keyword" },
			  { "true", "literal" },
			  { "local", "keyword" },
			  { "foreach_r", "keyword" },
			  { "fn", "keyword" },
			  { "case", "keyword" },
			  { "bitstruct", "keyword" },
			  { "interface", "keyword" },
			  { "attrdef", "keyword" },
			  { "distinct", "keyword" },
			  { "do", "keyword" },

			  { "anyfault", "keyword2" },
			  { "int", "keyword2" },
			  { "BigInt", "keyword2" },
			  { "char", "keyword2" },
			  { "isz", "keyword2" },
			  { "bool", "keyword2" },
			  { "long", "keyword2" },
			  { "short", "keyword2" },
			  { "float16", "keyword2" },
			  { "any", "keyword2" },
			  { "usz", "keyword2" },
			  { "float128", "keyword2" },
			  { "uptr", "keyword2" },
			  { "double", "keyword2" },
			  { "typeid", "keyword2" },
			  { "ulong", "keyword2" },
			  { "int128", "keyword2" },
			  { "byte", "keyword2" },
			  { "uint", "keyword2" },
			  { "iptr", "keyword2" },
			  { "ushort", "keyword2" },
			  { "float", "keyword2" },
			  { "uint128", "keyword2" },
			  { "ichar", "keyword2" },

			  { "String", "keyword2" },
			  { "List", "keyword2" },
			  { "File", "keyword2" },
			  { "Path", "keyword2" },

			  { "$alignof", "keyword" },
			  { "$assert", "keyword" },
			  { "$case", "keyword" },
			  { "$default", "keyword" },
			  { "$defined", "keyword" },
			  { "$echo", "keyword" },
			  { "$embed", "keyword" },
			  { "$exec", "keyword" },
			  { "$else", "keyword" },
			  { "$endfor", "keyword" },
			  { "$endforeach", "keyword" },
			  { "$endif", "keyword" },
			  { "$endswitch", "keyword" },
			  { "$eval", "keyword" },
			  { "$evaltype", "keyword" },
			  { "$error", "keyword" },
			  { "$extnameof", "keyword" },
			  { "$for", "keyword" },
			  { "$foreach", "keyword" },
			  { "$if", "keyword" },
			  { "$include", "keyword" },
			  { "$nameof", "keyword" },
			  { "$offsetof", "keyword" },
			  { "$qnameof", "keyword" },
			  { "$sizeof", "keyword" },
			  { "$stringify", "keyword" },
			  { "$switch", "keyword" },
			  { "$typefrom", "keyword" },
			  { "$typeof", "keyword" },
			  { "$vacount", "keyword" },
			  { "$vatype", "keyword" },
			  { "$vaconst", "keyword" },
			  { "$vaarg", "keyword" },
			  { "$vaexpr", "keyword" },
			  { "$vasplat", "keyword" },

			  { "@align", "keyword2" },
			  { "@benchmark", "keyword2" },
			  { "@bigendian", "keyword2" },
			  { "@builtin", "keyword2" },
			  { "@cdecl", "keyword2" },
			  { "@deprecated", "keyword2" },
			  { "@dynamic", "keyword2" },
			  { "@export", "keyword2" },
			  { "@extern", "keyword2" },
			  { "@extname", "keyword2" },
			  { "@inline", "keyword2" },
			  { "@interface", "keyword2" },
			  { "@littleendian", "keyword2" },
			  { "@local", "keyword2" },
			  { "@maydiscard", "keyword2" },
			  { "@naked", "keyword2" },
			  { "@nodiscard", "keyword2" },
			  { "@noinit", "keyword2" },
			  { "@noinline", "keyword2" },
			  { "@noreturn", "keyword2" },
			  { "@nostrip", "keyword2" },
			  { "@obfuscate", "keyword2" },
			  { "@operator", "keyword2" },
			  { "@overlap", "keyword2" },
			  { "@packed", "keyword2" },
			  { "@priority", "keyword2" },
			  { "@private", "keyword2" },
			  { "@public", "keyword2" },
			  { "@pure", "keyword2" },
			  { "@reflect", "keyword2" },
			  { "@section", "keyword2" },
			  { "@stdcall", "keyword2" },
			  { "@test", "keyword2" },
			  { "@unused", "keyword2" },
			  { "@used", "keyword2" },
			  { "@veccall", "keyword2" },
			  { "@wasm", "keyword2" },
			  { "@weak", "keyword2" },
			  { "@winmain", "keyword2" },

			  { "$$BENCHMARK_FNS", "literal" },
			  { "$$BENCHMARK_NAMES", "literal" },
			  { "$$DATE", "literal" },
			  { "$$FILE", "literal" },
			  { "$$FILEPATH", "literal" },
			  { "$$FUNC", "literal" },
			  { "$$FUNCTION", "literal" },
			  { "$$LINE", "literal" },
			  { "$$LINE_RAW", "literal" },
			  { "$$MODULE", "literal" },
			  { "$$TEST_FNS", "literal" },
			  { "$$TEST_NAMES", "literal" },
			  { "$$TIME", "literal" },
		  },
		  "//",
		  {}

		} );

	sd.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );
}

}}}} // namespace EE::UI::Doc::Language
