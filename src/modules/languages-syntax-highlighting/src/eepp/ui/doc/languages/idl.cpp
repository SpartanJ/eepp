#include <eepp/ui/doc/languages/idl.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addIdl() {

	return SyntaxDefinitionManager::instance()
		->add(

			{ "IDL",
			  { "%.idl$" },
			  {
				  { { "//.-\n" }, "comment" },
				  { { "/%*", "%*/" }, "comment" },
				  { { "\"", "[\"\n]", "\\" }, "string" },
				  { { "'", "'", "\\" }, "string" },
				  { { "%x%x%x%x%x%x%x%x%-%x%x%x%x%-%x%x%x%x%-%x%x%x%x%-%x%x%x%x%x%x%x%x%x%x%x%x" },
					"number" },
				  { { "c_number_parser" }, "number", "", SyntaxPatternMatchType::Parser },
				  { { "[%+%-=/%*%^%%<>!~|&%[%]%{%}]" }, "operator" },
				  { { "[%a_][%w_]*%f[(]" }, "function" },
				  { { "#?[%a_][%w_]*" }, "symbol" },

			  },
			  {
				  { "#pragma", "keyword" },	  { "truncatable", "keyword" },
				  { "interface", "keyword" }, { "switch", "keyword" },
				  { "typedef", "keyword" },	  { "support", "keyword" },
				  { "readonly", "keyword" },  { "#if", "keyword" },
				  { "raises", "keyword" },	  { "module", "keyword" },
				  { "abstract", "keyword" },  { "oneway", "keyword" },
				  { "case", "keyword" },	  { "exception", "keyword" },
				  { "fixed", "type" },		  { "wstring", "type" },
				  { "public", "keyword" },	  { "attribute", "keyword" },
				  { "sequence", "type" },	  { "enum", "keyword" },
				  { "custom", "keyword" },	  { "struct", "keyword" },
				  { "native", "keyword" },	  { "float", "type" },
				  { "factory", "keyword" },	  { "out", "keyword" },
				  { "in", "keyword" },		  { "default", "keyword" },
				  { "inout", "keyword" },	  { "#include", "keyword" },
				  { "local", "keyword" },	  { "const", "keyword" },
				  { "union", "keyword" },	  { "valuetype", "keyword" },
				  { "ValueBase", "type" },	  { "#ifndef", "keyword" },
				  { "#elif", "keyword" },	  { "any", "type" },
				  { "boolean", "type" },	  { "char", "type" },
				  { "Object", "type" },		  { "double", "type" },
				  { "TRUE", "keyword" },	  { "long", "type" },
				  { "#else", "keyword" },	  { "#endif", "keyword" },
				  { "octet", "type" },		  { "FALSE", "keyword" },
				  { "short", "type" },		  { "string", "type" },
				  { "unsigned", "type" },	  { "void", "type" },
				  { "private", "keyword" },	  { "wchar", "type" },
				  { "#warning", "keyword" },  { "int", "type" },
				  { "#define", "keyword" },	  { "#undef", "keyword" },
				  { "context", "keyword" },	  { "#ifdef", "keyword" },

			  },
			  "//",
			  {}

			} )
		.setFoldRangeType( FoldRangeType::Braces )
		.setFoldBraces( {
			{ '{', '}' },
		} )
		.setBlockComment( { "/*", "*/" } );
}

}}}} // namespace EE::UI::Doc::Language
