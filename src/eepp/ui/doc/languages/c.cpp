#include <eepp/ui/doc/languages/c.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addC() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "C",
		  { "%.c$", "%.C$", "%.h$", "%.icc$" },
		  {
			  { { "//.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "(#%s*include)%s+([<%\"][%w%d%.%\\%/%_%-]+[>%\"])" },
				{ "keyword", "keyword", "literal" } },
			  { { "\"", "[\"\n]", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "c_number_parser" }, "number", "", SyntaxPatternMatchType::Parser },
			  { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
			  { { "(if|for|while|switch|sizeof|_Alignof|defined)\\s*(?=\\()" },
				{ "normal", "keyword", "keyword" },
				"",
				SyntaxPatternMatchType::RegEx },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "#[%a_][%w_]*" }, "symbol" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "elseif", "keyword" },	  { "int", "keyword2" },	  { "then", "keyword" },
			  { "unsigned", "keyword2" }, { "continue", "keyword" },  { "default", "keyword" },
			  { "volatile", "keyword" },  { "switch", "keyword" },	  { "char", "keyword2" },
			  { "double", "keyword2" },	  { "auto", "keyword" },	  { "bool", "keyword2" },
			  { "return", "keyword" },	  { "extern", "keyword" },	  { "NULL", "literal" },
			  { "long", "keyword2" },	  { "void", "keyword" },	  { "union", "keyword" },
			  { "short", "keyword2" },	  { "static", "keyword" },	  { "inline", "keyword" },
			  { "false", "literal" },	  { "for", "keyword" },		  { "goto", "keyword" },
			  { "while", "keyword" },	  { "const", "keyword" },	  { "int32_t", "keyword2" },
			  { "int16_t", "keyword2" },  { "uint16_t", "keyword2" }, { "typedef", "keyword" },
			  { "true", "literal" },	  { "else", "keyword" },	  { "uint32_t", "keyword2" },
			  { "enum", "keyword" },	  { "int8_t", "keyword2" },	  { "int64_t", "keyword2" },
			  { "float", "keyword2" },	  { "struct", "keyword" },	  { "break", "keyword" },
			  { "uint8_t", "keyword2" },  { "uint64_t", "keyword2" }, { "case", "keyword" },
			  { "if", "keyword" },		  { "do", "keyword" },

			  { "#if", "keyword" },		  { "#ifdef", "keyword" },	  { "#ifndef", "keyword" },
			  { "#elif", "keyword" },	  { "#else", "keyword" },	  { "#endif", "keyword" },
			  { "#define", "keyword" },	  { "#undef", "keyword" },	  { "#include", "keyword" },
			  { "#line", "keyword" },	  { "#error", "keyword" },	  { "#pragma", "keyword" },
			  { "#warning ", "keyword" }, { "#elifdef ", "keyword" }, { "#elifndef ", "keyword" },
		  },
		  "//",
		  {}

		} );

	sd.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );
}

}}}} // namespace EE::UI::Doc::Language
