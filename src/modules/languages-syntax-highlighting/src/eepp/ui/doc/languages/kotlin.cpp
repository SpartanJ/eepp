#include <eepp/ui/doc/languages/kotlin.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addKotlin() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "Kotlin",
		  { "%.kt$" },
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
			  { { "%@[%a_][%w_]*" }, "function" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "object", "keyword" },		 { "case", "keyword" },
			  { "sealed", "keyword" },		 { "lateinit", "literal" },
			  { "override", "keyword" },	 { "false", "literal" },
			  { "true", "literal" },		 { "Double", "keyword2" },
			  { "companion", "keyword" },	 { "break", "keyword" },
			  { "else", "keyword" },		 { "Float", "keyword2" },
			  { "try", "keyword" },			 { "public", "keyword" },
			  { "if", "keyword" },			 { "String", "keyword2" },
			  { "var", "keyword" },			 { "import", "keyword" },
			  { "synchronized", "keyword" }, { "package", "keyword" },
			  { "Long", "keyword2" },		 { "protected", "keyword" },
			  { "val", "keyword" },			 { "Short", "keyword2" },
			  { "UByte", "keyword2" },		 { "do", "keyword" },
			  { "assert", "keyword" },		 { "native", "keyword" },
			  { "private", "keyword" },		 { "UShortArray", "keyword2" },
			  { "UInt", "keyword2" },		 { "implements", "keyword" },
			  { "extends", "keyword" },		 { "catch", "keyword" },
			  { "interface", "keyword" },	 { "UByteArray", "keyword2" },
			  { "IntArray", "keyword2" },	 { "UIntArray", "keyword2" },
			  { "ULong", "keyword2" },		 { "null", "literal" },
			  { "ShortArray", "keyword2" },	 { "ByteArray", "keyword2" },
			  { "for", "keyword" },			 { "UShort", "keyword2" },
			  { "Array", "keyword2" },		 { "enum", "keyword" },
			  { "elseif", "keyword" },		 { "fun", "keyword" },
			  { "final", "keyword" },		 { "ULongArray", "keyword2" },
			  { "then", "keyword" },		 { "class", "keyword" },
			  { "continue", "keyword" },	 { "instanceof", "keyword" },
			  { "while", "keyword" },		 { "abstract", "keyword" },
			  { "super", "keyword" },		 { "return", "keyword" },
			  { "Byte", "keyword2" },		 { "LongArray", "keyword2" },
			  { "goto", "keyword" },		 { "Boolean", "keyword2" },
			  { "throws", "keyword" },		 { "throw", "keyword" },
			  { "finally", "keyword" },		 { "new", "keyword" },
			  { "static", "keyword" },		 { "switch", "keyword" },
			  { "default", "keyword" },		 { "transient", "keyword" },
			  { "void", "keyword" },		 { "volatile", "keyword" },
			  { "Int", "keyword2" },

		  },
		  "//",
		  {}

		} );

	sd.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );
}

}}}} // namespace EE::UI::Doc::Language
