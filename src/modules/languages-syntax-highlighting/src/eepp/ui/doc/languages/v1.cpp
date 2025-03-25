#include <eepp/ui/doc/languages/v1.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addV1() {

	auto& sd = SyntaxDefinitionManager::instance()
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

	SyntaxDefinitionManager::instance()
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
					 { { "[%a_][%w_]*%f[(]" }, "function" },
					 { { "[%a_][%w_]*" }, "symbol" },
				 },
				 { { "return", "keyword" },		 { "if", "keyword" },
				   { "else", "keyword" },		 { "elseif", "keyword" },
				   { "endif", "keyword" },		 { "declare", "keyword" },
				   { "enddeclare", "keyword" },	 { "switch", "keyword" },
				   { "endswitch", "keyword" },	 { "as", "keyword" },
				   { "do", "keyword" },			 { "for", "keyword" },
				   { "endfor", "keyword" },		 { "foreach", "keyword" },
				   { "endforeach", "keyword" },	 { "while", "keyword" },
				   { "endwhile", "keyword" },	 { "switch", "keyword" },
				   { "case", "keyword" },		 { "continue", "keyword" },
				   { "default", "keyword" },	 { "break", "keyword" },
				   { "exit", "keyword" },		 { "goto", "keyword" },

				   { "catch", "keyword" },		 { "throw", "keyword" },
				   { "try", "keyword" },		 { "finally", "keyword" },

				   { "class", "keyword" },		 { "trait", "keyword" },
				   { "interface", "keyword" },	 { "public", "keyword" },
				   { "static", "keyword" },		 { "protected", "keyword" },
				   { "private", "keyword" },	 { "abstract", "keyword" },
				   { "final", "keyword" },

				   { "function", "keyword2" },	 { "global", "keyword2" },
				   { "var", "keyword2" },		 { "const", "keyword2" },
				   { "bool", "keyword2" },		 { "boolean", "keyword2" },
				   { "int", "keyword2" },		 { "integer", "keyword2" },
				   { "real", "keyword2" },		 { "double", "keyword2" },
				   { "float", "keyword2" },		 { "string", "keyword2" },
				   { "array", "keyword2" },		 { "object", "keyword2" },
				   { "callable", "keyword2" },	 { "iterable", "keyword2" },

				   { "namespace", "keyword2" },	 { "extends", "keyword2" },
				   { "implements", "keyword2" }, { "instanceof", "keyword2" },
				   { "require", "keyword2" },	 { "require_once", "keyword2" },
				   { "include", "keyword2" },	 { "include_once", "keyword2" },
				   { "use", "keyword2" },		 { "new", "keyword2" },
				   { "clone", "keyword2" },

				   { "true", "literal" },		 { "false", "literal" },
				   { "NULL", "literal" },		 { "parent", "literal" },
				   { "self", "literal" },		 { "echo", "function" } },
				 "//",
				 {},
				 "v1" } )
		.setVisible( false );

	sd.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );
}

}}}} // namespace EE::UI::Doc::Language
