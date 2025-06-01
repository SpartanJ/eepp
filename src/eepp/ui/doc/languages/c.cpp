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
			  { { "#?[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "elseif", "keyword" },	  { "int", "type" },	  { "then", "keyword" },
			  { "unsigned", "type" }, { "continue", "keyword" },  { "default", "keyword" },
			  { "volatile", "keyword" },  { "switch", "keyword" },	  { "char", "type" },
			  { "double", "type" },	  { "auto", "keyword" },	  { "bool", "type" },
			  { "return", "keyword" },	  { "extern", "keyword" },	  { "NULL", "literal" },
			  { "long", "type" },	  { "void", "keyword" },	  { "union", "keyword" },
			  { "short", "type" },	  { "static", "keyword" },	  { "inline", "keyword" },
			  { "false", "literal" },	  { "for", "keyword" },		  { "goto", "keyword" },
			  { "while", "keyword" },	  { "const", "keyword" },	  { "int32_t", "type" },
			  { "int16_t", "type" },  { "uint16_t", "type" }, { "typedef", "keyword" },
			  { "true", "literal" },	  { "else", "keyword" },	  { "uint32_t", "type" },
			  { "enum", "keyword" },	  { "int8_t", "type" },	  { "int64_t", "type" },
			  { "float", "type" },	  { "struct", "keyword" },	  { "break", "keyword" },
			  { "uint8_t", "type" },  { "uint64_t", "type" }, { "case", "keyword" },
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
