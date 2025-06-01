#include <eepp/ui/doc/languages/php.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addPHP() {

	auto& sd =
		SyntaxDefinitionManager::instance()
			->add(
				{ "PHP",
				  { "%.php$", "%.php3$", "%.php4$", "%.php5$" },
				  {
					  { { "<%s*[sS][cC][rR][iI][pP][tT]%s+[tT][yY][pP][eE]%s*=%s*['\"]%a+/"
						  "[jJ][aA][vV][aA][sS][cC][rR][iI][pP][tT]['\"]%s*>",
						  "<%s*/[sS][cC][rR][iI][pP][tT]>" },
						"function",
						"JavaScript" },
					  { { "<%s*[sS][cC][rR][iI][pP][tT]%s*>", "<%s*/%s*[sS][cC][rR][iI][pP][tT]>" },
						"function",
						"JavaScript" },
					  { { "<%s*[sS][tT][yY][lL][eE][^>]*>", "<%s*/%s*[sS][tT][yY][lL][eE]%s*>" },
						"function",
						"CSS" },
					  { { "<%?p?h?p?", "%?>" }, "function", "PHPCore" },
					  { { "<!%-%-", "%-%->" }, "comment" },
					  { { "%f[^>][^<]", "%f[<]" }, "normal" },
					  { { "\"", "\"", "\\" }, "string" },
					  { { "'", "'", "\\" }, "string" },
					  { { "0x[%da-fA-F]+" }, "number" },
					  { { "-?%d+[%d%.]*f?" }, "number" },
					  { { "-?%.?%d+f?" }, "number" },
					  { { "%f[^<]![%a_][%w%_%-]*" }, "type" },
					  { { "%f[^<][%a_][%w%_%-]*" }, "function" },
					  { { "%f[^<]/[%a_][%w%_%-]*" }, "function" },
					  { { "[%a_][%w_]*" }, "keyword" },
					  { { "[/<>=]" }, "operator" },
				  },
				  {},
				  "",
				  { "^#!.*[ /]php" } } )
			.setAutoCloseXMLTags( true );

	SyntaxDefinitionManager::instance()
		->add( { "PHPCore",
				 {},
				 {
					 { { "<%?p?h?p?" }, "function" },
					 { { "%?>", "<%?p?h?p?" }, "function", "HTML" },
					 { { "//.-\n" }, "comment" },
					 { { "/%*", "%*/" }, "comment" },
					 { { "#.-\n" }, "comment" },
					 { { "\"", "\"", "\\" }, "string" },
					 { { "'", "'", "\\" }, "string" },
					 { { "%\\x[%da-fA-F]+" }, "number" },
					 { { "-?%d+[%d%.eE]*" }, "number" },
					 { { "-?%.?%d+" }, "number" },
					 { { "[%.%+%-=/%*%^%%<>!~|&]" }, "operator" },
					 { { "[%a_][%w_]*%f[(]" }, "function" },
					 { { "%$[%a][%w_]*" }, "type" },
					 { { "[%a_][%w_]*" }, "symbol" },
				 },
				 { { "return", "keyword" },		{ "if", "keyword" },
				   { "else", "keyword" },		{ "elseif", "keyword" },
				   { "endif", "keyword" },		{ "declare", "keyword" },
				   { "enddeclare", "keyword" }, { "switch", "keyword" },
				   { "endswitch", "keyword" },	{ "as", "keyword" },
				   { "do", "keyword" },			{ "for", "keyword" },
				   { "endfor", "keyword" },		{ "foreach", "keyword" },
				   { "endforeach", "keyword" }, { "while", "keyword" },
				   { "endwhile", "keyword" },	{ "switch", "keyword" },
				   { "case", "keyword" },		{ "continue", "keyword" },
				   { "default", "keyword" },	{ "break", "keyword" },
				   { "exit", "keyword" },		{ "goto", "keyword" },

				   { "catch", "keyword" },		{ "throw", "keyword" },
				   { "try", "keyword" },		{ "finally", "keyword" },

				   { "class", "keyword" },		{ "trait", "keyword" },
				   { "interface", "keyword" },	{ "public", "keyword" },
				   { "static", "keyword" },		{ "protected", "keyword" },
				   { "private", "keyword" },	{ "abstract", "keyword" },
				   { "final", "keyword" },

				   { "function", "type" },		{ "global", "type" },
				   { "var", "type" },			{ "const", "type" },
				   { "bool", "type" },			{ "boolean", "type" },
				   { "int", "type" },			{ "integer", "type" },
				   { "real", "type" },			{ "double", "type" },
				   { "float", "type" },			{ "string", "type" },
				   { "array", "type" },			{ "object", "type" },
				   { "callable", "type" },		{ "iterable", "type" },

				   { "namespace", "type" },		{ "extends", "type" },
				   { "implements", "type" },	{ "instanceof", "type" },
				   { "require", "type" },		{ "require_once", "type" },
				   { "include", "type" },		{ "include_once", "type" },
				   { "use", "type" },			{ "new", "type" },
				   { "clone", "type" },

				   { "true", "literal" },		{ "false", "literal" },
				   { "NULL", "literal" },		{ "parent", "literal" },
				   { "self", "literal" },		{ "echo", "function" } },
				 "//",
				 {},
				 "php" } )
		.setVisible( false );

	sd.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );
	return sd;
}

}}}} // namespace EE::UI::Doc::Language
