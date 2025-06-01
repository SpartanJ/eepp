#include <eepp/ui/doc/languages/squirrel.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addSquirrel() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "Squirrel",
		  { "%.nut$" },
		  {
			  { { "//.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "@\"", "\"", "\\" }, "string" },
			  { { "@'", "'", "\\" }, "string" },
			  { { "\"", "[\"\n]", "\\" }, "string" },
			  { { "'", "['\n]", "\\" }, "string" },
			  { { "common_number_parser_o" }, "number", "", SyntaxPatternMatchType::Parser },
			  { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
			  { { "(class%s+)([%a_][%w_]*%s+)(extends%s+)([%a_][%w_]*)" },
				{ "keyword", "keyword", "type", "keyword", "type" } },
			  { { "(class%s+)([%a_][%w_]*)" }, { "keyword", "keyword", "type" } },
			  { { "(enum%s+)([%a_][%w_]*)" }, { "keyword", "keyword", "type" } },
			  { { "(function%s+)([%a_][%w_]*)(%:%:)([%a_][%w_]*)%f[(]" },
				{ "keyword", "keyword", "type", "operator", "function" } },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "_[A-Z]%w*" }, "keyword" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "vargc", "type" },			{ "in", "keyword" },	   { "resume", "keyword" },
			  { "continue", "keyword" },	{ "default", "keyword" },  { "switch", "keyword" },
			  { "this", "keyword" },		{ "delete", "keyword" },   { "return", "keyword" },
			  { "try", "keyword" },			{ "foreach", "keyword" },  { "class", "keyword" },
			  { "yield", "keyword" },		{ "typeof", "keyword" },   { "extends", "keyword" },
			  { "false", "literal" },		{ "vargv", "type" },	   { "for", "keyword" },
			  { "while", "keyword" },		{ "const", "keyword" },	   { "delegate", "keyword" },
			  { "null", "literal" },		{ "function", "keyword" }, { "throw", "keyword" },
			  { "catch", "keyword" },		{ "true", "literal" },	   { "else", "keyword" },
			  { "constructor", "keyword" }, { "enum", "keyword" },	   { "instanceof", "keyword" },
			  { "local", "keyword" },		{ "break", "keyword" },	   { "base", "keyword" },
			  { "case", "keyword" },		{ "if", "keyword" },	   { "clone", "keyword" },
			  { "do", "keyword" },

		  },
		  "//",
		  {}

		} );

	sd.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );
	return sd;
}

}}}} // namespace EE::UI::Doc::Language
