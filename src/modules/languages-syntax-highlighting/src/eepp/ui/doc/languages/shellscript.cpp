#include <eepp/ui/doc/languages/shellscript.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addShellScript() {

	return SyntaxDefinitionManager::instance()
		->add(

			{ "Shell script",
			  { "%.sh$", "%.bash$", "^%.bashrc$", "^%.bash_profile$", "^%.profile$", "%.zsh$",
				"%.fish$", "^PKGBUILD$", "%.winlib$", "^APKBUILD$" },
			  {
				  { { "$[%a_@*#][%w_]*" }, "type" },
				  { { "#.*\n" }, "comment" },
				  { { "<<%-?%s*EOF", "EOF" }, "string" },
				  { { "\"", "\"", "\\" }, "string" },
				  { { "'", "'", "\\" }, "string" },
				  { { "`", "`", "\\" }, "string" },
				  { { "%f[%w_%.%/]%d[%d%.]*%f[^%w_%.]" }, "number" },
				  { { "[!<>|&%[%]:=*]" }, "operator" },
				  { { "%f[%S][%+%-][%w%-_:]+" }, "function" },
				  { { "%f[%S][%+%-][%w%-_]+%f[=]" }, "function" },
				  { { "(%s%-%a[%w_%-]*%s+)(%d[%d%.]+)" }, { "normal", "function", "number" } },
				  { { "(%s%-%a[%w_%-]*%s+)(%a[%a%-_:=]+)" }, { "normal", "function", "symbol" } },
				  { { "[_%a][%w_]+%f[%+=]" }, "type" },
				  { { "${.-}" }, "type" },
				  { { "$[%d$%a_@*][%w_]*" }, "type" },
				  { { "[%a_%-][%w_%-]*[%s]*%f[(]" }, "function" },
				  { { "[%a_][%w_]*" }, "symbol" },

			  },
			  {
				  { "in", "keyword" },		{ "then", "keyword" },		{ "exit", "keyword" },
				  { "alias", "keyword" },	{ "continue", "keyword" },	{ "getopts", "keyword" },
				  { "set", "keyword" },		{ "return", "keyword" },	{ "unalias", "keyword" },
				  { "pwd", "keyword" },		{ "fi", "keyword" },		{ "printf", "keyword" },
				  { "unset", "keyword" },	{ "cd", "keyword" },		{ "echo", "keyword" },
				  { "false", "literal" },	{ "help", "keyword" },		{ "for", "keyword" },
				  { "test", "keyword" },	{ "mapfile", "keyword" },	{ "shift", "keyword" },
				  { "while", "keyword" },	{ "readarray", "keyword" }, { "eval", "keyword" },
				  { "select", "keyword" },	{ "elif", "keyword" },		{ "function", "keyword" },
				  { "true", "literal" },	{ "else", "keyword" },		{ "exec", "keyword" },
				  { "enable", "keyword" },	{ "local", "keyword" },		{ "jobs", "keyword" },
				  { "source", "keyword" },	{ "break", "keyword" },		{ "declare", "keyword" },
				  { "history", "keyword" }, { "case", "keyword" },		{ "until", "keyword" },
				  { "if", "keyword" },		{ "esac", "keyword" },		{ "hash", "keyword" },
				  { "kill", "keyword" },	{ "time", "keyword" },		{ "let", "keyword" },
				  { "export", "keyword" },	{ "do", "keyword" },		{ "done", "keyword" },
				  { "read", "keyword" },	{ "type", "keyword" },

			  },
			  "#",
			  { "^#!.*[ /]bash", "^#!.*[ /]sh", "^#!.*[ /]zsh", "^#!.*[ /]fish" },
			  "shellscript" } )
		.addAlternativeName( "bash" )
		.addAlternativeName( "fish" )
		.addAlternativeName( "zsh" )
		.addAlternativeName( "sh" );
}

}}}} // namespace EE::UI::Doc::Language
