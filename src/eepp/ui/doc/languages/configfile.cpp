#include <eepp/ui/doc/languages/configfile.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addConfigFile() {

	SyntaxDefinitionManager::instance()->add(

		{ "Config File",
		  { "%.ini$", "%.conf$", "%.desktop$", "%.service$", "%.cfg$", "%.properties$", "%.wrap$",
			"Doxyfile" },
		  {
			  { { "%s*#%x%x%x%x%x%x%x%x" }, "string" },
			  { { "%s*#%x%x%x%x%x%x" }, "string" },
			  { { "^#.-\n" }, "comment" },
			  { { "^;.-\n" }, "comment" },
			  { { "%s#.-\n" }, "comment" },
			  { { "[%a_][%w-+_%s%p]-%f[=]" }, "keyword" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "^%[.-%]" }, "type" },
			  { { "%s%[.-%]" }, "type" },
			  { { "=" }, "operator" },
			  { { "https?://[%w_.~!*:@&+$/?%%#-]-%w[-.%w]*%.%w%w%w?%w?:?%d*/?[%w_.~!*:@&+$/"
				  "?%%#=-]*" },
				"link" },
			  { { "[a-z]+" }, "symbol" },

		  },
		  {
			  { "false", "literal" },
			  { "true", "literal" },

		  },
		  "#",
		  { "^%[.-%]%f[^\n]" },
		  "ini" } );
}

}}}} // namespace EE::UI::Doc::Language
