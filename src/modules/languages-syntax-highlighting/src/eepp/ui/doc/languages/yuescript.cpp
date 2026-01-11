#include <eepp/ui/doc/languages/yuescript.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addYueScript() {

	SyntaxDefinitionManager::instance()
		->add(

			{ "YueScriptStringInterpolation",
			  {},
			  {
				  { { "#{", "}", "\\" }, "keyword", "YueScript" },
				  { { "[^ ]" }, "string" },

			  },
			  {

			  },
			  "",
			  {}

			} )
		.setVisible( false );

	return SyntaxDefinitionManager::instance()->add(

		{ "YueScript",
		  { "%.yue$" },
		  {
			  { { "%-%-%[%[", "%]%]" }, "comment" },
			  { { "%-%-.*\n" }, "comment" },
			  { { "#!.*\n" }, "comment" },
			  { { "%u[%w_]*" }, "type" },
			  { { "(<)(%s*close)" }, { "normal", "operator", "normal" } },
			  { { "close" }, "keyword" },
			  { { "(macro%s+)([%a_][%w_]*)" }, { "normal", "keyword", "type" } },
			  { { "(class%s+)([%a_][%w_]*)" }, { "normal", "keyword", "type" } },
			  { { "([%a_][%w_]*)(%s*%f[%(@\"'{#])" }, { "normal", "function", "normal" } },
			  { { "([a-zA-Z_][\\w_]*)(\\s+(?:try|default|do|export|switch|and|while|unless|global|"
				  "catch|until|goto|in|or|when|with|extends|for|from|then|as|return|macro|const|"
				  "local|class|if|else|elseif|repeat|continue|break|using|import)\\s+)" },
				{ "normal", "normal", "keyword" },
				"",
				SyntaxPatternMatchType::RegEx },
			  { { "([%a_][%w_]*)(%s+not%s+in)" }, { "normal", "normal", "keyword" } },
			  { { "([%a_][%w_]*)(%s+%f[%[%$%a_])" }, { "normal", "function", "normal" } },
			  { { "(|>%s*)([%a_][%.%w_]*%.%s*)([%a_][%w_]*)" },
				{ "normal", "operator", "normal", "function" } },
			  { { "(|>%s*)([%a_][%w_]*)" }, { "normal", "operator", "function" } },
			  { { "([a-zA-Z_]\\w*\\s+)(\\-?0x[\\da-fA-F](?:_?[\\da-fA-F])*(?:\\.[\\da-fA-F](?:_?["
				  "\\da-fA-F])*)?)" },
				{ "normal", "function", "number" },
				"",
				SyntaxPatternMatchType::RegEx },
			  { { "([a-zA-Z_]\\w*\\s+)(\\-?\\.\\d(?:_?\\d)*(?:[eE][\\+\\-]?\\d(?:_?\\d)*)?)" },
				{ "normal", "function", "number" },
				"",
				SyntaxPatternMatchType::RegEx },
			  { { "([a-zA-Z_]\\w*\\s+)(\\-?(?:\\d(?:_?\\d)*)(?:(?:\\.\\d(?:_?\\d)*(?:[eE][\\+\\-]?"
				  "\\d(?:_?\\d)*)?)|[eE][\\+\\-]?\\d(?:_?\\d)*)?)" },
				{ "normal", "function", "number" },
				"",
				SyntaxPatternMatchType::RegEx },
			  { { "([%a_][%w_]*)(%??%s*!=)" }, { "normal", "normal", "operator" } },
			  { { "([%a_][%w_]*)(%??%s*!)" }, { "normal", "function", "operator" } },
			  { { "(::)([%a_][%w_]*)(::)" }, { "normal", "operator", "type", "operator" } },
			  { { "(::)([%a_][%w_]*)" }, { "normal", "operator", "function" } },
			  { { "(\\)([%a_][%w_]*)" }, { "normal", "operator", "function" } },
			  { { "\\-?0x[\\da-fA-F](?:_?[\\da-fA-F])*(?:\\.[\\da-fA-F](?:_?[\\da-fA-F])*)?" },
				"number",
				"",
				SyntaxPatternMatchType::RegEx },
			  { { "\\-?\\.\\d(?:_?\\d)*(?:[eE][\\+\\-]?\\d(?:_?\\d)*)?" },
				"number",
				"",
				SyntaxPatternMatchType::RegEx },
			  { { "\\-?(?:\\d(?:_?\\d)*)(?:(?:\\.\\d(?:_?\\d)*(?:[eE][\\+\\-]?\\d(?:_?\\d)*)?)|[eE]"
				  "[\\+\\-]?\\d(?:_?\\d)*)?" },
				"number",
				"",
				SyntaxPatternMatchType::RegEx },
			  { { "@@?[%w_]*" }, "type" },
			  { { "$[%w_]+" }, "type" },
			  { { "[%a_][%w_]*" }, "symbol" },
			  { { "\"", "\"", "\\" }, "string", "YueScriptStringInterpolation" },
			  { { "'", "'", "\\" }, "string" },
			  { { "%[=*%[", "%]=*%]" }, "string" },
			  { { "[&<>%+-%*/=#%[%]!|?:~%%%.%^]+" }, "operator" },
			  { { "\\" }, "operator" },

		  },
		  {
			  { "elseif", "keyword" },	{ "in", "keyword" },	   { "then", "keyword" },
			  { "and", "keyword" },		{ "continue", "keyword" }, { "macro", "keyword" },
			  { "default", "keyword" }, { "unless", "keyword" },   { "switch", "keyword" },
			  { "using", "keyword" },	{ "return", "keyword" },   { "import", "keyword" },
			  { "try", "keyword" },		{ "as", "keyword" },	   { "class", "keyword" },
			  { "repeat", "keyword" },	{ "super", "type" },	   { "extends", "keyword" },
			  { "with", "keyword" },	{ "false", "literal" },	   { "for", "keyword" },
			  { "goto", "keyword" },	{ "while", "keyword" },	   { "const", "keyword" },
			  { "from", "keyword" },	{ "self", "type" },		   { "catch", "keyword" },
			  { "true", "literal" },	{ "else", "keyword" },	   { "global", "keyword" },
			  { "nil", "literal" },		{ "not", "keyword" },	   { "when", "keyword" },
			  { "local", "keyword" },	{ "break", "keyword" },	   { "or", "keyword" },
			  { "until", "keyword" },	{ "if", "keyword" },	   { "export", "keyword" },
			  { "do", "keyword" },

		  },
		  "--",
		  {}

		} );
}

}}}} // namespace EE::UI::Doc::Language
