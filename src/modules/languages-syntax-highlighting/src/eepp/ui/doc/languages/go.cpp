#include <eepp/ui/doc/languages/go.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addGo() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "Go",
		  { "%.go$" },
		  {
			  { { "//.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "`", "`", "\\" }, "string" },
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
			  { "elseif", "keyword" },		{ "int", "keyword2" },
			  { "uint8", "keyword2" },		{ "int64", "keyword2" },
			  { "go", "keyword" },			{ "continue", "keyword" },
			  { "float64", "keyword2" },	{ "default", "keyword" },
			  { "switch", "keyword" },		{ "bool", "keyword2" },
			  { "map", "keyword2" },		{ "int16", "keyword2" },
			  { "uintptr", "keyword2" },	{ "return", "keyword" },
			  { "var", "keyword" },			{ "error", "keyword2" },
			  { "import", "keyword" },		{ "byte", "keyword2" },
			  { "defer", "keyword" },		{ "int8", "keyword2" },
			  { "complex128", "keyword2" }, { "range", "keyword" },
			  { "false", "literal" },		{ "uint", "keyword2" },
			  { "uint64", "keyword2" },		{ "for", "keyword" },
			  { "const", "keyword" },		{ "select", "keyword" },
			  { "complex64", "keyword2" },	{ "true", "literal" },
			  { "else", "keyword" },		{ "chan", "keyword" },
			  { "func", "keyword" },		{ "int32", "keyword2" },
			  { "float32", "keyword2" },	{ "uint16", "keyword2" },
			  { "package", "keyword" },		{ "nil", "literal" },
			  { "rune", "keyword2" },		{ "struct", "keyword" },
			  { "break", "keyword" },		{ "uint32", "keyword2" },
			  { "string", "keyword2" },		{ "case", "keyword" },
			  { "if", "keyword" },			{ "interface", "keyword" },
			  { "fallthrough", "keyword" }, { "type", "keyword" },

		  },
		  "//",
		  {}

		} );

	sd.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );
	return sd;
}

}}}} // namespace EE::UI::Doc::Language
