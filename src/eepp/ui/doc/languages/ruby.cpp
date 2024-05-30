#include <eepp/ui/doc/languages/ruby.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addRuby() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "Ruby",
		  { "%.rb", "%.gemspec", "%.ruby" },
		  {
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "-?0x%x+" }, "number" },
			  { { "%#.-\n" }, "comment" },
			  { { "-?%d+[%d%.eE]*f?" }, "number" },
			  { { "-?%.?%d+f?" }, "number" },
			  { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "@?@[%a_][%w_]*" }, "keyword2" },
			  { { "::[%w_]*" }, "symbol" },
			  { { ":[%w_]*" }, "keyword2" },
			  { { "[%a_][%w_]*:[^:]" }, "keyword2" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "__FILE__", "keyword" },
			  { "in", "keyword" },
			  { "then", "keyword" },
			  { "alias", "keyword" },
			  { "and", "keyword" },
			  { "private", "keyword" },
			  { "require_dependency", "keyword" },
			  { "include", "keyword" },
			  { "__LINE__", "keyword" },
			  { "unless", "keyword" },
			  { "begin", "keyword" },
			  { "extend", "keyword" },
			  { "return", "keyword" },
			  { "BEGIN", "keyword" },
			  { "class", "keyword" },
			  { "__ENCODING__", "keyword" },
			  { "END", "keyword" },
			  { "end", "literal" },
			  { "undef", "keyword" },
			  { "yield", "keyword" },
			  { "super", "keyword" },
			  { "module", "keyword" },
			  { "elsif", "keyword" },
			  { "false", "literal" },
			  { "for", "keyword" },
			  { "while", "keyword" },
			  { "def", "keyword" },
			  { "self", "keyword" },
			  { "true", "literal" },
			  { "else", "keyword" },
			  { "next", "keyword" },
			  { "rescue", "keyword" },
			  { "nil", "literal" },
			  { "not", "keyword" },
			  { "require", "keyword" },
			  { "when", "keyword" },
			  { "retry", "keyword" },
			  { "break", "keyword" },
			  { "defined?", "keyword" },
			  { "or", "keyword" },
			  { "case", "keyword" },
			  { "until", "keyword" },
			  { "if", "keyword" },
			  { "ensure", "keyword" },
			  { "do", "keyword" },
			  { "redo", "keyword" },

		  },
		  "#",
		  { "^#!.*[ /]ruby" }

		} );

	sd.setFoldRangeType( FoldRangeType::Indentation );
}

}}}} // namespace EE::UI::Doc::Language
