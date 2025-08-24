#include <eepp/ui/doc/languages/markdown.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addMarkdown() {
	auto dynSyntax = []( const SyntaxPattern&, const std::string_view& match ) -> std::string {
		return SyntaxDefinitionManager::instance()
			->findFromString( String::trim( match.substr( 3 ) ) )
			.getLanguageName();
	};

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "Markdown",
		  { "%.md$", "%.markdown$" },
		  {
			  { { "\\." }, "normal" },
			  { { "```[%w \t%+%-#]+", "```" }, "function", dynSyntax },
			  { { "<!%-%-", "%-%->" }, "comment" },
			  { { "```", "```" }, "string" },
			  { { "``", "``" }, "string" },
			  { { "`", "`" }, "string" },
			  { { "~~", "~~", "\\" }, "type" },
			  { { "%-%-%-+" }, "comment" },
			  { { "%*%s+" }, "operator" },
			  { { "%*", "[%*\n]", "\\" }, "operator" },
			  { { "%s%_", "[%_\n]", "\\" }, "type" },
			  { { "^%_", "[%_\n]", "\\" }, "type" },
			  { { "^#.-\n" }, "keyword" },
			  { { "[^\n]#.-\n" }, "keyword" },
			  { { "\n%_", "[%_\n]", "\\" }, "type" },
			  { { "%[!%[([^%]].-)%]%((https?://[%w_.~!*:@&+$/?%%#-]-%w[-.%w]*%.%w%w%w?%w?:?%d*/"
				  "?[%w_.~!*:@&+$/?%%#=-]*)%)%]%((https?://[%w_.~!*:@&+$/"
				  "?%%#-]-%w[-.%w]*%.%w%w%w?%w?:?%d*/?[%w_.~!*:@&+$/?%%#=-]*)%)" },
				{ "keyword", "function", "link", "link" } },
			  { { "!?%[([^%]].-)%]%((https?://[%w_.~!*:@&+$/?%%#-]-%w[-.%w]*%.%w%w%w?%w?:?%d*/"
				  "?[%w_.~!*:@&+$/?%%#=-]*)%)" },
				{ "keyword", "function", "link" } },
			  { { "!?%[([^%]].-)%]%((%#+[%w-]*)%)" }, { "keyword", "function", "link" } },
			  { { "!?%[([^%]].-)%]%(([%w_.~!*:@&+$/?%%#=-]*)%)" },
				{ "keyword", "function", "string" } },
			  { { "https?://[%w_.~!*:@&+$/?%%#-]-%w[-.%w]*%.%w%w%w?%w?:?%d*/?[%w_.~!*:@&+$/"
				  "?%%#=-]*" },
				"link" },

		  },
		  {

		  },
		  "",
		  {}

		} );

	sd.setFoldRangeType( FoldRangeType::Markdown );
	sd.setBlockComment( { "<!--", "-->" } );
}

}}}} // namespace EE::UI::Doc::Language
