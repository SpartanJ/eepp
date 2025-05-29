#include <eepp/ui/doc/languages/gleam.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addGleam() {

	return SyntaxDefinitionManager::instance()
		->add(

			{ "Gleam",
			  { "%.gleam$" },
			  {
				  { { "include", "#comments" },
					{ "normal" },
					{},
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#keywords" },
					{ "normal" },
					{},
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#strings" },
					{ "normal" },
					{},
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#constant" },
					{ "normal" },
					{},
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#entity" },
					{ "normal" },
					{},
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#discards" },
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

			{ "strings",
			  {
				  { { "\"", "\"", "\\\\." }, { "string" }, {}, "", SyntaxPatternMatchType::RegEx },

			  } },
			{ "keywords",
			  {
				  { { "\\b(as|use|case|if|fn|import|let|assert|pub|type|opaque|const|todo|panic|"
					  "else|echo)\\b" },
					"keyword",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "(<\\-|\\->)" }, "operator", "", SyntaxPatternMatchType::RegEx },
				  { { "\\|>" }, "operator", "", SyntaxPatternMatchType::RegEx },
				  { { "\\.\\." }, "operator", "", SyntaxPatternMatchType::RegEx },
				  { { "(==|!=)" }, "operator", "", SyntaxPatternMatchType::RegEx },
				  { { "(<=\\.|>=\\.|<\\.|>\\.)" }, "operator", "", SyntaxPatternMatchType::RegEx },
				  { { "(<=|>=|<|>)" }, "operator", "", SyntaxPatternMatchType::RegEx },
				  { { "(&&|\\|\\|)" }, "operator", "", SyntaxPatternMatchType::RegEx },
				  { { "<>" }, "operator", "", SyntaxPatternMatchType::RegEx },
				  { { "\\|" }, "operator", "", SyntaxPatternMatchType::RegEx },
				  { { "(\\+\\.|\\-\\.|/\\.|\\*\\.)" },
					"operator",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "(\\+|\\-|/|\\*|%)" }, "operator", "", SyntaxPatternMatchType::RegEx },
				  { { "=" }, "operator", "", SyntaxPatternMatchType::RegEx },

			  } },
			{ "entity",
			  {
				  { { "\\b([[:lower:]][[:word:]]*)\\b[[:space:]]*\\(", "\\)" },
					{ "normal", "function" },
					{},
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "$self" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },
				  { { "\\b([[:lower:]][[:word:]]*):\\s" },
					"keyword3",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "\\b([[:lower:]][[:word:]]*):" },
					"normal",
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "hexadecimal_number",
			  {
				  { { "\\b0[xX][0-9a-fA-F_]+\\b" }, "number", "", SyntaxPatternMatchType::RegEx },

			  } },
			{ "discards",
			  {
				  { { "\\b_(?:[[:word:]]+)?\\b" }, "normal", "", SyntaxPatternMatchType::RegEx },

			  } },
			{ "octal_number",
			  {
				  { { "\\b0[oO][0-7_]*\\b" }, "number", "", SyntaxPatternMatchType::RegEx },

			  } },
			{ "decimal_number",
			  {
				  { { "\\b([0-9][0-9_]*)(\\.([0-9_]*)?(e-?[0-9]+)?)?\\b" },
					"number",
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "constant",
			  {
				  { { "include", "#binary_number" },
					{ "normal" },
					{},
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#octal_number" },
					{ "normal" },
					{},
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#hexadecimal_number" },
					{ "normal" },
					{},
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#decimal_number" },
					{ "normal" },
					{},
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "[[:upper:]][[:alnum:]]*" }, "keyword2", "", SyntaxPatternMatchType::RegEx },

			  } },
			{ "comments",
			  {
				  { { "//.*" }, "comment", "", SyntaxPatternMatchType::RegEx },

			  } },
			{ "binary_number",
			  {
				  { { "\\b0[bB][01_]*\\b" }, "number", "", SyntaxPatternMatchType::RegEx },

			  } },
		} );
}

}}}} // namespace EE::UI::Doc::Language
