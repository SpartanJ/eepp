#include <eepp/ui/doc/languages/squirrel.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addSquirrel() {

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
				{ "keyword", "keyword", "keyword2", "keyword", "keyword2" } },
			  { { "(class%s+)([%a_][%w_]*)" }, { "keyword", "keyword", "keyword2" } },
			  { { "(enum%s+)([%a_][%w_]*)" }, { "keyword", "keyword", "keyword2" } },
			  { { "(function%s+)([%a_][%w_]*)(%:%:)([%a_][%w_]*)%f[(]" },
				{ "keyword", "keyword", "keyword2", "operator", "function" } },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "_[A-Z]%w*" }, "keyword" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "vargc", "keyword2" },		{ "in", "keyword" },	   { "resume", "keyword" },
			  { "continue", "keyword" },	{ "default", "keyword" },  { "switch", "keyword" },
			  { "this", "keyword" },		{ "delete", "keyword" },   { "return", "keyword" },
			  { "try", "keyword" },			{ "foreach", "keyword" },  { "class", "keyword" },
			  { "yield", "keyword" },		{ "typeof", "keyword" },   { "extends", "keyword" },
			  { "false", "literal" },		{ "vargv", "keyword2" },   { "for", "keyword" },
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
}

}}}} // namespace EE::UI::Doc::Language
