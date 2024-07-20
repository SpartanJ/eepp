#include <eepp/ui/doc/languages/lobster.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addLobster() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "Lobster",
		  { "%.lobster$" },
		  {
			  { { "//.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "(struct%s)([%a_][%w_]*)" }, { "normal", "keyword", "keyword2" } },
			  { { "(class%s)([%a_][%w_]*)" }, { "normal", "keyword", "keyword2" } },
			  { { "import%s+from" }, "keyword" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "\"\"\"", "\"\"\"" }, "string" },
			  { { "0x%x+" }, "number" },
			  { { "%d+[%d%.eE]*f?" }, "number" },
			  { { "%.?%d+f?" }, "number" },
			  { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },
			  { { "%s+" }, "normal" },
			  { { "%w+%f[%s]" }, "normal" },

		  },
		  {
			  { "int", "keyword2" },	  { "and", "operator" },	   { "private", "keyword" },
			  { "default", "keyword" },	  { "switch", "keyword" },	   { "pakfile", "keyword" },
			  { "return", "keyword" },	  { "var", "keyword" },		   { "import", "keyword" },
			  { "namespace", "keyword" }, { "void", "keyword2" },	   { "any", "keyword2" },
			  { "typeof", "keyword" },	  { "enum_flags", "keyword" }, { "for", "keyword" },
			  { "while", "keyword" },	  { "def", "keyword" },		   { "else", "keyword" },
			  { "enum", "keyword" },	  { "nil", "literal" },		   { "not", "operator" },
			  { "float", "keyword2" },	  { "is", "keyword" },		   { "fn", "keyword" },
			  { "string", "keyword2" },	  { "or", "operator" },		   { "case", "keyword" },
			  { "if", "keyword" },		  { "resource", "keyword" },   { "let", "keyword" },
			  { "program", "keyword" },

		  },
		  "//",
		  {}

		} );

	sd.setFoldRangeType( FoldRangeType::Indentation );
}

}}}} // namespace EE::UI::Doc::Language
