#include <eepp/ui/doc/languages/kotlin.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addKotlin() {

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
			  { "object", "keyword" },
			  { "case", "keyword" },
			  { "sealed", "keyword" },
			  { "lateinit", "literal" },
			  { "override", "keyword" },
			  { "false", "literal" },
			  { "true", "literal" },
			  { "Double", "type" },
			  { "companion", "keyword" },
			  { "break", "keyword" },
			  { "else", "keyword" },
			  { "Float", "type" },
			  { "try", "keyword" },
			  { "public", "keyword" },
			  { "if", "keyword" },
			  { "String", "type" },
			  { "var", "keyword" },
			  { "import", "keyword" },
			  { "synchronized", "keyword" },
			  { "package", "keyword" },
			  { "Long", "type" },
			  { "protected", "keyword" },
			  { "val", "keyword" },
			  { "Short", "type" },
			  { "UByte", "type" },
			  { "do", "keyword" },
			  { "assert", "keyword" },
			  { "native", "keyword" },
			  { "private", "keyword" },
			  { "UShortArray", "type" },
			  { "UInt", "type" },
			  { "implements", "keyword" },
			  { "extends", "keyword" },
			  { "catch", "keyword" },
			  { "interface", "keyword" },
			  { "UByteArray", "type" },
			  { "IntArray", "type" },
			  { "UIntArray", "type" },
			  { "ULong", "type" },
			  { "null", "literal" },
			  { "ShortArray", "type" },
			  { "ByteArray", "type" },
			  { "for", "keyword" },
			  { "UShort", "type" },
			  { "Array", "type" },
			  { "enum", "keyword" },
			  { "elseif", "keyword" },
			  { "fun", "keyword" },
			  { "final", "keyword" },
			  { "ULongArray", "type" },
			  { "then", "keyword" },
			  { "class", "keyword" },
			  { "continue", "keyword" },
			  { "instanceof", "keyword" },
			  { "while", "keyword" },
			  { "abstract", "keyword" },
			  { "super", "keyword" },
			  { "return", "keyword" },
			  { "Byte", "type" },
			  { "LongArray", "type" },
			  { "goto", "keyword" },
			  { "Boolean", "type" },
			  { "throws", "keyword" },
			  { "throw", "keyword" },
			  { "finally", "keyword" },
			  { "new", "keyword" },
			  { "static", "keyword" },
			  { "switch", "keyword" },
			  { "default", "keyword" },
			  { "transient", "keyword" },
			  { "void", "keyword" },
			  { "volatile", "keyword" },
			  { "Int", "type" },

		  },
		  "//",
		  {}

		} );

	sd.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );
	sd.setBlockComment( { "/*", "*/" } );
	return sd;
}

}}}} // namespace EE::UI::Doc::Language
