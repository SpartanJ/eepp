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
			  { { "~~", "~~", "\\" }, "keyword2" },
			  { { "%-%-%-+" }, "comment" },
			  { { "%*%s+" }, "operator" },
			  { { "%*", "[%*\n]", "\\" }, "operator" },
			  { { "%s%_", "[%_\n]", "\\" }, "keyword2" },
			  { { "^%_", "[%_\n]", "\\" }, "keyword2" },
			  { { "^#.-\n" }, "keyword" },
			  { { "\n%_", "[%_\n]", "\\" }, "keyword2" },
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
}

}}}} // namespace EE::UI::Doc::Language
