#include <eepp/ui/doc/languages/x86assembly.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addObjeck() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "Objeck",
		  { "%.obs$" },
		  {
			  { { "#~", "~#" }, "comment" },
			  { { "#.*" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'\\u%x%x%x%x'" }, "string" },
			  { { "'\\?.'" }, "string" },
			  { { "-?0x%x+" }, "number" },
			  { { "-?0b[0-1]+" }, "number" },
			  { { "-?%d+[%d%.eE]*f?" }, "number" },
			  { { "-?%.?%d+f?" }, "number" },
			  { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
			  { { ":=" }, "operator" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "@[%a_][%w_]*" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "As", "keyword" },		   { "BaseArrayRef", "type" },
			  { "Bool", "type" },		   { "BoolArrayRef", "type" },
			  { "BoolRef", "type" },	   { "Byte", "type" },
			  { "ByteArrayRef", "type" },  { "ByteRef", "type" },
			  { "Char", "type" },		   { "CharArrayRef", "type" },
			  { "CharRef", "type" },	   { "Float", "type" },
			  { "FloatArrayRef", "type" }, { "FloatRef", "type" },
			  { "Func2Ref", "type" },	   { "Func3Ref", "type" },
			  { "Func4Ref", "type" },	   { "FuncRef", "type" },
			  { "Int", "type" },		   { "IntArrayRef", "type" },
			  { "IntRef", "type" },		   { "New", "keyword" },
			  { "Nil", "type" },		   { "Parent", "keyword" },
			  { "String", "type" },		   { "StringArrayRef", "type" },
			  { "abstract", "keyword" },   { "alias", "keyword" },
			  { "and", "keyword" },		   { "break", "keyword" },
			  { "bundle", "keyword" },	   { "class", "keyword" },
			  { "consts", "keyword" },	   { "continue", "keyword" },
			  { "critical", "keyword" },   { "do", "keyword" },
			  { "each", "keyword" },	   { "else", "keyword" },
			  { "enum", "keyword" },	   { "false", "literal" },
			  { "for", "keyword" },		   { "from", "keyword" },
			  { "function", "keyword" },   { "if", "keyword" },
			  { "implements", "keyword" }, { "in", "keyword" },
			  { "interface", "keyword" },  { "label", "keyword" },
			  { "leaving", "keyword" },	   { "method", "keyword" },
			  { "native", "keyword" },	   { "not", "keyword" },
			  { "or", "keyword" },		   { "other", "keyword" },
			  { "private", "keyword" },	   { "public", "keyword" },
			  { "return", "keyword" },	   { "reverse", "keyword" },
			  { "select", "keyword" },	   { "static", "keyword" },
			  { "true", "literal" },	   { "use", "keyword" },
			  { "virtual", "keyword" },	   { "while", "keyword" },
			  { "xor", "keyword" },

		  },
		  "#",
		  {}

		} );

	sd.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );
	return sd;
}

}}}} // namespace EE::UI::Doc::Language
