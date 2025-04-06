#include <eepp/ui/doc/languages/blade.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addBlade() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "Blade",
		  { "%.b$" },
		  {
			  { { "#.*" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "%d+%.%d+" }, "number" },
			  { { "0x%x+" }, "number" },
			  { { "0c[0-8]+" }, "number" },
			  { { "0c[01]+" }, "number" },
			  { { "%d+" }, "number" },
			  { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "%a[%w_]+" }, "symbol" },

		  },
		  {
			  { "in", "keyword" },		{ "and", "keyword" },	  { "continue", "keyword" },
			  { "finally", "keyword" }, { "default", "keyword" }, { "using", "keyword" },
			  { "return", "keyword" },	{ "var", "keyword" },	  { "parent", "keyword" },
			  { "import", "keyword" },	{ "try", "keyword" },	  { "as", "keyword" },
			  { "iter", "keyword" },	{ "class", "keyword" },	  { "static", "keyword" },
			  { "echo", "function" },	{ "false", "literal" },	  { "for", "keyword" },
			  { "while", "keyword" },	{ "assert", "keyword" },  { "def", "keyword" },
			  { "self", "keyword2" },	{ "catch", "keyword" },	  { "true", "literal" },
			  { "else", "keyword" },	{ "nil", "literal" },	  { "when", "keyword" },
			  { "break", "keyword" },	{ "die", "keyword" },	  { "or", "keyword" },
			  { "if", "keyword" },		{ "do", "keyword" },

		  },
		  "#",
		  {}

		} );

	sd.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );
}

}}}} // namespace EE::UI::Doc::Language
