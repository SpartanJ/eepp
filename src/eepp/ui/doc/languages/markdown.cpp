#include <eepp/ui/doc/languages/markdown.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addMarkdown() {
	auto dynSyntax = []( const SyntaxPattern&, const std::string_view& match ) -> std::string {
		std::string lang = String::toLower( std::string{ match.substr( 3 ) } );
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
			  /*{ { "```[Xx][Mm][Ll]", "```" }, "function", "XML" },
			  { { "```[Hh][Tt][Mm][Ll]", "```" }, "function", "html" },
			  { { "```[Cc]++", "```" }, "function", "C++" },
			  { { "```[Cc][Pp][Pp]", "```" }, "function", "C++" },
			  { { "```[Cc]sharp", "```" }, "function", "C#" },
			  { { "```[Cc][Ss][Ss]", "```" }, "function", "CSS" },
			  { { "```[Cc]", "```" }, "function", "C" },
			  { { "```[Dd][Aa][Rr][Tt]", "```" }, "function", "Dart" },
			  { { "```[Dd]", "```" }, "function", "D" },
			  { { "```[Ll]ua", "```" }, "function", "Lua" },
			  { { "```[Jj][Ss][Oo][Nn]", "```" }, "function", "JSON" },
			  { { "```[Ja]va[Ss]cript", "```" }, "function", "JavaScript" },
			  { { "```[Tt]ype[Ss]cript", "```" }, "function", "TypeScript" },
			  { { "```[Pp]ython", "```" }, "function", "Python" },
			  { { "```[Bb]ash", "```" }, "function", "Bash" },
			  { { "```[Pp][Hh][Pp]", "```" }, "function", "PHPCore" },
			  { { "```[Ss][Qq][Ll]", "```" }, "function", "SQL" },
			  { { "```[Gg][Ll][Ss][Ll]", "```" }, "function", "GLSL" },
			  { { "```[Ii][Nn][Ii]", "```" }, "function", "Config File" },
			  { { "```[Mm]akefile", "```" }, "function", "Makefile" },
			  { { "```[Gg][Oo]", "```" }, "function", "Go" },
			  { { "```[Rr]ust", "```" }, "function", "Rust" },
			  { { "```[Rr]uby", "```" }, "function", "Ruby" },
			  { { "```[Gg][Dd][Ss]cript", "```" }, "function", "GSCript" },
			  { { "```[Jj]ava", "```" }, "function", "java" },
			  { { "```[Ss]wift", "```" }, "function", "Swift" },
			  { { "```[Oo]bjective[Cc]", "```" }, "function", "Objective-C" },
			  { { "```[Yy][Aa][Mm][Ll]", "```" }, "function", "YAML" },
			  { { "```[Kk]otlin", "```" }, "function", "Kotlin" },
			  { { "```[Ss]olidity", "```" }, "function", "Solidity" },
			  { { "```[Hh]askell", "```" }, "function", "Haskell" },
			  { { "```[Oo]din", "```" }, "function", "Odin" },
			  { { "```[Nn]im", "```" }, "function", "Nim" },
			  { { "```[Zz]ig", "```" }, "function", "Zig" },*/
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
