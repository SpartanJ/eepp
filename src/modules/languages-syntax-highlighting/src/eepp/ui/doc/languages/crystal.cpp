#include <eepp/ui/doc/languages/crystal.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addCrystal() {

	SyntaxDefinitionManager::instance()->add(

		{ "Crystal",
		  { "%.cr$" },
		  {
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'" }, "string" },
			  { { "-?0x%x+" }, "number" },
			  { { "#.-\n" }, "comment" },
			  { { " : [%w_| :]*" }, "comment" },
			  { { "-?%d+[%d%.eE]*%f[^eE]" }, "number" },
			  { { "-?%.?%d+f?" }, "number" },
			  { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
			  { { "[%a_][%w_]*%f[(?]" }, "function" },
			  { { "%.[%a_][%w_]*%f[(%s]" }, "function" },
			  { { "@?@[%a_][%w_]*" }, "keyword2" },
			  { { ":-[%u_][%u%d_]*%f[^%w]" }, "keyword2" },
			  { { "::[%w_]*" }, "symbol" },
			  { { ":[%w_]*" }, "symbol" },
			  { { "[%a_][%w_]*:[^:]" }, "keyword2" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "end", "keyword" },
			  { "END", "keyword" },
			  { "else", "keyword" },
			  { "case", "keyword" },
			  { "BEGIN", "keyword" },
			  { "__FILE__", "keyword" },
			  { "annotation", "keyword2" },
			  { "__LINE__", "keyword" },
			  { "until", "keyword" },
			  { "abstract", "keyword" },
			  { "Void", "literal" },
			  { "fun", "keyword" },
			  { "alias", "keyword" },
			  { "self", "keyword2" },
			  { "break", "keyword" },
			  { "class", "keyword" },
			  { "yield", "keyword" },
			  { "then", "keyword" },
			  { "unless", "keyword" },
			  { "include", "keyword" },
			  { "when", "keyword" },
			  { "extend", "keyword" },
			  { "next", "keyword" },
			  { "enum", "keyword2" },
			  { "nil", "literal" },
			  { "begin", "keyword" },
			  { "if", "keyword" },
			  { "out", "function" },
			  { "false", "literal" },
			  { "in", "keyword" },
			  { "def", "keyword" },
			  { "elsif", "keyword" },
			  { "responds_to?", "function" },
			  { "module", "keyword" },
			  { "ensure", "keyword" },
			  { "while", "keyword" },
			  { "do", "keyword" },
			  { "require", "keyword" },
			  { "defined?", "keyword" },
			  { "private", "keyword2" },
			  { "rescue", "keyword" },
			  { "p!", "function" },
			  { "puts", "function" },
			  { "true", "literal" },
			  { "__ENCODING__", "keyword" },
			  { "return", "keyword" },
			  { "super", "keyword2" },
			  { "undef", "keyword" },

		  },
		  "#",
		  {}

		} );
}

}}}} // namespace EE::UI::Doc::Language
