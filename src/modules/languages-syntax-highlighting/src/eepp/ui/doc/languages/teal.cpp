#include <eepp/ui/doc/languages/teal.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addTeal() {

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
			  { { "<%a+>" }, "type" },
			  { { "%.%.%.?" }, "operator" },
			  { { "[<>~=]=" }, "operator" },
			  { { "[%+%-=/%*%^%%#<>]" }, "operator" },
			  { { "[%a_][%w_]*%s*%f[(\"{]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },
			  { { "::[%a_][%w_]*::" }, "function" },

		  },
		  {
			  { "true", "literal" },   { "if", "keyword" },		  { "any", "type" },
			  { "boolean", "type" },   { "while", "keyword" },	  { "end", "keyword" },
			  { "else", "keyword" },   { "number", "type" },	  { "break", "keyword" },
			  { "not", "keyword" },	   { "local", "keyword" },	  { "elseif", "keyword" },
			  { "then", "keyword" },   { "return", "keyword" },	  { "do", "keyword" },
			  { "until", "keyword" },  { "function", "keyword" }, { "string", "type" },
			  { "global", "keyword" }, { "false", "literal" },	  { "repeat", "keyword" },
			  { "in", "keyword" },	   { "and", "keyword" },	  { "for", "keyword" },
			  { "or", "keyword" },	   { "goto", "keyword" },	  { "record", "keyword" },
			  { "nil", "literal" },	   { "enum", "keyword" },	  { "thread", "type" },

		  },
		  "--",
		  { "^#!.*[ /]tl" }

		} );

	sd.setFoldRangeType( FoldRangeType::Indentation );
	return sd;
}

}}}} // namespace EE::UI::Doc::Language
