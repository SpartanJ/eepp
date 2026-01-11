#include <eepp/ui/doc/languages/x86assembly.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addPICO8() {

	return SyntaxDefinitionManager::instance()->add(

		{ "PICO-8",
		  { "%.p8$" },
		  {
			  { { "pico%-8 cartridge", "__lua__" }, "comment" },
			  { { "__gfx__\n", "%z" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "%[%[", "%]%]" }, "string" },
			  { { "%-%-%[%[", "%]%]" }, "comment" },
			  { { "%-%-.-\n" }, "comment" },
			  { { "-?0x%x+" }, "number" },
			  { { "-?%d+[%d%.eE]*" }, "number" },
			  { { "-?%.?%d+" }, "number" },
			  { { "%.%.%.?" }, "operator" },
			  { { "[<>~=&|]=" }, "operator" },
			  { { "[%+%-=/%*%^%%#<>]" }, "operator" },
			  { { "[%a_][%w_]*%s*%f[(\"{]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },
			  { { "::[%a_][%w_]*::" }, "function" },

		  },
		  {
			  { "true", "literal" },   { "self", "type" },	  { "return", "keyword" },
			  { "repeat", "keyword" }, { "or", "keyword" },	  { "and", "keyword" },
			  { "break", "keyword" },  { "else", "keyword" }, { "until", "keyword" },
			  { "do", "keyword" },	   { "in", "keyword" },	  { "while", "keyword" },
			  { "end", "keyword" },	   { "nil", "literal" },  { "elseif", "keyword" },
			  { "false", "literal" },  { "not", "keyword" },  { "local", "keyword" },
			  { "for", "keyword" },	   { "then", "keyword" }, { "function", "keyword" },
			  { "goto", "keyword" },   { "if", "keyword" },

		  },
		  "--",
		  {}

		} );
}

}}}} // namespace EE::UI::Doc::Language
