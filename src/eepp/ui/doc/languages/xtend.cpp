#include <eepp/ui/doc/languages/xtend.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addXtend() {

	SyntaxDefinitionManager::instance()->add(

		{ "Xtend",
		  { "%.xtend$" },
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
			  { "abstract", "keyword" },
			  { "annotation", "keyword" },
			  { "as", "keyword" },
			  { "case", "keyword" },
			  { "catch", "keyword" },
			  { "class", "keyword" },
			  { "create", "keyword" },
			  { "def", "keyword" },
			  { "default", "keyword" },
			  { "dispatch", "keyword" },
			  { "do", "keyword" },
			  { "else", "keyword" },
			  { "enum", "keyword" },
			  { "extends", "keyword" },
			  { "extension", "keyword" },
			  { "final", "keyword" },
			  { "finally", "keyword" },
			  { "for", "keyword" },
			  { "if", "keyword" },
			  { "implements", "keyword" },
			  { "import", "keyword" },
			  { "interface", "keyword" },
			  { "instanceof", "keyword" },
			  { "it", "keyword" },
			  { "new", "keyword" },
			  { "override", "keyword" },
			  { "package", "keyword" },
			  { "private", "keyword" },
			  { "protected", "keyword" },
			  { "public", "keyword" },
			  { "return", "keyword" },
			  { "self", "keyword" },
			  { "static", "keyword" },
			  { "super", "keyword" },
			  { "switch", "keyword" },
			  { "synchronized", "keyword" },
			  { "this", "keyword" },
			  { "throw", "keyword" },
			  { "throws", "keyword" },
			  { "try", "keyword" },
			  { "typeof", "keyword" },
			  { "val", "keyword" },
			  { "var", "keyword" },
			  { "while", "keyword" },
			  { "AFTER", "keyword" },
			  { "BEFORE", "keyword" },
			  { "ENDFOR", "keyword" },
			  { "ENDIF", "keyword" },
			  { "FOR", "keyword" },
			  { "IF", "keyword" },
			  { "SEPARATOR", "keyword" },
			  { "boolean", "keyword2" },
			  { "byte", "keyword2" },
			  { "char", "keyword2" },
			  { "double", "keyword2" },
			  { "float", "keyword2" },
			  { "int", "keyword2" },
			  { "long", "keyword2" },
			  { "short", "keyword2" },
			  { "void", "keyword2" },
			  { "Boolean", "keyword2" },
			  { "Byte", "keyword2" },
			  { "Character", "keyword2" },
			  { "Double", "keyword2" },
			  { "Float", "keyword2" },
			  { "Integer", "keyword2" },
			  { "Long", "keyword2" },
			  { "Short", "keyword2" },
			  { "String", "keyword2" },
			  { "true", "literal" },
			  { "false", "literal" },
			  { "null", "literal" },
		  },
		  "//",
		  {}

		} );
}

}}}} // namespace EE::UI::Doc::Language
