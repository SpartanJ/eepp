#include <eepp/ui/doc/languages/java.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addJava() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "Java",
		  { "%.java$", "%.bsh$" },
		  {
			  { { "//.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "'\\x%x?%x?%x?%x'" }, "string" },
			  { { "'\\u%x%x%x%x'" }, "string" },
			  { { "'\\?.'" }, "string" },
			  { { "-?0x%x+" }, "number" },
			  { { "-?%d+[%d%.eE]*f?" }, "number" },
			  { { "-?%.?%d+f?" }, "number" },
			  { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "int", "type" },		   { "then", "keyword" },
			  { "new", "keyword" },		   { "continue", "keyword" },
			  { "default", "keyword" },	   { "char", "type" },
			  { "long", "type" },		   { "short", "type" },
			  { "extends", "keyword" },	   { "false", "literal" },
			  { "for", "keyword" },		   { "goto", "keyword" },
			  { "while", "keyword" },	   { "null", "literal" },
			  { "else", "keyword" },	   { "final", "keyword" },
			  { "enum", "keyword" },	   { "break", "keyword" },
			  { "if", "keyword" },		   { "elseif", "keyword" },
			  { "native", "keyword" },	   { "public", "keyword" },
			  { "throws", "keyword" },	   { "private", "keyword" },
			  { "finally", "keyword" },	   { "abstract", "keyword" },
			  { "volatile", "keyword" },   { "switch", "keyword" },
			  { "double", "type" },		   { "return", "keyword" },
			  { "import", "keyword" },	   { "try", "keyword" },
			  { "class", "keyword" },	   { "void", "keyword" },
			  { "byte", "type" },		   { "static", "keyword" },
			  { "super", "keyword" },	   { "boolean", "type" },
			  { "assert", "keyword" },	   { "throw", "keyword" },
			  { "catch", "keyword" },	   { "true", "literal" },
			  { "implements", "keyword" }, { "instanceof", "keyword" },
			  { "package", "keyword" },	   { "transient", "keyword" },
			  { "float", "type" },		   { "synchronized", "keyword" },
			  { "case", "keyword" },	   { "interface", "keyword" },
			  { "protected", "keyword" },  { "do", "keyword" },

		  },
		  "//",
		  {}

		} );

	sd.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );
	return sd;
}

}}}} // namespace EE::UI::Doc::Language
