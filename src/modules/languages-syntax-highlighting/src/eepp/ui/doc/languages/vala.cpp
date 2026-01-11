#include <eepp/ui/doc/languages/vala.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addVala() {

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
				{ "keyword", "keyword", "type" },
				"",
				SyntaxPatternMatchType::RegEx },
			  { { "(class|interface|struct)\\s+([A-Za-z]\\w*)\\s*:\\s*([A-Za-z]\\w*)" },
				{ "keyword", "keyword", "type", "type" },
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

			  { "bool", "type" },			{ "char", "type" },			{ "double", "type" },
			  { "float", "type" },			{ "int", "type" },			{ "int8", "type" },
			  { "int16", "type" },			{ "int32", "type" },		{ "int64", "type" },
			  { "long", "type" },			{ "short", "type" },

			  { "size_t", "type" },			{ "ssize_t", "type" },		{ "string", "type" },
			  { "uchar", "type" },			{ "uint", "type" },			{ "uint8", "type" },
			  { "uint16", "type" },			{ "uint32", "type" },		{ "uint64", "type" },
			  { "ulong", "type" },			{ "IOError", "type" },		{ "Object", "type" },

			  { "unichar", "type" },		{ "ushort", "type" },		{ "Error", "type" },

		  },
		  "//",
		  {}

		} );

	sd.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );
	sd.setBlockComment( { "/*", "*/" } );
	return sd;
}

}}}} // namespace EE::UI::Doc::Language
