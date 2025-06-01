#include <eepp/ui/doc/languages/rescript.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addRescript() {

	return SyntaxDefinitionManager::instance()->add(

		{ "ReScript",
		  { "%.res$" },
		  {
			  { { "//.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "\\?\"", "\"", "\\" }, "string" },
			  { { "\\?`", "`", "\\" }, "string" },

			  { { "%f[^<]![%a_][%w%_%-]*" }, "type" },
			  { { "%f[^<][%a_][%w%_%-]*" }, "function" },
			  { { "%f[^<]/[%a_][%w%_%-]*" }, "function" },
			  { { "([%a_-][%w-_]*)(%\?\?)(=)%f[%{%\"]" },
				{ "normal", "keyword", "normal", "operator" } },

			  { { "js_number_parser" }, "number", "", SyntaxPatternMatchType::Parser },
			  { { "#[%a_][%w_]*" }, "literal" },
			  { { "%l[%w_]*%f[(]" }, "function" },
			  { { "%u[%w_]*" }, "type" },
			  { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
			  { { "'[%a_][%w_]*" }, "parameter" },
			  { { "@%l[%w_]*" }, "parameter" },
			  { { "[%l_][%w_%.]*" }, "symbol" },

		  },
		  {
			  { "int", "type" },		   { "in", "keyword" },		 { "external", "keyword" },
			  { "and", "keyword" },		   { "include", "keyword" }, { "switch", "keyword" },
			  { "bool", "type" },		   { "try", "keyword" },	 { "as", "keyword" },
			  { "exception", "keyword" },  { "lazy", "keyword" },	 { "of", "keyword" },
			  { "rec", "keyword" },		   { "with", "keyword" },	 { "module", "keyword" },
			  { "false", "literal" },	   { "for", "keyword" },	 { "array", "type" },
			  { "while", "keyword" },	   { "assert", "keyword" },	 { "true", "literal" },
			  { "else", "keyword" },	   { "option", "type" },	 { "when", "keyword" },
			  { "downto", "keyword" },	   { "string", "type" },	 { "if", "keyword" },
			  { "constraint", "keyword" }, { "let", "keyword" },	 { "mutable", "keyword" },
			  { "open", "keyword" },	   { "type", "keyword" },	 { "to", "keyword" },

		  },
		  "//",
		  {}

		} );
}

}}}} // namespace EE::UI::Doc::Language
