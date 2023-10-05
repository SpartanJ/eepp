#include <eepp/ui/doc/languages/markdown.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addMarkdown() {
	auto dynSyntax = []( const SyntaxPattern&, const std::string& match ) -> std::string {
		std::string lang = String::toLower( match.substr( 3 ) );
		String::trimInPlace( lang );
		if ( !lang.empty() && lang[lang.size() - 1] == '\n' )
			lang.pop_back();
		return SyntaxDefinitionManager::instance()->findFromString( lang ).getLanguageName();
	};

	SyntaxDefinitionManager::instance()->add(

		{ "Markdown",
		  { "%.md$", "%.markdown$" },
		  {
			  { { "\\." }, "normal" },
			  { { "```[%w%s+-#]+", "```" }, "function", dynSyntax },
			  { { "<!%-%-", "%-%->" }, "comment" },
			  { { "```", "```" }, "string" },
			  { { "``", "``" }, "string" },
			  { { "`", "`" }, "string" },
			  { { "~~", "~~", "\\" }, "keyword2" },
			  { { "%-%-%-+" }, "comment" },
			  { { "%*%s+" }, "operator" },
			  { { "%*", "[%*\n]", "\\" }, "operator" },
			  { { "%s%_", "[%_\n]", "\\" }, "keyword2" },
			  { { "^%_", "[%_\n]", "\\" }, "keyword2" },
			  { { "^#.-\n" }, "keyword" },
			  { { "\n%_", "[%_\n]", "\\" }, "keyword2" },
			  { { "\n#.-\n" }, "keyword" },
			  { { "%[!%[([^%]].-)%]%((https?://[%w_.~!*:@&+$/?%%#-]-%w[-.%w]*%.%w%w%w?%w?:?%d*/"
				  "?[%w_.~!*:@&+$/?%%#=-]*)%)%]%((https?://[%w_.~!*:@&+$/"
				  "?%%#-]-%w[-.%w]*%.%w%w%w?%w?:?%d*/?[%w_.~!*:@&+$/?%%#=-]*)%)" },
				{ "keyword", "function", "link", "link" } },
			  { { "!?%[([^%]].-)%]%((https?://[%w_.~!*:@&+$/?%%#-]-%w[-.%w]*%.%w%w%w?%w?:?%d*/"
				  "?[%w_.~!*:@&+$/?%%#=-]*)%)" },
				{ "keyword", "function", "link" } },
			  { { "!?%[([^%]].-)%]%((%#+[%w-]*)%)" }, { "keyword", "function", "link" } },
			  { { "https?://[%w_.~!*:@&+$/?%%#-]-%w[-.%w]*%.%w%w%w?%w?:?%d*/?[%w_.~!*:@&+$/"
				  "?%%#=-]*" },
				"link" },

		  },
		  {

		  },
		  "",
		  {}

		} );
}

}}}} // namespace EE::UI::Doc::Language
