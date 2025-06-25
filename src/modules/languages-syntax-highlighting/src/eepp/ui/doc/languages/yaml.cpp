#include <eepp/ui/doc/languages/yaml.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addYAML() {

	SyntaxDefinitionManager::instance()
		->add(

			{ "YAMLBL",
			  {},
			  {
				  { { "#", "\n" }, "comment" },
				  { { "\"", "\"", "\\" }, "string" },
				  { { "'", "'", "\\" }, "string" },
				  { { "([%w%d]+%g+)(%s*)(:)(%s)" }, { "type", "normal", "operator", "normal" } },
				  { { "%$[%a%w_]+" }, "keyword" },
				  { { "%$%{%{.-%}%}" }, "keyword" },
				  { { "%-?%.inf" }, "number" },
				  { { "%.NaN" }, "number" },
				  { { "[%+%-]?0%d+" }, "number" },
				  { { "[%+%-]?0x%x+" }, "number" },
				  { { "[%+%-]?%d+[,%.eE:%+%d]*%d+" }, "number" },
				  { { "[%+%-]?%d+" }, "number" },
				  { { "," }, "operator" },
				  { { "%w+" }, "string" },
				  { { "[_%(%)%*@~`!%%%^&=%+%-\\;%.><%?/%s]+" }, "string" },

			  },
			  {

			  },
			  "",
			  {}

			} )
		.setVisible( false );

	return SyntaxDefinitionManager::instance()
		->add(

			{ "YAML",
			  { "%.yml$", "%.yaml$" },
			  {
				  { { "^[%w%d]+%g+%s*%f[:]" }, "keyword" },
				  { { "^%s+[%w%d]+%g+%s*%f[:]" }, "keyword" },
				  { { ":%s+%[", "%]" }, { "operator" }, {}, "YAMLBL" },
				  { { ":%s+{", "}" }, { "operator" }, {}, "YAMLBL" },
				  { { "(^%s+)([%w%d]+%g+)(%s*)(:)(%s)" },
					{ "normal", "normal", "keyword", "normal", "operator", "normal" } },
				  { { "^%s+(%-)%s+([%w%d]+%g+)%s*(:)%s" },
					{ "normal", "operator", "keyword", "operator" } },
				  { { "^%s*%[", "%]" }, { "operator" }, {}, "YAMLBL" },
				  { { "^%s*{", "}" }, { "operator" }, {}, "YAMLBL" },
				  { { "^%s*%-%s*%[", "%]" }, { "operator" }, {}, "YAMLBL" },
				  { { "^%s*%-%s*{", "}" }, { "operator" }, {}, "YAMLBL" },
				  { { "%s+" }, "normal" },
				  { { "#", "\n" }, "comment" },
				  { { "\"", "\"", "\\" }, "string" },
				  { { "'", "'", "\\" }, "string" },
				  { { "!!%w+%s+%[", "%]" }, { "operator" }, {}, "YAMLBL" },
				  { { "!!%w+%s+{", "}" }, { "operator" }, {}, "YAMLBL" },
				  { { "%-?%.inf" }, "number" },
				  { { "%.NaN" }, "number" },
				  { { "(^%-)%s+([%w%d]+%g+)%s*(:)%s" },
					{ "normal", "operator", "keyword", "operator" } },
				  { { "(%&)(%g+)" }, { "normal", "keyword", "type" } },
				  { { "<<" }, "literal" },
				  { { "(%*)([%w%d_-]+)" }, { "normal", "keyword", "type" } },
				  { { "!!%g+" }, "keyword" },
				  { { "(^[%w%d]+%g+)%s*(:)%s" }, { "normal", "literal", "operator" } },
				  { { "%$[%a%w_]+" }, "keyword" },
				  { { "%$%{%{.-%}%}" }, "keyword" },
				  { { "[%+%-]?0%d+" }, "number" },
				  { { "[%+%-]?0x%x+" }, "number" },
				  { { "[%+%-]?%d+[,%.eE:%+%d]*%d+" }, "number" },
				  { { "[%+%-]?%d+" }, "number" },
				  { { "[%*%|%!>%%]" }, "keyword" },
				  { { "[%-%$:%?]+" }, "operator" },
				  { { "[%d%a_][%g_]*" }, "string" },
				  { { "%p+" }, "string" },
				  { { "%w+%f[%s]" }, "normal" },

			  },
			  {
				  { "true", "number" },
				  { "n", "number" },
				  { "y", "number" },
				  { "false", "number" },

			  },
			  "#",
			  { "^%%YAML %d+%.%d+" }

			} )
		.setFoldRangeType( FoldRangeType::Indentation );
}

}}}} // namespace EE::UI::Doc::Language
