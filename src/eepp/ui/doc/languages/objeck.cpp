#include <eepp/ui/doc/languages/x86assembly.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addObjeck() {

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
			  { { "%s+" }, "normal" },
			  { { "%w+%f[%s]" }, "normal" },

		  },
		  {
			  { "As", "keyword" },
			  { "BaseArrayRef", "keyword2" },
			  { "Bool", "keyword2" },
			  { "BoolArrayRef", "keyword2" },
			  { "BoolRef", "keyword2" },
			  { "Byte", "keyword2" },
			  { "ByteArrayRef", "keyword2" },
			  { "ByteRef", "keyword2" },
			  { "Char", "keyword2" },
			  { "CharArrayRef", "keyword2" },
			  { "CharRef", "keyword2" },
			  { "Float", "keyword2" },
			  { "FloatArrayRef", "keyword2" },
			  { "FloatRef", "keyword2" },
			  { "Func2Ref", "keyword2" },
			  { "Func3Ref", "keyword2" },
			  { "Func4Ref", "keyword2" },
			  { "FuncRef", "keyword2" },
			  { "Int", "keyword2" },
			  { "IntArrayRef", "keyword2" },
			  { "IntRef", "keyword2" },
			  { "New", "keyword" },
			  { "Nil", "keyword2" },
			  { "Parent", "keyword" },
			  { "String", "keyword2" },
			  { "StringArrayRef", "keyword2" },
			  { "abstract", "keyword" },
			  { "alias", "keyword" },
			  { "and", "keyword" },
			  { "break", "keyword" },
			  { "bundle", "keyword" },
			  { "class", "keyword" },
			  { "consts", "keyword" },
			  { "continue", "keyword" },
			  { "critical", "keyword" },
			  { "do", "keyword" },
			  { "each", "keyword" },
			  { "else", "keyword" },
			  { "enum", "keyword" },
			  { "false", "literal" },
			  { "for", "keyword" },
			  { "from", "keyword" },
			  { "function", "keyword" },
			  { "if", "keyword" },
			  { "implements", "keyword" },
			  { "in", "keyword" },
			  { "interface", "keyword" },
			  { "label", "keyword" },
			  { "leaving", "keyword" },
			  { "method", "keyword" },
			  { "native", "keyword" },
			  { "not", "keyword" },
			  { "or", "keyword" },
			  { "other", "keyword" },
			  { "private", "keyword" },
			  { "public", "keyword" },
			  { "return", "keyword" },
			  { "reverse", "keyword" },
			  { "select", "keyword" },
			  { "static", "keyword" },
			  { "true", "literal" },
			  { "use", "keyword" },
			  { "virtual", "keyword" },
			  { "while", "keyword" },
			  { "xor", "keyword" },

		  },
		  "#",
		  {}

		} );

	sd.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );
}

}}}} // namespace EE::UI::Doc::Language
