#include <eepp/ui/doc/languages/angelscript.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addAngelScript() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "AngelScript",
		  { "%.as$", "%.asc$" },
		  {
			  { { "//.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "#", "\n" }, "comment" },
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
			  { { "[%a_][%w_]*@" }, "type" },
			  { { "[%-%+!~@%?:&|%^<>%*/=%%]" }, "operator" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "float", "type" },		  { "uint64", "type" },		  { "uint", "type" },
			  { "int64", "type" },		  { "mixin", "keyword" },	  { "set", "keyword" },
			  { "get", "keyword" },		  { "uint16", "type" },		  { "funcdef", "keyword" },
			  { "false", "literal" },	  { "shared", "keyword" },	  { "uint32", "type" },
			  { "override", "keyword" },  { "class", "keyword" },	  { "int16", "type" },
			  { "external", "keyword" },  { "default", "keyword" },	  { "uint8", "type" },
			  { "protected", "keyword" }, { "int8", "type" },		  { "typedef", "keyword" },
			  { "switch", "keyword" },	  { "private", "keyword" },	  { "final", "keyword" },
			  { "if", "keyword" },		  { "int", "type" },		  { "import", "keyword" },
			  { "catch", "keyword" },	  { "return", "keyword" },	  { "case", "keyword" },
			  { "and", "operator" },	  { "while", "keyword" },	  { "do", "keyword" },
			  { "cast", "keyword" },	  { "break", "keyword" },	  { "continue", "keyword" },
			  { "else", "keyword" },	  { "try", "keyword" },		  { "true", "literal" },
			  { "function", "keyword" },  { "bool", "type" },		  { "is", "operator" },
			  { "property", "keyword" },  { "const", "keyword" },	  { "void", "type" },
			  { "abstract", "keyword" },  { "or", "operator" },		  { "for", "keyword" },
			  { "explicit", "keyword" },  { "int32", "type" },		  { "xor", "operator" },
			  { "auto", "keyword" },	  { "namespace", "keyword" }, { "double", "type" },
			  { "null", "literal" },	  { "enum", "keyword" },	  { "interface", "keyword" },

		  },
		  "//",
		  {}

		} );

	sd.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );
	sd.setBlockComment( { "/*", "*/" } );
	return sd;
}

}}}} // namespace EE::UI::Doc::Language
