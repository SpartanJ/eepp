#include <eepp/ui/doc/languages/cmakecache.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addCMakeCache() {

	return SyntaxDefinitionManager::instance()->add(

		{ "CMakeCache",
		  { "^CMakeCache.txt$" },
		  {
			  { { "//+.*$|#+.*$" }, "comment", "", SyntaxPatternMatchType::RegEx },
			  { { "\\b-ADVANCED\\b" }, "keyword", "", SyntaxPatternMatchType::RegEx },
			  { { "%f[^=].*" }, "string" },
			  { { "\\b(?i:(YES|NO|ON|OFF|TRUE|FALSE|Y|N|\\d+))$" },
				"literal",
				"",
				SyntaxPatternMatchType::RegEx },
			  { { "\\b(?i:([.\\w_-]+))\\b" }, "parameter", "", SyntaxPatternMatchType::RegEx },
			  { { "(:)(BOOL|STRING|FILEPATH|PATH|STATIC|INTERNAL)(=)" },
				{ "operator", "type", "operator" },
				{},
				"",
				SyntaxPatternMatchType::RegEx },

		  },
		  {

		  },
		  "",
		  {}

		} );
}

}}}} // namespace EE::UI::Doc::Language
