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
			  { "elseif", "keyword" },
			  { "int", "type" },
			  { "uint8", "type" },
			  { "int64", "type" },
			  { "go", "keyword" },
			  { "continue", "keyword" },
			  { "float64", "type" },
			  { "default", "keyword" },
			  { "switch", "keyword" },
			  { "bool", "type" },
			  { "map", "type" },
			  { "int16", "type" },
			  { "uintptr", "type" },
			  { "return", "keyword" },
			  { "var", "keyword" },
			  { "error", "type" },
			  { "import", "keyword" },
			  { "byte", "type" },
			  { "defer", "keyword" },
			  { "int8", "type" },
			  { "complex128", "type" },
			  { "range", "keyword" },
			  { "false", "literal" },
			  { "uint", "type" },
			  { "uint64", "type" },
			  { "for", "keyword" },
			  { "const", "keyword" },
			  { "select", "keyword" },
			  { "complex64", "type" },
			  { "true", "literal" },
			  { "else", "keyword" },
			  { "chan", "keyword" },
			  { "func", "keyword" },
			  { "int32", "type" },
			  { "float32", "type" },
			  { "uint16", "type" },
			  { "package", "keyword" },
			  { "nil", "literal" },
			  { "rune", "type" },
			  { "struct", "keyword" },
			  { "break", "keyword" },
			  { "uint32", "type" },
			  { "string", "type" },
			  { "case", "keyword" },
			  { "if", "keyword" },
			  { "interface", "keyword" },
			  { "fallthrough", "keyword" },
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
