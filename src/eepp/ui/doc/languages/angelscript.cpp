#include <eepp/ui/doc/languages/angelscript.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addAngelScript() {

	SyntaxDefinitionManager::instance()->add(

		{ "AngelScript",
		  { "%.as$", "%.asc$" },
		  {
			  { { "//.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "#", "[^\\]\n" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "-?0[xX]%x+" }, "number" },
			  { { "-?0[bB][0-1]+" }, "number" },
			  { { "-?0[oO][0-7]+" }, "number" },
			  { { "-?%d+[%d%.eE]*f?" }, "number" },
			  { { "-?%.?%d+f?" }, "number" },
			  { { "&inout" }, "keyword" },
			  { { "&in" }, "keyword" },
			  { { "&out" }, "keyword" },
			  { { "[%a_][%w_]*@" }, "keyword2" },
			  { { "[%-%+!~@%?:&|%^<>%*/=%%]" }, "operator" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "float", "keyword2" },	  { "uint64", "keyword2" },	  { "uint", "keyword2" },
			  { "int64", "keyword2" },	  { "mixin", "keyword" },	  { "set", "keyword" },
			  { "get", "keyword" },		  { "uint16", "keyword2" },	  { "funcdef", "keyword" },
			  { "false", "literal" },	  { "shared", "keyword" },	  { "uint32", "keyword2" },
			  { "override", "keyword" },  { "class", "keyword" },	  { "int16", "keyword2" },
			  { "external", "keyword" },  { "default", "keyword" },	  { "uint8", "keyword2" },
			  { "protected", "keyword" }, { "int8", "keyword2" },	  { "typedef", "keyword" },
			  { "switch", "keyword" },	  { "private", "keyword" },	  { "final", "keyword" },
			  { "if", "keyword" },		  { "int", "keyword2" },	  { "import", "keyword" },
			  { "catch", "keyword" },	  { "return", "keyword" },	  { "case", "keyword" },
			  { "and", "operator" },	  { "while", "keyword" },	  { "do", "keyword" },
			  { "cast", "keyword" },	  { "break", "keyword" },	  { "continue", "keyword" },
			  { "else", "keyword" },	  { "try", "keyword" },		  { "true", "literal" },
			  { "function", "keyword" },  { "bool", "keyword2" },	  { "is", "operator" },
			  { "property", "keyword" },  { "const", "keyword" },	  { "void", "keyword2" },
			  { "abstract", "keyword" },  { "or", "operator" },		  { "for", "keyword" },
			  { "explicit", "keyword" },  { "int32", "keyword2" },	  { "xor", "operator" },
			  { "auto", "keyword" },	  { "namespace", "keyword" }, { "double", "keyword2" },
			  { "null", "literal" },	  { "enum", "keyword" },	  { "interface", "keyword" },

		  },
		  "//",
		  {}

		} );
}

}}}} // namespace EE::UI::Doc::Language
