#include <eepp/ui/doc/languages/vue.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addVue() {

	SyntaxDefinitionManager::instance()
		->add( { "Vue-HTML",
				 {},
				 {
					 { { "%{%{%{", "%}%}%}" }, "function", "JavaScript" },
					 { { "%{%{", "%}%}" }, "function", "JavaScript" },
					 { { "<!%-%-", "%-%->" }, "comment" },
					 { { "%f[^>][^<]", "%f[<]" }, "normal" },
					 { { "\"", "\"", "\\" }, "string" },
					 { { "'", "'", "\\" }, "string" },
					 { { "0x[%da-fA-F]+" }, "number" },
					 { { "-?%d+[%d%.]*f?" }, "number" },
					 { { "-?%.?%d+f?" }, "number" },
					 { { "%f[^<]![%a_][%w%_%-]*" }, "type" },
					 { { "%f[^<][%a_][%w%_%-]*" }, "function" },
					 { { "%f[^<]/[%a_][%w%_%-]*" }, "function" },
					 { { "[%a_][%w_]*" }, "keyword" },
					 { { "[/<>=]" }, "operator" },
				 },
				 {},
				 "",
				 {} } )
		.setVisible( false )
		.setAutoCloseXMLTags( true );

	return SyntaxDefinitionManager::instance()->add(
		{ "Vue",
		  { "%.vue?$" },
		  {
			  { { "<%s*[sS][cC][rR][iI][pP][tT][^>]*>", "<%s*/%s*[sS][cC][rR][iI][pP][tT]>" },
				"function",
				"JavaScript" },
			  { { "<%s*[sS][tT][yY][lL][eE][^>]*>", "<%s*/%s*[sS][tT][yY][lL][eE]%s*>" },
				"function",
				"CSS" },
			  { { "<%s*[tT][eE][mM][pP][lL][aA][tT][eE][^>]*>",
				  "<%s*/%s*[tT][eE][mM][pP][lL][aA][tT][eE]%s*>" },
				"function",
				"Vue-HTML" },
			  { { "<!%-%-", "%-%->" }, "comment" },
			  { { "%f[^>][^<]", "%f[<]" }, "normal" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "0x[%da-fA-F]+" }, "number" },
			  { { "-?%d+[%d%.]*f?" }, "number" },
			  { { "-?%.?%d+f?" }, "number" },
			  { { "%f[^<]![%a_][%w%_%-]*" }, "type" },
			  { { "%f[^<][%a_][%w%_%-]*" }, "function" },
			  { { "%f[^<]/[%a_][%w%_%-]*" }, "function" },
			  { { "[%a_][%w_]*" }, "keyword" },
			  { { "[/<>=]" }, "operator" },
		  },
		  {},
		  "",
		  {} } );
}

}}}} // namespace EE::UI::Doc::Language
