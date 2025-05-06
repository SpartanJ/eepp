#include <eepp/ui/doc/languages/yaml.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addYAML() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "YAML",
		  { "%.yml$", "%.yaml$" },
		  {
			  { { "#", "\n" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "%-?%.inf" }, "number" },
			  { { "%.NaN" }, "number" },
			  { { "(%&)(%g+)" }, { "normal", "keyword", "literal" } },
			  { { "!%g+" }, "keyword" },
			  { { "<<" }, "literal" },
			  { { "https?://[%w_.~!*:@&+$/?%%#-]-%w[-.%w]*%.%w%w%w?%w?:?%d*/?[%w_.~!*:@&+$/"
				  "?%%#=-]*" },
				"link" },
			  { { "([%s]%*)([%w%d_]+)" }, { "keyword", "keyword", "keyword2" } },
			  { { "(%*)([%w%d_]+)" }, { "keyword", "keyword", "literal" } },
			  { { "([%[%{])(%s*[%w%d]+%g+%s*)(:%s)" },
				{ "keyword", "operator", "operator", "keyword" } },
			  { { "([%s][%w%d]+%g+%s*)(:%s)" }, { "keyword", "keyword", "operator" } },
			  { { "([%w%d]+%g+%s*)(:%s)" }, { "keyword", "keyword", "operator" } },
			  { { "0%d+" }, "number" },
			  { { "0x%x+" }, "number" },
			  { { "[%+%-]?%d+[,%.eE:%+%d]*%d+" }, "number" },
			  { { "[%*%|%!>%%]" }, "keyword" },
			  { { "[%-:%?%*%{%}%[%]]" }, "operator" },
			  { { "([%d%a_][%g_]*)([%]%},])" }, { "string", "operator", "operator" } },
			  { { "[%d%a$/_][%g_]*" }, "string" },

		  },
		  {
			  { "false", "number" },
			  { "n", "number" },
			  { "y", "number" },
			  { "true", "number" },

		  },
		  "#",
		  { "^%%YAML %d+%.%d+" }

		} );

	sd.setFoldRangeType( FoldRangeType::Indentation );
}

}}}} // namespace EE::UI::Doc::Language
