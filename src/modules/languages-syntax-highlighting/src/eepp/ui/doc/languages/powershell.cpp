#include <eepp/ui/doc/languages/powershell.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addPowerShell() {

	return SyntaxDefinitionManager::instance()->add(

		{ "PowerShell",
		  { "%.ps1$", "%.psm1$", "%.psd1$", "%.ps1xml$", "%.pssc$", "%.psrc$", "%.cdxml$" },
		  {
			  { { "#.*\n" }, "comment" },
			  { { "[[\\.]]" }, "normal" },
			  { { "\"", "\"" }, "string" },
			  { { "'", "'" }, "string" },
			  { { "%f[%w_][%d%.]+%f[^%w_]" }, "number" },
			  { { "[%+=/%*%^%%<>!~|&,:]+" }, "operator" },
			  { { "%f[%S]%-[%w%-%_]+" }, "function" },
			  { { "[%u][%a]+[%-][%u][%a]+" }, "function" },
			  { { "${.*}" }, "symbol" },
			  { { "$[%a_@*][%w_]*" }, "keyword2" },
			  { { "$[%$][%a]+" }, "keyword2" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "elseif", "keyword" },
			  { "in", "keyword" },
			  { "exit", "function" },
			  { "TBD", "comment" },
			  { "XXX", "comment" },
			  { "dynamicparam", "keyword" },
			  { "continue", "keyword" },
			  { "finally", "keyword" },
			  { "default", "keyword" },
			  { "begin", "function" },
			  { "Parameter", "keyword" },
			  { "switch", "keyword" },
			  { "data", "keyword" },
			  { "sequence", "function" },
			  { "workflow", "keyword" },
			  { "configuration", "keyword" },
			  { "return", "keyword" },
			  { "try", "keyword" },
			  { "foreach", "keyword" },
			  { "class", "keyword" },
			  { "process", "function" },
			  { "end", "function" },
			  { "TODO", "comment" },
			  { "filter", "keyword" },
			  { "ValidateScript", "keyword" },
			  { "false", "literal" },
			  { "inlinescript", "function" },
			  { "for", "keyword" },
			  { "while", "keyword" },
			  { "select", "function" },
			  { "function", "keyword" },
			  { "throw", "keyword" },
			  { "where", "function" },
			  { "catch", "keyword" },
			  { "true", "literal" },
			  { "else", "keyword" },
			  { "parallel", "function" },
			  { "enum", "keyword" },
			  { "param", "keyword" },
			  { "NOTE", "comment" },
			  { "break", "keyword" },
			  { "CmdletBinding", "keyword" },
			  { "trap", "keyword" },
			  { "until", "keyword" },
			  { "if", "keyword" },
			  { "do", "keyword" },
			  { "FIXME", "comment" },
			  { "HACK", "comment" },

		  },
		  "#",
		  {}

		} );
}

}}}} // namespace EE::UI::Doc::Language
