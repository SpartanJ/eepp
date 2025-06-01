#include <eepp/ui/doc/languages/objective-c.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addObjectiveC() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "Objective-C",
		  { "%.m$" },
		  {
			  { { "//.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "#", "\n" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "-?0x%x+" }, "number" },
			  { { "-?%d+[%d%.eE]*f?" }, "number" },
			  { { "-?%.?%d+f?" }, "number" },
			  { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "@[%a_][%w_]*" }, "type" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "elseif", "keyword" },	 { "int", "type" },			{ "then", "keyword" },
			  { "unsigned", "type" },	 { "continue", "keyword" }, { "default", "keyword" },
			  { "volatile", "keyword" }, { "switch", "keyword" },	{ "char", "type" },
			  { "double", "type" },		 { "auto", "keyword" },		{ "bool", "type" },
			  { "return", "keyword" },	 { "extern", "keyword" },	{ "NULL", "literal" },
			  { "long", "type" },		 { "void", "keyword" },		{ "union", "keyword" },
			  { "short", "type" },		 { "static", "keyword" },	{ "inline", "keyword" },
			  { "false", "literal" },	 { "for", "keyword" },		{ "goto", "keyword" },
			  { "while", "keyword" },	 { "const", "keyword" },	{ "typedef", "keyword" },
			  { "true", "literal" },	 { "else", "keyword" },		{ "enum", "keyword" },
			  { "nil", "literal" },		 { "float", "type" },		{ "struct", "keyword" },
			  { "break", "keyword" },	 { "case", "keyword" },		{ "if", "keyword" },
			  { "do", "keyword" },

		  },
		  "//",
		  {}

		} );

	sd.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );
	return sd;
}

}}}} // namespace EE::UI::Doc::Language
