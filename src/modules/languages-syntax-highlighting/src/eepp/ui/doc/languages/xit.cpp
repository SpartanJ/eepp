#include <eepp/ui/doc/languages/xit.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addXit() {
	auto dynSyntax = []( const SyntaxPattern&, const std::string_view& match ) -> std::string {
		std::string lang = String::toLower( std::string{ match.substr( 3 ) } );
		String::trimInPlace( lang );
		if ( !lang.empty() && lang[lang.size() - 1] == '\n' )
			lang.pop_back();
		return SyntaxDefinitionManager::instance()->findFromString( lang ).getLanguageName();
	};

	SyntaxDefinitionManager::instance()->add(

		{ "[x]it!",
		  { "%.xit$" },
		  {
			  { { "```[%w%s+-#]+", "```" }, "function", dynSyntax },
			  { { "%f[^%s%(]%-%>%s%d%d%d%d%-%d%d%-%d%d%f[\n%s%!%?%)]" }, "number" },
			  { { "%f[^%s%(]%-%>%s%d%d%d%d%/%d%d%/%d%d%f[\n%s%!%?%)]" }, "number" },
			  { { "%f[^%s%(]%-%>%s%d%d%d%d%-[wWqQ]?%d%d?%f[\n%s%!%?%)]" }, "number" },
			  { { "%f[^%s%(]%-%>%s%d%d%d%d%/[wWqQ]?%d%d?%f[\n%s%!%?%)]" }, "number" },
			  { { "%f[^%s%(]%-%>%s%d%d%d%d%f[\n%s%!%?%)]" }, "number" },
			  { { "^(%[%s%]%s)([%.!]+)%s" }, { "operator", "operator", "red" } },
			  { { "^(%[x%]%s)([%.!]+)%s" }, { "function", "function", "red" } },
			  { { "^(%[@%]%s)([%.!]+)%s" }, { "keyword", "keyword", "red" } },
			  { { "^(%[~%]%s)([%.!]+)%s" }, { "comment", "comment", "red" } },
			  { { "^(%[%.%]%s)([%.!]+)%s" }, { "comment", "comment", "red" } },
			  { { "%#[%wñàáâãäåèéêëìíîïòóôõöùúûüýÿÑÀÁÂÃÄÅÈÉÊËÌÍÎÏÒÓÔÕÖÙÚÛÜÝ%-%_]+%=\"", "\"" },
				"string" },
			  { { "%#[%wñàáâãäåèéêëìíîïòóôõöùúûüýÿÑÀÁÂÃÄÅÈÉÊËÌÍÎÏÒÓÔÕÖÙÚÛÜÝ%-%_]+%='", "'" },
				"string" },
			  { { "%#[%wñàáâãäåèéêëìíîïòóôõöùúûüýÿÑÀÁÂÃÄÅÈÉÊËÌÍÎÏÒÓÔÕÖÙÚÛÜÝ%-%_]+%=[%w%-%_]*" },
				"string" },
			  { { "%#[%wñàáâãäåèéêëìíîïòóôõöùúûüýÿÑÀÁÂÃÄÅÈÉÊËÌÍÎÏÒÓÔÕÖÙÚÛÜÝ%-%_]+" }, "string" },
			  { { "^%[%s%]%s" }, "operator" },
			  { { "^%[x%]%s" }, "function" },
			  { { "^%[@%]%s" }, "keyword" },
			  { { "^%[~%]%s" }, "comment" },
			  { { "^%[%?%]%s" }, "warning" },
			  { { "^%[%.%]%s" }, "notice" },
			  { { "^[%wñàáâãäåèéêëìíîïòóôõöùúûüýÿÑÀÁÂÃÄÅÈÉÊËÌÍÎÏÒÓÔÕÖÙÚÛÜÝ][%"
				  "wñàáâãäåèéêëìíîïòóôõöùúûüýÿÑÀÁÂÃÄÅÈÉÊËÌÍÎÏÒÓÔÕÖÙÚÛÜÝ%s%p]*%f[\n]" },
				"underline" },
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
