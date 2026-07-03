#include <eepp/ui/doc/languages/gemini.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addGemini() {

	return SyntaxDefinitionManager::instance()
		->add(

			{ "Gemini",
			  { "%.gmi$" },
			  {
				  { { "include", "#headings" }, "normal", "", SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#links" }, "normal", "", SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#quote" }, "normal", "", SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#raw" }, "normal", "", SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#unorderedLists" },
					"normal",
					"",
					SyntaxPatternMatchType::LuaPattern },

			  },
			  {

			  },
			  "",
			  {}

			} )
		.addRepositories( {

			{ "unorderedLists",
			  {
				  { { "^(\\*)[ \t]+.+\n" },
					{ "normal", "operator" },
					{},
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "raw",
			  {
				  { { "^```.*\n", "^```.*\n" },
					{ "normal" },
					{},
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "quote",
			  {
				  { { "^(>).*$" },
					{ "string", "operator" },
					{},
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "links",
			  {
				  { { "^=>[ \t]+([^ \t]+)(?:[ \t]+(.*))?" },
					{ "normal", "link", "string" },
					{},
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "headings",
			  {
				  { { "^(#)(?:[^#].*)?\n" },
					{ "keyword", "operator" },
					{},
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "^(##)(?:[^#].*)?\n" },
					{ "keyword", "operator" },
					{},
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "^(###)(?:[^#].*)?\n" },
					{ "keyword", "operator" },
					{},
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
		} );
}

}}}} // namespace EE::UI::Doc::Language
