#include <eepp/ui/doc/languages/html.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addHTML() {

	SyntaxDefinitionManager::instance()
		->add(

			{ "HTML",
			  { "%.[mp]?html?$", "%.handlebars$" },
			  {
				  { { "<%s*[sS][cC][rR][iI][pP][tT]%s+[tT][yY][pP][eE]%s*=%s*['\"]%a+/"
					  "[jJ][aA][vV][aA][sS][cC][rR][iI][pP][tT]['\"]%s*>",
					  "<%s*/[sS][cC][rR][iI][pP][tT]>" },
					"function",
					"JavaScript" },
				  { { "<%s*[sS][cC][rR][iI][pP][tT]%s+[tT][yY][pP][eE]%s*=%s*['\"]%a+/"
					  "[pP][yY][tT][hH][oO][nN]['\"]%s*>",
					  "<%s*/[sS][cC][rR][iI][pP][tT]>" },
					"function",
					"Python" },
				  { { "<%s*[sS][cC][rR][iI][pP][tT]%s*>", "<%s*/%s*[sS][cC][rR][iI][pP][tT]>" },
					"function",
					"JavaScript" },
				  { { "<%s*[sS][tT][yY][lL][eE][^>]*>", "<%s*/%s*[sS][tT][yY][lL][eE]%s*>" },
					"function",
					"CSS" },
				  { { "<%?p?h?p?", "%?>" }, "function", "PHPCore" },
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
				  { { "[%a_][%w_-]*" }, "keyword" },
				  { { "[/<>=]" }, "operator" },

			  },
			  {

			  },
			  "",
			  { "^<html", "^<![Dd][Oo][Cc][Tt][Yy][Pp][Ee]%s[Hh][Tt][Mm][Ll]>" }

			} )
		.setAutoCloseXMLTags( true )
		.setBlockComment( { "<!--", "-->" } );
}

}}}} // namespace EE::UI::Doc::Language
