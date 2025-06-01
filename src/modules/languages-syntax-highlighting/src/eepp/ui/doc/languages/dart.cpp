#include <eepp/ui/doc/languages/dart.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addDart() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "Dart",
		  { "%.dart$" },
		  {
			  { { "//.-\n" }, "comment" },
			  { { "///.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "-?0x%x+" }, "number" },
			  { { "-?%d+[%d%.eE]*f?" }, "number" },
			  { { "-?%.?%d+f?" }, "number" },
			  { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
			  { { "%?%?" }, "operator" },
			  { { "%?%." }, "operator" },
			  { { "[%$%@]?\"", "\"", "\\" }, "string" },
			  { { "'\\x%x?%x?%x?%x'" }, "string" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "int", "type" },		{ "in", "keyword" },	  { "then", "keyword" },
			  { "dynamic", "type" },	{ "new", "keyword" },	  { "continue", "keyword" },
			  { "finally", "keyword" }, { "default", "keyword" }, { "switch", "keyword" },
			  { "double", "type" },		{ "bool", "type" },		  { "this", "type" },
			  { "return", "keyword" },	{ "var", "keyword" },	  { "part of", "keyword" },
			  { "String", "type" },		{ "class", "keyword" },	  { "void", "keyword" },
			  { "static", "keyword" },	{ "false", "literal" },	  { "for", "keyword" },
			  { "while", "keyword" },	{ "const", "keyword" },	  { "null", "literal" },
			  { "true", "literal" },	{ "else", "keyword" },	  { "final", "keyword" },
			  { "enum", "keyword" },	{ "print", "keyword" },	  { "Map", "type" },
			  { "break", "keyword" },	{ "List", "type" },		  { "Function", "type" },
			  { "case", "keyword" },	{ "if", "keyword" },	  { "await", "keyword" },
			  { "do", "keyword" },

		  },
		  "//",
		  {}

		} );

	sd.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );
	return sd;
}

}}}} // namespace EE::UI::Doc::Language
