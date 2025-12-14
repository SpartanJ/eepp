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
			  { { "```[%w \t%+%-#]+", "```" }, "function", dynSyntax },
			  { { "include", "#comments" }, "normal" },
			  { { "include", "#strings" }, "normal" },
			  { { "include", "#enumerations" }, "normal" },
			  { { "include", "#decorations" }, "normal" },
			  { { "include", "#headings" }, "normal" },
			  { { "include", "#links" }, "normal" },
		  },
		  {},
		  "",
		  {} } );

	sd.addRepositories( {

		{ "comments",
		  {
			  { { "<!%-%-", "%-%->" }, "comment" },
			  { { "%-%-%-+" }, "comment" },
		  } },

		{ "strings",
		  {
			  { { "```", "```" }, "string" },
			  { { "``", "(``|\n)" }, { "string" }, {}, "", SyntaxPatternMatchType::RegEx },
			  { { "`", "[`\n]" }, "string" },
			  { { "^%s*>+%s.*" }, "string" },
		  } },

		{ "enumerations",
		  {
			  { { "^%s*%*%s" }, "number" },
			  { { "^%s*%-%s" }, "number" },
			  { { "^%s*%+%s" }, "number" },
			  { { "^%s*[0-9]+[%.%)]%s" }, "number" },
			  { { "%*%s+" }, "number" },
		  } },

		{ "headings",
		  {
			  { { "^#.-\n" }, "keyword" },
			  { { "\n#.-\n" }, "keyword" },
			  { { "^=-\n" }, "keyword" },
		  } },

		{ "links",
		  {
			  { { "%[!%[([^%]]-)%]%((https?://[%w_.~!*:@&+$/?%%#-]-%w[-.%w]*%.%w%w%w?%w?:?%d*/"
				  "?[%w_.~!*:@&+$/?%%#=-]*)%)%]%((https?://[%w_.~!*:@&+$/"
				  "?%%#-]-%w[-.%w]*%.%w%w%w?%w?:?%d*/?[%w_.~!*:@&+$/?%%#=-]*)%)" },
				{ "keyword", "function", "link", "link" } },
			  { { "!?%[([^%]]-)%]%((https?://[%w_.~!*:@&+$/?%%#-]-%w[-.%w]*%.%w%w%w?%w?:?%d*/"
				  "?[%w_.~!*:@&+$/?%%#=-]*)%)" },
				{ "keyword", "function", "link" } },
			  { { "!?%[([^%]]-)%]%((%#+[%w-]*)%)" }, { "keyword", "function", "link" } },
			  { { "!?%[([^%]]-)%]%(([%w_.~!*:@&+$/?%%#=-]*)%)" },
				{ "keyword", "function", "string" } },
			  { { "https?://[%w_.~!*:@&+$/?%%#-]-%w[-.%w]*%.%w%w%w?%w?:?%d*/?[%w_.~!*:@&+$/"
				  "?%%#=-]*" },
				"link" },
		  } },

		{ "inside_bold",
		  {
			  { { "include", "#strings" }, "normal" },
			  { { "include", "#decorations_italic" }, "normal" },
			  { { "include", "#decorations_strikethrough" }, "normal" },
			  { { "include", "#links" }, "normal" },
		  } },

		{ "decorations_bold",
		  {
			  { { "\\*\\*(?!\\*)", "(\\*\\*|\n)" },
				{ "type,bold" },
				{},
				"",
				SyntaxPatternMatchType::RegEx,
				{
					{ { "include", "#inside_bold" }, "type,bold" },
				} },

			  { { "__(?!_)", "(__|\n)" },
				{ "type,bold" },
				{},
				"",
				SyntaxPatternMatchType::RegEx,
				{
					{ { "include", "#inside_bold" }, "type,bold" },
				} },
		  } },

		{ "inside_italic",
		  {
			  { { "include", "#strings" }, "normal" },
			  { { "include", "#decorations_bold" }, "normal" },
			  { { "include", "#decorations_strikethrough" }, "normal" },
			  { { "include", "#links" }, "normal" },
		  } },

		{ "decorations_italic",
		  {
			  { { "%s%_%f[^_]", "[%_\n]" },
				{ "type,italic" },
				{},
				"",
				SyntaxPatternMatchType::LuaPattern,
				{
					{ { "include", "#inside_italic" }, "type,italic" },
				} },

			  { { "^%_%f[^_]", "[%_\n]" },
				{ "type,italic" },
				{},
				"",
				SyntaxPatternMatchType::LuaPattern,
				{
					{ { "include", "#inside_italic" }, "type,italic" },
				} },

			  { { "%s%*%f[^*]", "[%*\n]" },
				{ "type,italic" },
				{},
				"",
				SyntaxPatternMatchType::LuaPattern,
				{
					{ { "include", "#inside_italic" }, "type,italic" },
				} },

			  { { "^%*%f[^*]", "[%*\n]" },
				{ "type,italic" },
				{},
				"",
				SyntaxPatternMatchType::LuaPattern,
				{
					{ { "include", "#inside_italic" }, "type,italic" },
				} },

			  { { "\n%_%f[^_]", "[%_\n]" },
				{ "type,italic" },
				{},
				"",
				SyntaxPatternMatchType::LuaPattern,
				{
					{ { "include", "#inside_italic" }, "type,italic" },
				} },
		  } },

		{ "decorations_strikethrough",
		  {
			  { { "~~~" }, "normal" },
			  { { "~~", "(~~|\n)" },
				{ "type,strikethrough" },
				{},
				"",
				SyntaxPatternMatchType::RegEx,
				{
					{ { "include", "$self" }, "type,strikethrough" },
				} },
			  { { "~", "(~|\n)" },
				{ "type,strikethrough" },
				{},
				"",
				SyntaxPatternMatchType::RegEx,
				{
					{ { "include", "$self" }, "type,strikethrough" },
				} },
		  } },

		{ "decorations",
		  {
			  { { "include", "#decorations_bold" }, "normal" },
			  { { "include", "#decorations_italic" }, "normal" },
			  { { "include", "#decorations_strikethrough" }, "normal" },
		  } },

	} );
	sd.setFoldRangeType( FoldRangeType::Markdown );
	sd.setBlockComment( { "<!--", "-->" } );
}

}}}} // namespace EE::UI::Doc::Language
