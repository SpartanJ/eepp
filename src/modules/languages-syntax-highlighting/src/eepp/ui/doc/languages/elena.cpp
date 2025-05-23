#include <eepp/ui/doc/languages/elena.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addElena() {

	SyntaxDefinitionManager::instance()
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
					{ "normal", "keyword", "keyword2", "operator", "keyword2" } },
				  { { "(class%s+)([%a_][%w_]*)" }, { "normal", "keyword", "keyword2" } },
				  { { "(singleton%s+)([%a_][%w_]*)" }, { "normal", "keyword", "keyword2" } },
				  { { "(extension%s+)([%a_][%w_]*)" }, { "normal", "keyword", "keyword2" } },
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
				  { "internal", "keyword" },	{ "int", "keyword2" },
				  { "new", "keyword" },			{ "continue", "keyword" },
				  { "default", "keyword" },		{ "remove", "keyword" },
				  { "sizeof", "keyword" },		{ "char", "keyword2" },
				  { "set", "keyword" },			{ "bool", "keyword2" },
				  { "extern", "keyword" },		{ "long", "keyword2" },
				  { "foreach", "keyword" },		{ "namespace", "keyword" },
				  { "partial", "keyword" },		{ "short", "keyword2" },
				  { "typeof", "keyword" },		{ "object", "keyword2" },
				  { "false", "literal" },		{ "for", "keyword" },
				  { "goto", "keyword" },		{ "while", "keyword" },
				  { "const", "keyword" },		{ "get", "keyword" },
				  { "delegate", "keyword" },	{ "add", "keyword" },
				  { "null", "literal" },		{ "unsafe", "keyword" },
				  { "fixed", "keyword" },		{ "else", "keyword" },
				  { "stackalloc", "keyword" },	{ "global", "keyword" },
				  { "record", "keyword" },		{ "enum", "keyword" },
				  { "value", "keyword" },		{ "lock", "keyword" },
				  { "operator", "keyword" },	{ "when", "keyword" },
				  { "struct", "keyword" },		{ "break", "keyword" },
				  { "is", "keyword" },			{ "decimal", "keyword2" },
				  { "string", "keyword2" },		{ "if", "keyword" },
				  { "await", "keyword" },		{ "in", "keyword" },
				  { "public", "keyword" },		{ "virtual", "keyword" },
				  { "dynamic", "keyword" },		{ "private", "keyword" },
				  { "finally", "keyword" },		{ "abstract", "keyword" },
				  { "unchecked", "keyword" },	{ "volatile", "keyword" },
				  { "switch", "keyword" },		{ "using", "keyword" },
				  { "checked", "keyword" },		{ "double", "keyword2" },
				  { "event", "keyword" },		{ "auto", "keyword" },
				  { "ulong", "keyword2" },		{ "var", "keyword" },
				  { "out", "keyword" },			{ "try", "keyword" },
				  { "as", "keyword" },			{ "class", "keyword" },
				  { "byte", "keyword2" },		{ "yield", "keyword" },
				  { "static", "keyword" },		{ "super", "keyword" },
				  { "uint", "keyword2" },		{ "explicit", "keyword" },
				  { "sbyte", "keyword2" },		{ "throw", "keyword" },
				  { "self", "keyword" },		{ "catch", "keyword" },
				  { "true", "literal" },		{ "ushort", "keyword2" },
				  { "override", "keyword" },	{ "sealed", "keyword" },
				  { "float", "keyword2" },		{ "params", "keyword" },
				  { "readonly", "keyword" },	{ "interface", "keyword" },
				  { "protected", "keyword" },	{ "do", "keyword" },
				  { "implicit", "keyword" },	{ "ref", "keyword" },
				  { "constructor", "keyword" }, { "singleton", "keyword" },
				  { "real", "keyword2" },		{ "ifnot", "keyword" },
				  { "symbol", "keyword" },

			  },
			  "//",
			  {}

			} )
		.setFoldRangeType( FoldRangeType::Braces )
		.setFoldBraces( { { '{', '}' } } );
	;
}

}}}} // namespace EE::UI::Doc::Language
