#include <eepp/ui/doc/languages/lua.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addLua() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "Lua",
		  { "%.lua$", "%.rockspec$" },
		  {
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "%[%[", "%]%]" }, "string" },
			  { { "%-%-%[%[", "%]%]" }, "comment" },
			  { { "%-%-.-\n" }, "comment" },
			  { { "-?0x%x+" }, "number" },
			  { { "-?%d+[%d%.eE]*" }, "number" },
			  { { "-?%.?%d+" }, "number" },
			  { { "%.%.%.?" }, "operator" },
			  { { "[<>~=]=" }, "operator" },
			  { { "[%+%-=/%*%^%%#<>]" }, "operator" },
			  { { "[%a_][%w_]*%s*%f[(\"{]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },
			  { { "::[%a_][%w_]*::" }, "function" },

		  },
		  {
			  { "elseif", "keyword" }, { "false", "literal" }, { "in", "keyword" },
			  { "then", "keyword" },   { "for", "keyword" },   { "goto", "keyword" },
			  { "while", "keyword" },  { "and", "keyword" },   { "function", "keyword" },
			  { "self", "keyword2" },  { "true", "literal" },  { "else", "keyword" },
			  { "nil", "literal" },	   { "not", "keyword" },   { "return", "keyword" },
			  { "local", "keyword" },  { "break", "keyword" }, { "or", "keyword" },
			  { "end", "keyword" },	   { "until", "keyword" }, { "if", "keyword" },
			  { "repeat", "keyword" }, { "do", "keyword" },

		  },
		  "--",
		  { "^#!.*[ /]lua" }

		} );

	sd.setFoldRangeType( FoldRangeType::Indentation );
}

}}}} // namespace EE::UI::Doc::Language
