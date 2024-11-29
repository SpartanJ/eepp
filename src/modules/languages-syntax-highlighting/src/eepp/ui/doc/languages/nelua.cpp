#include <eepp/ui/doc/languages/nelua.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addNelua() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "Nelua",
		  { "%.nelua$" },
		  {
			  { { "##%[=*%[", "%]=*%]" }, "function", "Lua" },
			  { { "#|", "|#" }, "function", "Lua" },
			  { { "##", "\n" }, "function", "Lua" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "%[%[", "%]%]" }, "string" },
			  { { "%-%-%[%[", "%]%]" }, "comment" },
			  { { "%-%-%[=+%[", "%]=+%]" }, "comment" },
			  { { "%-%-.-\n" }, "comment" },
			  { { "0x%x+%.%x*[pP][-+]?%d+" }, "number" },
			  { { "0x%x+%.%x*" }, "number" },
			  { { "0x%.%x+[pP][-+]?%d+" }, "number" },
			  { { "0x%.%x+" }, "number" },
			  { { "0x%x+[pP][-+]?%d+" }, "number" },
			  { { "0x%x+" }, "number" },
			  { { "0b[01_]+" }, "number" },
			  { { "%d%.%d*[eE][-+]?%d+" }, "number" },
			  { { "%d%.%d*" }, "number" },
			  { { "%.?%d*[eE][-+]?%d+" }, "number" },
			  { { "%.?%d+" }, "number" },
			  { { "%.%.%.?" }, "operator" },
			  { { "[<>~=]=" }, "operator" },
			  { { "_[iunbf][s1368][246i]?[8z]?[e]?" }, "keyword2" },
			  { { "[%+%-=/%*%^%%#<>]" }, "operator" },
			  { { "([%a_][%w_]*)(%s*%f[(\"'{])" }, { "function", "function", "normal" } },
			  { { "(@)(enum)" }, { "normal", "keyword", "keyword2" } },
			  { { "(@)(record)" }, { "normal", "keyword", "keyword2" } },
			  { { "(@)(union)" }, { "normal", "keyword", "keyword2" } },
			  { { "%((@)(%w+)%)" }, { "normal", "keyword", "keyword2" } },
			  { { "[%a_][%w_]*" }, "symbol" },
			  { { "::[%a_][%w_]*::" }, "function" },

		  },
		  {
			  { "isize", "keyword2" },	   { "break", "keyword" },
			  { "else", "keyword" },	   { "in", "keyword" },
			  { "number", "keyword2" },	   { "byte", "keyword2" },
			  { "boolean", "keyword2" },   { "culonglong", "keyword2" },
			  { "false", "literal" },	   { "or", "keyword" },
			  { "culong", "keyword2" },	   { "clongdouble", "keyword2" },
			  { "clonglong", "keyword2" }, { "cshort", "keyword2" },
			  { "cemit", "keyword2" },	   { "global", "keyword" },
			  { "cemitdecl", "keyword2" }, { "nodecl", "keyword2" },
			  { "integer", "keyword2" },   { "cuchar", "keyword2" },
			  { "cint", "keyword2" },	   { "cimport", "keyword2" },
			  { "for", "keyword" },		   { "cemitdefn", "keyword2" },
			  { "defer", "keyword" },	   { "codename", "keyword2" },
			  { "int8", "keyword2" },	   { "goto", "keyword" },
			  { "true", "literal" },	   { "usize", "keyword2" },
			  { "niltype", "keyword2" },   { "uint8", "keyword2" },
			  { "inline", "literal" },	   { "while", "keyword" },
			  { "uint32", "keyword2" },	   { "auto", "keyword2" },
			  { "then", "keyword" },	   { "self", "keyword2" },
			  { "void", "keyword2" },	   { "function", "keyword" },
			  { "string", "keyword2" },	   { "float128", "keyword2" },
			  { "and", "keyword" },		   { "case", "keyword" },
			  { "float32", "keyword2" },   { "float64", "keyword2" },
			  { "int64", "keyword2" },	   { "uint16", "keyword2" },
			  { "noinit", "literal" },	   { "volatile", "literal" },
			  { "int16", "keyword2" },	   { "comptime", "literal" },
			  { "uint128", "keyword2" },   { "uint64", "keyword2" },
			  { "int32", "keyword2" },	   { "repeat", "keyword" },
			  { "continue", "keyword" },   { "not", "keyword" },
			  { "cexport", "keyword2" },   { "cptrdiff", "keyword2" },
			  { "nil", "literal" },		   { "cstring", "keyword2" },
			  { "end", "keyword" },		   { "nilptr", "keyword" },
			  { "clong", "keyword2" },	   { "switch", "keyword" },
			  { "cchar", "keyword2" },	   { "cuint", "keyword2" },
			  { "pointer", "keyword2" },   { "int128", "keyword2" },
			  { "return", "keyword" },	   { "elseif", "keyword" },
			  { "uinteger", "keyword2" },  { "csize", "keyword2" },
			  { "if", "keyword" },		   { "const", "literal" },
			  { "local", "keyword" },	   { "cushort", "keyword2" },
			  { "do", "keyword" },		   { "until", "keyword" },
			  { "cschar", "keyword2" },	   { "cinclude", "keyword2" },

		  },
		  "--",
		  { "^#!.*[ /]nelua" }

		} );

	sd.setFoldRangeType( FoldRangeType::Indentation );
}

}}}} // namespace EE::UI::Doc::Language
