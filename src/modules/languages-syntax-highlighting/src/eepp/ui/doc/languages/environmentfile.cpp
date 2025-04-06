#include <eepp/ui/doc/languages/environmentfile.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addEnvironmentFile() {

	SyntaxDefinitionManager::instance()->add(

		{ "Environment File",
		  { "%.env$", "%.env.[%w%-%_]*$" },
		  {
			  { { "#.-\n" }, "comment" },
			  { { "\\[nrtfb\\\"']" }, "literal" },
			  { { "'?\\u%x%x%x%x'?" }, "literal" },
			  { { "(%${)([%w]+[%w_]*)(})" }, { "keyword", "keyword", "keyword2", "keyword" } },
			  { { "%$[%w]+[%w_]*" }, "keyword2" },
			  { { "[%a_][%w-+_%s%p]-%f[=]" }, "keyword" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "^%[.-%]" }, "keyword2" },
			  { { "%s%[.-%]" }, "keyword2" },
			  { { "=" }, "operator" },
			  { { "https?://[%w_.~!*:@&+$/?%%#-]-%w[-.%w]*%.%w%w%w?%w?:?%d*/?[%w_.~!*:@&+$/"
				  "?%%#=-]*" },
				"link" },
			  { { "[a-z]+" }, "symbol" },

		  },
		  {
			  { "false", "literal" },
			  { "export", "literal" },
			  { "null", "literal" },
			  { "true", "literal" },

		  },
		  "#",
		  {}

		} );
}

}}}} // namespace EE::UI::Doc::Language
