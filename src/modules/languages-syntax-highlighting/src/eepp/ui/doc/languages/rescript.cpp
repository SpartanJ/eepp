#include <eepp/ui/doc/languages/rescript.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addRescript() {

	SyntaxDefinitionManager::instance()->add(

		{ "ReScript",
		  { "%.res$" },
		  {
			  { { "//.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "`", "`", "\\" }, "string" },

			  { { "%f[^<]![%a_][%w%_%-]*" }, "keyword2" },
			  { { "%f[^<][%a_][%w%_%-]*" }, "function" },
			  { { "%f[^<]/[%a_][%w%_%-]*" }, "function" },
			  { { "([%a_-][%w-_]*)(%\?\?)(=)%f[%{%\"]" },
				{ "normal", "keyword", "normal", "operator" } },

			  { { "js_number_parser" }, "number", "", SyntaxPatternMatchType::Parser },
			  { { "#[%a_][%w_]*" }, "literal" },
			  { { "%l[%w_]*%f[(]" }, "function" },
			  { { "%u[%w_]*" }, "keyword2" },
			  { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
			  { { "'[%a_][%w_]*" }, "keyword3" },
			  { { "@%l[%w_]*" }, "keyword3" },
			  { { "[%l_][%w_%.]*" }, "symbol" },

		  },
		  {
			  { "int", "keyword2" },	   { "in", "keyword" },		 { "external", "keyword" },
			  { "and", "keyword" },		   { "include", "keyword" }, { "switch", "keyword" },
			  { "bool", "keyword2" },	   { "try", "keyword" },	 { "as", "keyword" },
			  { "exception", "keyword" },  { "lazy", "keyword" },	 { "of", "keyword" },
			  { "rec", "keyword" },		   { "with", "keyword" },	 { "module", "keyword" },
			  { "false", "literal" },	   { "for", "keyword" },	 { "array", "keyword2" },
			  { "while", "keyword" },	   { "assert", "keyword" },	 { "true", "literal" },
			  { "else", "keyword" },	   { "option", "keyword2" }, { "when", "keyword" },
			  { "downto", "keyword" },	   { "string", "keyword2" }, { "if", "keyword" },
			  { "constraint", "keyword" }, { "let", "keyword" },	 { "mutable", "keyword" },
			  { "open", "keyword" },	   { "type", "keyword" },	 { "to", "keyword" },

		  },
		  "//",
		  {}

		} );
}

}}}} // namespace EE::UI::Doc::Language
