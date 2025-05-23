#include <eepp/ui/doc/languages/ispc.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addISPC() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "ISPC",
		  { "%.ispc$", "%.isph$", "%.ih$" },
		  {
			  { { "//.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "(#%s*include)%s+([<%\"][%w%d%.%\\%/%_%-]+[>%\"])" },
				{ "keyword", "keyword", "literal" } },
			  { { "L?\"", "[\"\n]", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "c_number_parser" }, "number", "", SyntaxPatternMatchType::Parser },
			  { { "(struct)%s+([%a_][%w_]+)" }, { "keyword", "keyword", "keyword2" } },
			  { { "(enum)%s+([%a_][%w_]+)" }, { "keyword", "keyword", "keyword2" } },
			  { { "(cdo|cfor|cif|cwhiledo|else|for|foreach|foreach_active|foreach_tiled|foreach_"
				  "unique|goto|if|switch|while)\\s*(?=\\()" },
				{ "normal", "keyword", "keyword" },
				"",
				SyntaxPatternMatchType::RegEx },
			  { { "[%a_][%w_]*%s*%f[(]" }, "function" },
			  { { "@llvm%.[a-zA-Z_0-9%.]*" }, "function" },
			  { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
			  { { "#[%a_][%w_]*" }, "symbol" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {

			  { "assert", "keyword" },
			  { "break", "keyword" },
			  { "case", "keyword" },
			  { "cdo", "keyword" },
			  { "cfor", "keyword" },
			  { "cif", "keyword" },
			  { "cwhile", "keyword" },
			  { "continue", "keyword" },
			  { "default", "keyword" },
			  { "do", "keyword" },
			  { "else", "keyword" },
			  { "for", "keyword" },
			  { "foreach", "keyword" },
			  { "foreach_active", "keyword" },
			  { "foreach_tiled", "keyword" },
			  { "foreach_unique", "keyword" },
			  { "goto", "keyword" },
			  { "if", "keyword" },
			  { "in", "keyword" },
			  { "return", "keyword" },
			  { "switch", "keyword" },
			  { "sync", "keyword" },
			  { "task", "keyword" },
			  { "while", "keyword" },
			  { "launch", "keyword" },
			  { "invoke_sycl", "keyword" },
			  { "delete", "keyword" },
			  { "new", "keyword" },
			  { "sizeof", "keyword" },
			  { "alloca", "keyword" },
			  { "const", "keyword" },
			  { "extern", "keyword" },
			  { "inline", "keyword" },
			  { "noinline", "keyword" },
			  { "static", "keyword" },
			  { "uniform", "keyword" },
			  { "unmasked", "keyword" },
			  { "varying", "keyword" },
			  { "template", "keyword" },
			  { "typedef", "keyword" },
			  { "typename", "keyword" },
			  { "export", "keyword" },
			  { "signed", "keyword2" },
			  { "unsigned", "keyword2" },
			  { "soa", "keyword" },
			  { "__vectorcall", "keyword" },
			  { "__regcall", "keyword" },
			  { "__attribute__", "keyword" },
			  { "volatile", "keyword" },

			  { "bool", "keyword2" },
			  { "double", "keyword2" },
			  { "float", "keyword2" },
			  { "float16", "keyword2" },
			  { "int", "keyword2" },
			  { "uint", "keyword2" },
			  { "int8", "keyword2" },
			  { "uint8", "keyword2" },
			  { "int16", "keyword2" },
			  { "uint16", "keyword2" },
			  { "int32", "keyword2" },
			  { "uint32", "keyword2" },
			  { "int64", "keyword2" },
			  { "uint64", "keyword2" },
			  { "void", "keyword" },
			  { "struct", "keyword" },
			  { "enum", "keyword" },
			  { "size_t", "keyword2" },
			  { "ptrdiff_t", "keyword2" },
			  { "intptr_t", "keyword2" },
			  { "uintptr_t", "keyword2" },

			  { "true", "literal" },
			  { "false", "literal" },
			  { "NULL", "literal" },
			  { "programIndex", "literal" },
			  { "programCount", "literal" },
			  { "threadIndex", "literal" },
			  { "threadIndex0", "literal" },
			  { "threadIndex1", "literal" },
			  { "threadIndex2", "literal" },
			  { "threadCount", "literal" },
			  { "taskIndex", "literal" },
			  { "taskIndex0", "literal" },
			  { "taskIndex1", "literal" },
			  { "taskIndex2", "literal" },
			  { "taskCount", "literal" },

			  { "#if", "keyword" },
			  { "#ifdef", "keyword" },
			  { "#ifndef", "keyword" },
			  { "#elif", "keyword" },
			  { "#else", "keyword" },
			  { "#endif", "keyword" },
			  { "#define", "keyword" },
			  { "#undef", "keyword" },
			  { "#include", "keyword" },
			  { "#line", "keyword" },
			  { "#error", "keyword" },
			  { "#pragma", "keyword" },
			  { "#warning ", "keyword" },
			  { "#elifdef ", "keyword" },
			  { "#elifndef ", "keyword" },
		  },
		  "//",
		  {}

		} );

	sd.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );
}

}}}} // namespace EE::UI::Doc::Language
