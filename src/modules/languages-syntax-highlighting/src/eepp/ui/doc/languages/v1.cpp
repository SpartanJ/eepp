#include <eepp/ui/doc/languages/v1.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addV1() {

	SyntaxDefinitionManager::instance()
		->add( { "V1",
				 { "%.v1$" },
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
					 { { "<%?v?1?", "%?>" }, "function", "V1Core" },
					 { { "<!%-%-", "%-%->" }, "comment" },
					 { { "%f[^>][^<]", "%f[<]" }, "normal" },
					 { { "\"", "\"", "\\" }, "string" },
					 { { "'", "'", "\\" }, "string" },
					 { { "0x[%da-fA-F]+" }, "number" },
					 { { "-?%d+[%d%.]*f?" }, "number" },
					 { { "-?%.?%d+f?" }, "number" },
					 { { "%f[^<]![%a_][%w%_%-]*" }, "keyword2" },
					 { { "%f[^<][%a_][%w%_%-]*" }, "function" },
					 { { "%f[^<]/[%a_][%w%_%-]*" }, "function" },
					 { { "[%a_][%w_]*" }, "keyword" },
					 { { "[/<>=]" }, "operator" },
				 },
				 {},
				 "",
				 { "^#!.*[ /]v1" } } )
		.setAutoCloseXMLTags( true );

	auto& sd = SyntaxDefinitionManager::instance()
				   ->add( { "V1Core",
							{},
							{
								{ { "<%?v?1?" }, "function" },
								{ { "%?>", "<%?v?1?" }, "function", "HTML" },
								{ { "//.-\n" }, "comment" },
								{ { "/%*", "%*/" }, "comment" },
								{ { "#.-\n" }, "comment" },
								{ { "\"", "\"", "\\" }, "string" },
								{ { "'", "'", "\\" }, "string" },
								{ { "%\\x[%da-fA-F]+" }, "number" },
								{ { "-?%d+[%d%.eE]*" }, "number" },
								{ { "-?%.?%d+" }, "number" },
								{ { "[%.%+%-=/%*%^%%<>!~|&]" }, "operator" },
								{ { "(if|for|while|foreach|switch)\\s*(?=\\()" },
								  { "normal", "keyword", "keyword" },
								  "",
								  SyntaxPatternMatchType::RegEx },
								{ { "[%a_][%w_]*%s*%f[(]" }, "function" },
								{ { "[%a_][%w_]*" }, "symbol" },
							},
							{

								{ "return", "keyword" },   { "if", "keyword" },
								{ "else", "keyword" },	   { "as", "keyword" },
								{ "do", "keyword" },	   { "for", "keyword" },
								{ "foreach", "keyword" },  { "while", "keyword" },
								{ "switch", "keyword" },   { "case", "keyword" },
								{ "continue", "keyword" }, { "default", "keyword" },
								{ "break", "keyword" },	   { "exit", "keyword" },
								{ "function", "keyword" }, { "global", "keyword" },
								{ "const", "keyword" },	   { "true", "literal" },
								{ "false", "literal" },	   { "null", "literal" },

							},
							"//",
							{},
							"v1" } )
				   .setVisible( false );

	sd.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );
}

}}}} // namespace EE::UI::Doc::Language
