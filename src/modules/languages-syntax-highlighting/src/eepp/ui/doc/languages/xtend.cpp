#include <eepp/ui/doc/languages/xtend.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addXtend() {

	return SyntaxDefinitionManager::instance()->add(

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
			  { "abstract", "keyword" },   { "annotation", "keyword" },
			  { "as", "keyword" },		   { "case", "keyword" },
			  { "catch", "keyword" },	   { "class", "keyword" },
			  { "create", "keyword" },	   { "def", "keyword" },
			  { "default", "keyword" },	   { "dispatch", "keyword" },
			  { "do", "keyword" },		   { "else", "keyword" },
			  { "enum", "keyword" },	   { "extends", "keyword" },
			  { "extension", "keyword" },  { "final", "keyword" },
			  { "finally", "keyword" },	   { "for", "keyword" },
			  { "if", "keyword" },		   { "implements", "keyword" },
			  { "import", "keyword" },	   { "interface", "keyword" },
			  { "instanceof", "keyword" }, { "it", "keyword" },
			  { "new", "keyword" },		   { "override", "keyword" },
			  { "package", "keyword" },	   { "private", "keyword" },
			  { "protected", "keyword" },  { "public", "keyword" },
			  { "return", "keyword" },	   { "self", "keyword" },
			  { "static", "keyword" },	   { "super", "keyword" },
			  { "switch", "keyword" },	   { "synchronized", "keyword" },
			  { "this", "keyword" },	   { "throw", "keyword" },
			  { "throws", "keyword" },	   { "try", "keyword" },
			  { "typeof", "keyword" },	   { "val", "keyword" },
			  { "var", "keyword" },		   { "while", "keyword" },
			  { "AFTER", "keyword" },	   { "BEFORE", "keyword" },
			  { "ENDFOR", "keyword" },	   { "ENDIF", "keyword" },
			  { "FOR", "keyword" },		   { "IF", "keyword" },
			  { "SEPARATOR", "keyword" },  { "boolean", "type" },
			  { "byte", "type" },		   { "char", "type" },
			  { "double", "type" },		   { "float", "type" },
			  { "int", "type" },		   { "long", "type" },
			  { "short", "type" },		   { "void", "type" },
			  { "Boolean", "type" },	   { "Byte", "type" },
			  { "Character", "type" },	   { "Double", "type" },
			  { "Float", "type" },		   { "Integer", "type" },
			  { "Long", "type" },		   { "Short", "type" },
			  { "String", "type" },		   { "true", "literal" },
			  { "false", "literal" },	   { "null", "literal" },
		  },
		  "//",
		  {}

		} ).setBlockComment( { "/*", "*/" } );
}

}}}} // namespace EE::UI::Doc::Language
