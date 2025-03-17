#include <eepp/ui/doc/languages/rave.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addRave() {
	auto& sd = SyntaxDefinitionManager::instance()->add(
		{ "Rave",
		  { "%.rave$" },
		  {
			  { { "//.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "0[oO_][0-7]+" }, "number" },
			  { { "-?0x[%x_]+" }, "number" },
			  { { "-?%d+_%d" }, "number" },
			  { { "-?%d+[%d%.eE]*f?" }, "number" },
			  { { "-?%.?%d+f?" }, "number" },
			  { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
			  { { ":=" }, "operator" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },
		  },
		  {
			  { "extern", "keyword" },
			  { "import", "keyword" },
			  { "break", "keyword" },
			  { "continue", "keyword" },
			  { "if", "keyword" },
			  { "else", "keyword" },
			  { "in", "keyword" },
			  { "likely", "keyword" },
			  { "unlikely", "keyword" },
			  { "foreach", "keyword" },
			  { "switch", "keyword" },
			  { "case", "keyword" },
			  { "cast", "keyword" },
			  { "while", "keyword" },
			  { "for", "keyword" },
			  { "return", "keyword" },
			  { "defer", "keyword" },
			  { "fdefer", "keyword" },
			  { "const", "keyword2" },
			  { "alias", "keyword2" },
			  { "void", "keyword2" },
			  { "bool", "keyword2" },
			  { "char", "keyword2" },
			  { "uchar", "keyword2" },
			  { "short", "keyword2" },
			  { "ushort", "keyword2" },
			  { "int", "keyword2" },
			  { "uint", "keyword2" },
			  { "long", "keyword2" },
			  { "ulong", "keyword2" },
			  { "cent", "keyword2" },
			  { "ucent", "keyword2" },
			  { "float", "keyword2" },
			  { "double", "keyword2" },
			  { "auto", "keyword2" },
			  { "half", "keyword2" },
			  { "bhalf", "keyword2" },
			  { "real", "keyword2" },
			  { "short8", "keyword2" },
			  { "int4", "keyword2" },
			  { "float4", "keyword2" },
			  { "int8", "keyword2" },
			  { "float8", "keyword2" },
			  { "true", "literal" },
			  { "false", "literal" },
			  { "struct", "literal" },
			  { "namespace", "literal" },
			  
		  },
		  "//" } );

	sd.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );
}

}}}} // namespace EE::UI::Doc::Language
