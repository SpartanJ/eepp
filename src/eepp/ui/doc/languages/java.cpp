#include <eepp/ui/doc/languages/java.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addJava() {

	SyntaxDefinitionManager::instance()->add(

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
			  { "int", "keyword2" },	   { "then", "keyword" },
			  { "new", "keyword" },		   { "continue", "keyword" },
			  { "default", "keyword" },	   { "char", "keyword2" },
			  { "long", "keyword2" },	   { "short", "keyword2" },
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
			  { "double", "keyword2" },	   { "return", "keyword" },
			  { "import", "keyword" },	   { "try", "keyword" },
			  { "class", "keyword" },	   { "void", "keyword" },
			  { "byte", "keyword2" },	   { "static", "keyword" },
			  { "super", "keyword" },	   { "boolean", "keyword2" },
			  { "assert", "keyword" },	   { "throw", "keyword" },
			  { "catch", "keyword" },	   { "true", "literal" },
			  { "implements", "keyword" }, { "instanceof", "keyword" },
			  { "package", "keyword" },	   { "transient", "keyword" },
			  { "float", "keyword2" },	   { "synchronized", "keyword" },
			  { "case", "keyword" },	   { "interface", "keyword" },
			  { "protected", "keyword" },  { "do", "keyword" },

		  },
		  "//",
		  {}

		} );
}

}}}} // namespace EE::UI::Doc::Language
