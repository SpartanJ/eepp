#include <eepp/ui/doc/languages/svelte.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addSvelte() {

	SyntaxDefinitionManager::instance()
		->add(

			{ "Svelte",
			  { "%.svelte$" },
			  {
				  { { "<%s*[sS][cC][rR][iI][pP][tT]%s+[tT][yY][pP][eE]%s*=%s*['\"]%a+/"
					  "[jJ][aA][vV][aA][sS][cC][rR][iI][pP][tT]['\"]%s*>",
					  "<%s*/[sS][cC][rR][iI][pP][tT]>" },
					"function",
					"JavaScript" },
				  { { "<%s*[sS][cC][rR][iI][pP][tT]%s+([lL][aA][nN][gG])%s*(=)%s*(['\"][tT][sS]['"
					  "\"])%s*>",
					  "<%s*/[sS][cC][rR][iI][pP][tT]>" },
					{ "function", "keyword", "operator", "string" },
					"TypeScript" },
				  { { "<%s*[sS][cC][rR][iI][pP][tT]%s*>", "<%s*/%s*[sS][cC][rR][iI][pP][tT]>" },
					"function",
					"JavaScript" },
				  { { "<%s*[sS][tT][yY][lL][eE][^>]*>", "<%s*/%s*[sS][tT][yY][lL][eE]%s*>" },
					"function",
					"CSS" },
				  { { "<!%-%-", "%-%->" }, "comment" },
				  { { "\"", "\"", "\\" }, "string" },
				  { { "'", "'", "\\" }, "string" },
				  { { "{@html[^}]*}" }, "keyword", "", true },
				  { { "{@debug[^}]*}" }, "keyword", "", true },
				  { { "{(:else|:then|:catch)}" },
					std::vector<std::string>{ "keyword", "keyword2" },
					"",
					true },
				  { { "{(/if|/each|/await|/key)}" },
					std::vector<std::string>{ "keyword", "keyword2" },
					"",
					true },
				  { { "{(#if|#each|#await|#key)", "}", "\\" },
					std::vector<std::string>{ "keyword", "keyword2", "normal" },
					"JavaScript",
					true },
				  { { "{", "}", "\\" }, "keyword", "JavaScript" },
				  { { "([%a_][%w_-]*)(:)([%a_][%w_-]*)(=)" },
					{ "keyword", "keyword2", "normal", "keyword", "operator" } },
				  { { "([%a_][%w_-]*)(=)" }, { "keyword", "keyword", "operator" } },
				  { { "0x[%da-fA-F]+" }, "number" },
				  { { "-?%d+[%d%.]*f?" }, "number" },
				  { { "-?%.?%d+f?" }, "number" },
				  { { "%f[^<][%a_][%w%_%-]*(:)([%w_-]+)" }, { "function", "normal", "keyword" } },
				  { { "%f[^<]/[%a_][%w%_%-]*(:)([%w_-]+)" }, { "function", "normal", "keyword" } },
				  { { "%f[^<]![%a_][%w%_%-]*" }, "keyword2" },
				  { { "%f[^<][%a_][%w%_%-]*" }, "function" },
				  { { "%f[^<]/[%a_][%w%_%-]*" }, "function" },
				  { { "[/<>=]" }, "operator" },

			  },
			  {

			  },
			  "",
			  {}

			} )
		.setAutoCloseXMLTags( true );
}

}}}} // namespace EE::UI::Doc::Language
