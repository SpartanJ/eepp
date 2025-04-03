#include <eepp/ui/doc/languages/flow9.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addFlow9() {

	SyntaxDefinitionManager::instance()->add(

		{ "Flow9",
		  { "%.flow$" },
		  {
			  { { "//.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "\"", "[\"\n]", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "(import)%s+(%a+[%w/_]+)" }, { "normal", "keyword", "literal" } },
			  { { "common_number_parser" }, "number", "", SyntaxPatternMatchType::Parser },
			  { { "[%+%-=/%*%^%%<>!~|&\\]" }, "operator" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "int", "keyword2" },	{ "false", "literal" },	  { "native", "keyword" },
			  { "default", "keyword" }, { "true", "literal" },	  { "else", "keyword" },
			  { "switch", "keyword" },	{ "UInt64", "keyword2" }, { "forbid", "keyword" },
			  { "flow", "keyword2" },	{ "double", "keyword2" }, { "bool", "keyword2" },
			  { "import", "keyword" },	{ "void", "keyword2" },	  { "string", "keyword2" },
			  { "if", "keyword" },		{ "export", "keyword" },  { "unittest", "keyword" },
			  { "io", "keyword2" },		{ "ref", "keyword2" },

		  },
		  "//",
		  {}

		} );
}

}}}} // namespace EE::UI::Doc::Language
