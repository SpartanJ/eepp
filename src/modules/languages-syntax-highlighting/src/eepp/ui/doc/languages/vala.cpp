#include <eepp/ui/doc/languages/vala.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addVala() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "Vala",
		  { "%.vala$", "%.genie$" },
		  {
			  { { "//.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "\"\"\"", "\"\"\"" }, "string" },
			  { { "@\"", "\"", "\\" }, "string" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'" }, "string" },
			  { { "-?0x%x+" }, "number" },
			  { { "-?%.?%d+[uUlLfFdDmM]?" }, "number" },
			  { { "(class|interface|struct)\\s+([A-Za-z]\\w*)" },
				{ "keyword", "keyword", "keyword2" },
				"",
				SyntaxPatternMatchType::RegEx },
			  { { "(class|interface|struct)\\s+([A-Za-z]\\w*)\\s*:\\s*([A-Za-z]\\w*)" },
				{ "keyword", "keyword", "keyword2", "keyword2" },
				"",
				SyntaxPatternMatchType::RegEx },
			  { { "-?%d+[%d%.eE]*f?" }, "number" },
			  { { "[%+%-/%*%<>!=%^&|?~:;%.%(%)%[%]{}]" }, "operator" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },
		  },
		  {
			  { "class", "keyword" },		{ "delegate", "keyword" },	{ "enum", "keyword" },
			  { "errordomain", "keyword" }, { "interface", "keyword" }, { "namespace", "keyword" },
			  { "signal", "keyword" },		{ "struct", "keyword" },	{ "using", "keyword" },

			  { "abstract", "keyword" },	{ "const", "keyword" },		{ "dynamic", "keyword" },
			  { "extern", "keyword" },		{ "inline", "keyword" },	{ "out", "keyword" },
			  { "override", "keyword" },	{ "private", "keyword" },	{ "protected", "keyword" },

			  { "public", "keyword" },		{ "ref", "keyword" },		{ "static", "keyword" },
			  { "virtual", "keyword" },		{ "volatile", "keyword" },	{ "weak", "keyword" },

			  { "as", "keyword" },			{ "base", "keyword" },		{ "break", "keyword" },
			  { "case", "keyword" },		{ "catch", "keyword" },		{ "construct", "keyword" },
			  { "continue", "keyword" },	{ "default", "keyword" },	{ "delete", "keyword" },
			  { "do", "keyword" },			{ "owned", "keyword" },		{ "yield", "keyword" },

			  { "else", "keyword" },		{ "ensures", "keyword" },	{ "finally", "keyword" },
			  { "for", "keyword" },			{ "foreach", "keyword" },	{ "get", "keyword" },
			  { "if", "keyword" },			{ "in", "keyword" },		{ "is", "keyword" },
			  { "lock", "keyword" },		{ "new", "keyword" },

			  { "requires", "keyword" },	{ "return", "keyword" },	{ "set", "keyword" },
			  { "sizeof", "keyword" },		{ "switch", "keyword" },	{ "this", "keyword" },
			  { "throw", "keyword" },		{ "throws", "keyword" },	{ "try", "keyword" },
			  { "typeof", "keyword" },

			  { "value", "keyword" },		{ "var", "keyword" },		{ "void", "keyword" },
			  { "while", "keyword" },		{ "async", "keyword" },		{ "internal", "keyword" },

			  { "null", "keyword" },		{ "true", "keyword" },		{ "false", "keyword" },

			  { "bool", "keyword2" },		{ "char", "keyword2" },		{ "double", "keyword2" },
			  { "float", "keyword2" },		{ "int", "keyword2" },		{ "int8", "keyword2" },
			  { "int16", "keyword2" },		{ "int32", "keyword2" },	{ "int64", "keyword2" },
			  { "long", "keyword2" },		{ "short", "keyword2" },

			  { "size_t", "keyword2" },		{ "ssize_t", "keyword2" },	{ "string", "keyword2" },
			  { "uchar", "keyword2" },		{ "uint", "keyword2" },		{ "uint8", "keyword2" },
			  { "uint16", "keyword2" },		{ "uint32", "keyword2" },	{ "uint64", "keyword2" },
			  { "ulong", "keyword2" },		{ "IOError", "keyword2" },	{ "Object", "keyword2" },

			  { "unichar", "keyword2" },	{ "ushort", "keyword2" },	{ "Error", "keyword2" },

		  },
		  "//",
		  {}

		} );

	sd.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );
}

}}}} // namespace EE::UI::Doc::Language
