#include <eepp/ui/doc/languages/teal.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addTeal() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "Teal",
		  { "%.tl$", "%.d%.tl$" },
		  {
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "%[%[", "%]%]" }, "string" },
			  { { "%-%-%[%[", "%]%]" }, "comment" },
			  { { "%-%-.-\n" }, "comment" },
			  { { "-?0x%x+" }, "number" },
			  { { "-?%d+[%d%.eE]*" }, "number" },
			  { { "-?%.?%d+" }, "number" },
			  { { "<%a+>" }, "keyword2" },
			  { { "%.%.%.?" }, "operator" },
			  { { "[<>~=]=" }, "operator" },
			  { { "[%+%-=/%*%^%%#<>]" }, "operator" },
			  { { "[%a_][%w_]*%s*%f[(\"{]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },
			  { { "::[%a_][%w_]*::" }, "function" },

		  },
		  {
			  { "true", "literal" },	 { "if", "keyword" },		{ "any", "keyword2" },
			  { "boolean", "keyword2" }, { "while", "keyword" },	{ "end", "keyword" },
			  { "else", "keyword" },	 { "number", "keyword2" },	{ "break", "keyword" },
			  { "not", "keyword" },		 { "local", "keyword" },	{ "elseif", "keyword" },
			  { "then", "keyword" },	 { "return", "keyword" },	{ "do", "keyword" },
			  { "until", "keyword" },	 { "function", "keyword" }, { "string", "keyword2" },
			  { "global", "keyword" },	 { "false", "literal" },	{ "repeat", "keyword" },
			  { "in", "keyword" },		 { "and", "keyword" },		{ "for", "keyword" },
			  { "or", "keyword" },		 { "goto", "keyword" },		{ "record", "keyword" },
			  { "nil", "literal" },		 { "enum", "keyword" },		{ "thread", "keyword2" },

		  },
		  "--",
		  { "^#!.*[ /]tl" }

		} );

	sd.setFoldRangeType( FoldRangeType::Indentation );
}

}}}} // namespace EE::UI::Doc::Language
