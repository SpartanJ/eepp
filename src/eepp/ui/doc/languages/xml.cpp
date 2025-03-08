#include <eepp/ui/doc/languages/xml.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addXML() {

	SyntaxDefinitionManager::instance()
		->add(

			{ "XML",
			  { "%.xml$", "%.svg$" },
			  {
				  { { "<%s*[sS][tT][yY][lL][eE]%s*>", "<%s*/%s*[sS][tT][yY][lL][eE]%s*>" },
					"function",
					"CSS" },
				  { { "<!%-%-", "%-%->" }, "comment" },
				  { { "%f[^>][^<]", "%f[<]" }, "normal" },
				  { { "(<!%[CDATA%[)", "(%]%]>)", "\\" }, { "string", "function", "function" } },
				  { { "\"", "\"", "\\" }, "string" },
				  { { "'", "'", "\\" }, "string" },
				  { { "0x[%da-fA-F]+" }, "number" },
				  { { "-?%d+[%d%.]*f?" }, "number" },
				  { { "-?%.?%d+f?" }, "number" },
				  { { "%f[^<]![%a_][%w%_%-]*" }, "keyword2" },
				  { { "%f[^<][%a_][%w%_%-]*" }, "function" },
				  { { "%f[^<]/[%a_][%w%_%-]*" }, "function" },
				  { { "[%a_][%w_]*" }, "keyword" },
				  { { "[/<>=]" }, "operator" },

			  },
			  {

			  },
			  "",
			  { "^<%?xml" }

			} )
		.setAutoCloseXMLTags( true );
}

}}}} // namespace EE::UI::Doc::Language
