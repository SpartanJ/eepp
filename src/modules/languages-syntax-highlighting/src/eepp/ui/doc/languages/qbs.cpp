#include <eepp/ui/doc/languages/qbs.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addQbs() {

	return SyntaxDefinitionManager::instance()
		->add(

			{ "Qbs",
			  { "%.qbs$" },
			  {
				  { { "include", "#import" },
					{ "normal" },
					{},
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#object" },
					{ "normal" },
					{},
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#comment" },
					{ "normal" },
					{},
					"",
					SyntaxPatternMatchType::LuaPattern },

			  },
			  {

			  },
			  "",
			  {}

			} )
		.addRepositories( {

			{ "obj-declaration",
			  {
				  { { "\\b([A-Z][a-z]*([A-Z][a-z]*)*)\\s*\\{", "\\}" },
					{ "normal", "type" },
					{},
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "obj-attributes" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },
						{ { "include", "#comment" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },

			  } },
			{ "attr-array",
			  {
				  { { "\\b([\\w\\.]*)\\s*:\\s*\\[\\s*", "\\]" },
					{ "normal", "parameter" },
					{},
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "#object" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },
						{ { "include", "source.js" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },

			  } },
			{ "comment-contents",
			  {
				  { { "\\b(NOTE|TODO|DEBUG|XXX)\\b" },
					"literal",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "\\b(BUG|FIXME|WARNING)\\b" }, "error", "", SyntaxPatternMatchType::RegEx },

			  } },
			{ "attr-block",
			  {
				  { { "\\b([\\w\\.]*)\\s*:\\s*\\{\\s*", "\\}" },
					{ "normal", "parameter" },
					{},
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "source.js" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },

			  } },
			{ "attr-expr",
			  {
				  { { "\\b([\\w\\.]*)\\s*:\\s*(?=[^\\s]+)", ";|$" },
					{ "normal", "parameter" },
					{},
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "source.js" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },

			  } },
			{ "comment",
			  {
				  { { "(\\/\\/)", "$" },
					{ "comment" },
					{},
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "#comment-contents" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },
				  { { "(\\/\\*)", "(\\*\\/)" },
					{ "comment" },
					{},
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "#comment-contents" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },

			  } },
			{ "string",
			  {
				  { { "'", "'" }, { "string" }, {}, "", SyntaxPatternMatchType::RegEx },
				  { { "\"", "\"" }, { "string" }, {}, "", SyntaxPatternMatchType::RegEx },

			  } },
			{ "attr-prop",
			  {
				  { { "\\b([\\w\\.]*)\\s*:\\s*(?=[A-Z]\\w*\\s*\\{)", "(?=\\})" },
					{ "normal", "parameter" },
					{},
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "#object" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },

			  } },
			{ "import",
			  {
				  { { "\\b(import)\\b", "$" },
					{ "normal", "keyword" },
					{},
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "\\b([\\w\\.]+)\\s+(\\d+\\.\\d+)?\\s" },
						  { "normal", "normal", "number" },
						  {},
						  "",
						  SyntaxPatternMatchType::RegEx },
						{ { "\\b(as)\\s+(\\w*)" },
						  { "normal", "keyword", "type" },
						  {},
						  "",
						  SyntaxPatternMatchType::RegEx },
						{ { "include", "#string" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },
						{ { "include", "#comment" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },

			  } },
			{ "obj-attributes",
			  {
				  { { "include", "#attr-prop" },
					{ "normal" },
					{},
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#attr-array" },
					{ "normal" },
					{},
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#attr-block" },
					{ "normal" },
					{},
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#attr-expr" },
					{ "normal" },
					{},
					"",
					SyntaxPatternMatchType::LuaPattern },

			  } },
			{ "obj-method",
			  {
				  { { "\\b(?=function)\\b", "(?<=\\})" },
					{ "normal" },
					{},
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "source.js" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },

			  } },
			{ "object",
			  {
				  { { "\\b([A-Z][\\w\\.]*)\\s*(\\{|$)", "\\}" },
					{ "normal", "type" },
					{},
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "$self" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },
						{ { "include", "#obj-property" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },
						{ { "include", "#obj-method" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },
						{ { "include", "#obj-declaration" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },
						{ { "include", "#obj-attributes" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },

			  } },
			{ "obj-property",
			  {
				  { { "\\b(readonly)\\s+(?=property)" },
					"keyword",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "\\b(property)\\s+([\\w<>]+)(?=\\s+\\w+\\s*:)" },
					{ "normal", "keyword", "keyword" },
					{},
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "\\b(property)\\s+([\\w<>]+)\\s+(\\w+)\\s*$" },
					{ "normal", "keyword", "keyword", "parameter" },
					{},
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
		} );
}

}}}} // namespace EE::UI::Doc::Language
