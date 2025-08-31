#include <eepp/ui/doc/languages/bolt.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addBolt() {

	return SyntaxDefinitionManager::instance()
		->add(

			{ "Bolt",
			  { "%.bolt$" },
			  {
				  { { "include", "#comment" }, "normal", "", SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#keywords" }, "normal", "", SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#statement" }, "normal", "", SyntaxPatternMatchType::LuaPattern },

			  },
			  {

			  },
			  "",
			  {}

			} )
		.addRepositories( {

			{ "string",
			  {
				  { { "\"", "\"", "\\\\." }, { "string" }, {}, "", SyntaxPatternMatchType::RegEx },

			  } },
			{ "literals",
			  {
				  { { "\\d[\\d_]*(\\.[\\d_]+)?" }, "number", "", SyntaxPatternMatchType::RegEx },
				  { { "\\b(true|false|null)\\b" }, "literal", "", SyntaxPatternMatchType::RegEx },
				  { { "\\bthis\\b" }, "parameter", "", SyntaxPatternMatchType::RegEx },
				  { { "\\[", "\\]" },
					{ "literal" },
					{},
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "#expression" },
						  "normal",
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },
				  { { "include", "#string" }, "normal", "", SyntaxPatternMatchType::LuaPattern },

			  } },
			{ "keywords",
			  {
				  { { "\\b(if|else|for|return|break|continue|do|then)\\b" },
					"keyword",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "(\\+|-|\\*|/)" }, "operator", "", SyntaxPatternMatchType::RegEx },
				  { { "(>|>=|<|<=|=|==|=>|\\!=|\\?\\?)" },
					"operator",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "(\\.|,|:|\\!|\\?|\\?\\.)" }, "operator", "", SyntaxPatternMatchType::RegEx },
				  { { "(type)%s*([%a_][%w_]*)%s*%f[=]" },
					{ "normal", "keyword", "type" },
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "(fn)%s*([%a_][%w_]*)%.([%a_][%w_]*%s*)%f[(]" },
					{ "normal", "keyword", "type", "function" },
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "\\b(function|fn|import|from|as|export|type|typeof|let|const|final|unsealed|"
					  "enum|in|by|to|and|or|not|is|match)\\b" },
					"keyword",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "\\b(any|number|string|bool|array|table|Type)\\b" },
					"type",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "[%a_][%w_]*%s*%f[(]" }, "function" },

			  } },
			{ "statement",
			  {
				  { { "include", "#keywords" }, "normal", "", SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#expression" },
					"normal",
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#code-block" },
					"normal",
					"",
					SyntaxPatternMatchType::LuaPattern },

			  } },
			{ "identifier",
			  {
				  { { "\\b(@|_|\\w)(@|_|\\w|\\d)*\\b" },
					"normal",
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "expression",
			  {
				  { { "include", "#keywords" }, "normal", "", SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#literals" }, "normal", "", SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#identifier" },
					"normal",
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#code-block" },
					"normal",
					"",
					SyntaxPatternMatchType::LuaPattern },

			  } },
			{ "comment",
			  {
				  { { "//", "\\n" }, { "comment" }, {}, "", SyntaxPatternMatchType::RegEx },
				  { { "/\\*", "\\*/" }, { "comment" }, {}, "", SyntaxPatternMatchType::RegEx },

			  } },
			{ "code-block",
			  {
				  { { "\\{", "\\}" },
					{ "normal" },
					{},
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "#comment" },
						  "normal",
						  "",
						  SyntaxPatternMatchType::LuaPattern },
						{ { "include", "#statement" },
						  "normal",
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },

			  } },
		} )
		.setFoldRangeType( FoldRangeType::Braces )
		.setFoldBraces( { { '{', '}' } } )
		.setBlockComment( { "/*", "*/" } );
}

}}}} // namespace EE::UI::Doc::Language
