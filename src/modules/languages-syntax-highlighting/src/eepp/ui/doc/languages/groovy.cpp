#include <eepp/ui/doc/languages/groovy.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addGroovy() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "Groovy",
		  { "%.groovy$" },
		  {
			  { { "//.*" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "%/", "%/", "\\" }, "string" },
			  { { "%$%/", "%/%$", "\\" }, "string" },
			  { { "'\\x%x?%x?%x?%x'" }, "string" },
			  { { "'\\u%x%x%x%x'" }, "string" },
			  { { "'\\?.'" }, "string" },
			  { { "-?0x%x+" }, "number" },
			  { { "-?%d+[%d%.eE]*[a-zA-Z]?" }, "number" },
			  { { "-?%.?%d+" }, "number" },
			  { { "-?[%d_+]+[a-zA-Z]?" }, "number" },
			  { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "[%a_][%w_]*%f[%[]" }, "function" },
			  { { "[A-Z]+_?[A-Z]+" }, "type" },
			  { { "[%a_][%w_]*" }, "symbol" },
			  { { "[a-zA-Z]+%.+" }, "function" },

		  },
		  {
			  { "int", "keyword" },
			  { "new", "keyword" },
			  { "continue", "keyword" },
			  { "default", "keyword" },
			  { "char", "keyword" },
			  { "BigDecimal", "keyword" },
			  { "long", "keyword" },
			  { "String", "keyword" },
			  { "Long", "keyword" },
			  { "short", "keyword" },
			  { "yields", "keyword" },
			  { "extends", "keyword" },
			  { "false", "literal" },
			  { "for", "keyword" },
			  { "goto", "keyword" },
			  { "while", "keyword" },
			  { "const", "keyword" },
			  { "null", "literal" },
			  { "permitsrecord", "keyword" },
			  { "else", "keyword" },
			  { "final", "keyword" },
			  { "enum", "keyword" },
			  { "BigInteger", "keyword" },
			  { "break", "keyword" },
			  { "if", "keyword" },
			  { "strictfp", "keyword" },
			  { "in", "keyword" },
			  { "native", "keyword" },
			  { "public", "keyword" },
			  { "throws", "keyword" },
			  { "Float", "keyword" },
			  { "private", "keyword" },
			  { "finally", "keyword" },
			  { "abstract", "keyword" },
			  { "switch", "keyword" },
			  { "Integer", "keyword" },
			  { "double", "keyword" },
			  { "trait", "keyword" },
			  { "synchronizedthis", "keyword" },
			  { "return", "keyword" },
			  { "var", "keyword" },
			  { "import", "keyword" },
			  { "try", "keyword" },
			  { "as", "keyword" },
			  { "class", "keyword" },
			  { "byte", "keyword" },
			  { "static", "keyword" },
			  { "Double", "keyword" },
			  { "super", "keyword" },
			  { "boolean", "literal" },
			  { "threadsafe", "keyword" },
			  { "assert", "keyword" },
			  { "throw", "keyword" },
			  { "def", "keyword" },
			  { "catch", "keyword" },
			  { "true", "literal" },
			  { "implements", "keyword" },
			  { "instanceof", "keyword" },
			  { "package", "keyword" },
			  { "non-sealed", "keyword" },
			  { "transient", "keyword" },
			  { "sealed", "keyword" },
			  { "float", "keyword" },
			  { "case", "keyword" },
			  { "interface", "keyword" },
			  { "protected", "keyword" },
			  { "do", "keyword" },

		  },
		  "//",
		  {}

		} );

	sd.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );
	return sd;
}

}}}} // namespace EE::UI::Doc::Language
