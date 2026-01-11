#include <eepp/ui/doc/languages/pkgconfig.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addPkgConfig() {

	return SyntaxDefinitionManager::instance()->add(

		{ "pkg-config",
		  { "%.pc$", "%.pc%.in$" },
		  {
			  { { "#.*" }, "comment" },
			  { { "^(Name|Description|URL|Version|Conflicts|Cflags):" },
				"keyword",
				"",
				SyntaxPatternMatchType::RegEx },
			  { { "^(Requires|Libs)(\\.private)?:" },
				"keyword",
				"",
				SyntaxPatternMatchType::RegEx },
			  { { "(%$%{)([%a_][%w_]*)(%})" }, { "normal", "keyword", "parameter", "keyword" } },
			  { { "^[%w_]+%f[=]" }, "parameter" },
			  { { "=" }, "operator" },

		  },
		  {

		  },
		  "",
		  {}

		} );
}

}}}} // namespace EE::UI::Doc::Language
