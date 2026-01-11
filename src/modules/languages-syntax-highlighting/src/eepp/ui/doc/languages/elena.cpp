#include <eepp/ui/doc/languages/elena.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addElena() {

	return SyntaxDefinitionManager::instance()
		->add(

			{ "ELENA",
			  { "%.l$" },
			  {
				  { { "//.-\n" }, "comment" },
				  { { "/%*", "%*/" }, "comment" },
				  { { "\"", "\"", "\\" }, "string" },
				  { { "[%$%@]?\"", "\"", "\\" }, "string" },
				  { { "$x%x?%x?%x?%x" }, "string" },
				  { { "^%s*(import)%s+([%a_][%w_']*)%s*%f[%;]" },
					{ "normal", "keyword", "literal" } },
				  { { "(class%s+)([%a_][%w_]*)%s*(:)%s*([%a_][%w_]*)" },
					{ "normal", "keyword", "type", "operator", "type" } },
				  { { "(class%s+)([%a_][%w_]*)" }, { "normal", "keyword", "type" } },
				  { { "(singleton%s+)([%a_][%w_]*)" }, { "normal", "keyword", "type" } },
				  { { "(extension%s+)([%a_][%w_]*)" }, { "normal", "keyword", "type" } },
				  { { "-?0x%x+" }, "number" },
				  { { "-?%d+[%d%.eE]*f?" }, "number" },
				  { { "-?%.?%d+f?" }, "number" },
				  { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
				  { { "%?%?" }, "operator" },
				  { { "%?%." }, "operator" },
				  { { "[%a_][%w_]*%f[(]" }, "function" },
				  { { "[%a_][%w_]*" }, "symbol" },

			  },
			  {

				  { "internal", "keyword" },	{ "int", "type" },
				  { "new", "keyword" },			{ "continue", "keyword" },
				  { "default", "keyword" },		{ "remove", "keyword" },
				  { "sizeof", "keyword" },		{ "char", "type" },
				  { "set", "keyword" },			{ "bool", "type" },
				  { "extern", "keyword" },		{ "long", "type" },
				  { "foreach", "keyword" },		{ "namespace", "keyword" },
				  { "partial", "keyword" },		{ "short", "type" },
				  { "typeof", "keyword" },		{ "object", "type" },
				  { "false", "literal" },		{ "for", "keyword" },
				  { "goto", "keyword" },		{ "while", "keyword" },
				  { "const", "keyword" },		{ "get", "keyword" },
				  { "delegate", "keyword" },	{ "add", "keyword" },
				  { "nil", "literal" },			{ "unsafe", "keyword" },
				  { "fixed", "keyword" },		{ "else", "keyword" },
				  { "stackalloc", "keyword" },	{ "global", "keyword" },
				  { "record", "keyword" },		{ "enum", "keyword" },
				  { "value", "keyword" },		{ "lock", "keyword" },
				  { "operator", "keyword" },	{ "when", "keyword" },
				  { "struct", "keyword" },		{ "break", "keyword" },
				  { "is", "keyword" },			{ "decimal", "type" },
				  { "string", "type" },			{ "if", "keyword" },
				  { "await", "keyword" },		{ "in", "keyword" },
				  { "public", "keyword" },		{ "virtual", "keyword" },
				  { "dynamic", "keyword" },		{ "private", "keyword" },
				  { "finally", "keyword" },		{ "abstract", "keyword" },
				  { "unchecked", "keyword" },	{ "volatile", "keyword" },
				  { "switch", "keyword" },		{ "using", "keyword" },
				  { "checked", "keyword" },		{ "double", "type" },
				  { "event", "keyword" },		{ "auto", "keyword" },
				  { "ulong", "type" },			{ "var", "keyword" },
				  { "out", "keyword" },			{ "try", "keyword" },
				  { "as", "keyword" },			{ "class", "keyword" },
				  { "byte", "type" },			{ "yield", "keyword" },
				  { "static", "keyword" },		{ "super", "keyword" },
				  { "uint", "type" },			{ "explicit", "keyword" },
				  { "sbyte", "type" },			{ "throw", "keyword" },
				  { "self", "keyword" },		{ "catch", "keyword" },
				  { "true", "literal" },		{ "ushort", "type" },
				  { "override", "keyword" },	{ "sealed", "keyword" },
				  { "float", "type" },			{ "params", "keyword" },
				  { "readonly", "keyword" },	{ "interface", "keyword" },
				  { "protected", "keyword" },	{ "do", "keyword" },
				  { "implicit", "keyword" },	{ "ref", "keyword" },
				  { "constructor", "keyword" }, { "singleton", "keyword" },
				  { "real", "type" },			{ "ifnot", "keyword" },
				  { "symbol", "keyword" },		{ "use", "keyword" },
				  { "pointer", "type" },		{ "this", "keyword" },

			  },
			  "//",
			  {}

			} )
		.setFoldRangeType( FoldRangeType::Braces )
		.setFoldBraces( { { '{', '}' } } )
		.setBlockComment( { "/*", "*/" } );
	;
}

}}}} // namespace EE::UI::Doc::Language
