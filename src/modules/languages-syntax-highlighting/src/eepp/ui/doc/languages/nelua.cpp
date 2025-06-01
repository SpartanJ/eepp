#include <eepp/ui/doc/languages/nelua.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addNelua() {

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
			  { { "_[iunbf][s1368][246i]?[8z]?[e]?" }, "type" },
			  { { "[%+%-=/%*%^%%#<>]" }, "operator" },
			  { { "([%a_][%w_]*)(%s*%f[(\"'{])" }, { "function", "function", "normal" } },
			  { { "(@)(enum)" }, { "normal", "keyword", "type" } },
			  { { "(@)(record)" }, { "normal", "keyword", "type" } },
			  { { "(@)(union)" }, { "normal", "keyword", "type" } },
			  { { "%((@)(%w+)%)" }, { "normal", "keyword", "type" } },
			  { { "[%a_][%w_]*" }, "symbol" },
			  { { "::[%a_][%w_]*::" }, "function" },

		  },
		  {
			  { "isize", "type" },		 { "break", "keyword" },	{ "else", "keyword" },
			  { "in", "keyword" },		 { "number", "type" },		{ "byte", "type" },
			  { "boolean", "type" },	 { "culonglong", "type" },	{ "false", "literal" },
			  { "or", "keyword" },		 { "culong", "type" },		{ "clongdouble", "type" },
			  { "clonglong", "type" },	 { "cshort", "type" },		{ "cemit", "type" },
			  { "global", "keyword" },	 { "cemitdecl", "type" },	{ "nodecl", "type" },
			  { "integer", "type" },	 { "cuchar", "type" },		{ "cint", "type" },
			  { "cimport", "type" },	 { "for", "keyword" },		{ "cemitdefn", "type" },
			  { "defer", "keyword" },	 { "codename", "type" },	{ "int8", "type" },
			  { "goto", "keyword" },	 { "true", "literal" },		{ "usize", "type" },
			  { "niltype", "type" },	 { "uint8", "type" },		{ "inline", "literal" },
			  { "while", "keyword" },	 { "uint32", "type" },		{ "auto", "type" },
			  { "then", "keyword" },	 { "self", "type" },		{ "void", "type" },
			  { "function", "keyword" }, { "string", "type" },		{ "float128", "type" },
			  { "and", "keyword" },		 { "case", "keyword" },		{ "float32", "type" },
			  { "float64", "type" },	 { "int64", "type" },		{ "uint16", "type" },
			  { "noinit", "literal" },	 { "volatile", "literal" }, { "int16", "type" },
			  { "comptime", "literal" }, { "uint128", "type" },		{ "uint64", "type" },
			  { "int32", "type" },		 { "repeat", "keyword" },	{ "continue", "keyword" },
			  { "not", "keyword" },		 { "cexport", "type" },		{ "cptrdiff", "type" },
			  { "nil", "literal" },		 { "cstring", "type" },		{ "end", "keyword" },
			  { "nilptr", "keyword" },	 { "clong", "type" },		{ "switch", "keyword" },
			  { "cchar", "type" },		 { "cuint", "type" },		{ "pointer", "type" },
			  { "int128", "type" },		 { "return", "keyword" },	{ "elseif", "keyword" },
			  { "uinteger", "type" },	 { "csize", "type" },		{ "if", "keyword" },
			  { "const", "literal" },	 { "local", "keyword" },	{ "cushort", "type" },
			  { "do", "keyword" },		 { "until", "keyword" },	{ "cschar", "type" },
			  { "cinclude", "type" },

		  },
		  "--",
		  { "^#!.*[ /]nelua" }

		} );

	sd.setFoldRangeType( FoldRangeType::Indentation );
	return sd;
}

}}}} // namespace EE::UI::Doc::Language
