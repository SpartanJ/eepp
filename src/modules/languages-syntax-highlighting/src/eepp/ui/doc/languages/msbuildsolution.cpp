#include <eepp/ui/doc/languages/msbuildsolution.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addMsbuildsolution() {

	return SyntaxDefinitionManager::instance()
		->add(

			{ "MSBuild Solution",
			  { "%.sln$" },
			  {
				  { { "include", "#strings" },
					{ "normal" },
					{},
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#keywords" },
					{ "normal" },
					{},
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#header" },
					{ "normal" },
					{},
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#guids" },
					{ "normal" },
					{},
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "#", "\n" }, "comment" },

			  },
			  {

			  },
			  "",
			  {}

			} )
		.addRepositories( {

			{ "strings",
			  {
				  { { "\"", "\"" },
					{ "string" },
					{},
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "#guids" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },

			  } },
			{ "keywords",
			  {
				  { { "\\b((End)?(Global(Section)?)|(End)?Project)\\b" },
					"keyword",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "(=)" }, "operator", "", SyntaxPatternMatchType::RegEx },
				  { { "(,)" }, "operator", "", SyntaxPatternMatchType::RegEx },

			  } },
			{ "header",
			  {
				  { { "\\b(Microsoft Visual Studio Solution File\\, Format Version .*$)\\b" },
					"literal",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "\\b^((Minimum)?VisualStudioVersion)\\b" },
					"literal",
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "guids",
			  {
				  { { "(\\{)", "(\\})" },
					{ "normal", "operator" },
					{ "normal", "operator" },
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "([0-9A-Fa-f]{8}[-][0-9A-Fa-f]{4}[-][0-9A-Fa-f]{4}[-][0-9A-Fa-f]{4}[-]["
							"0-9A-Fa-f]{12})" },
						  "number",
						  "",
						  SyntaxPatternMatchType::RegEx },

					} },

			  } },
		} );
}

}}}} // namespace EE::UI::Doc::Language
