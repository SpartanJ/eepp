#include <eepp/ui/doc/languages/objective-c.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addObjectiveC() {

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
			  { { "@[%a_][%w_]*" }, "keyword2" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "elseif", "keyword" },	  { "int", "keyword2" },	 { "then", "keyword" },
			  { "unsigned", "keyword2" }, { "continue", "keyword" }, { "default", "keyword" },
			  { "volatile", "keyword" },  { "switch", "keyword" },	 { "char", "keyword2" },
			  { "double", "keyword2" },	  { "auto", "keyword" },	 { "bool", "keyword2" },
			  { "return", "keyword" },	  { "extern", "keyword" },	 { "NULL", "literal" },
			  { "long", "keyword2" },	  { "void", "keyword" },	 { "union", "keyword" },
			  { "short", "keyword2" },	  { "static", "keyword" },	 { "inline", "keyword" },
			  { "false", "literal" },	  { "for", "keyword" },		 { "goto", "keyword" },
			  { "while", "keyword" },	  { "const", "keyword" },	 { "typedef", "keyword" },
			  { "true", "literal" },	  { "else", "keyword" },	 { "enum", "keyword" },
			  { "nil", "literal" },		  { "float", "keyword2" },	 { "struct", "keyword" },
			  { "break", "keyword" },	  { "case", "keyword" },	 { "if", "keyword" },
			  { "do", "keyword" },

		  },
		  "//",
		  {}

		} );

	sd.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );
}

}}}} // namespace EE::UI::Doc::Language
