#include <eepp/ui/doc/languages/configfile.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addConfigFile() {

	SyntaxDefinitionManager::instance()->add(

		{ "Config File",
		  { "%.ini$", "%.conf$", "%.desktop$", "%.service$", "%.cfg$", "%.properties$", "%.wrap$",
			"%.dev$", "Doxyfile", "%.timer$", "%.rules$" },
		  {
			  { { "%f[^%s=,]#%x%x%x%x%x%x%x%x" }, "string" },
			  { { "%f[^%s=,]#%x%x%x%x%x%x" }, "string" },
			  { { "^#.-\n" }, "comment" },
			  { { "^;.-\n" }, "comment" },
			  { { "%f[^%s]#.-\n" }, "comment" },
			  { { "[%a_][%w-+_%s%p]-%f[=]" }, "keyword" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "^%[.-%]" }, "type" },
			  { { "%f[^%s]%[.-%]" }, "type" },
			  { { "^%s*(%w[%w%d_-]+)%s?%f[{]" }, "keyword" },
			  { { "[={}]" }, "operator" },
			  { { "https?://[%w_.~!*:@&+$/?%%#-]-%w[-.%w]*%.%w%w%w?%w?:?%d*/?[%w_.~!*:@&+$/"
				  "?%%#=-]*" },
				"link" },
			  { { "%f[^%s=]-?%d+[%d%._e]*%f[%%;\n]" }, "number" },
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
