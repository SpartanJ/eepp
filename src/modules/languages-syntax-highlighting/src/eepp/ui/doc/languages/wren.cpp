#include <eepp/ui/doc/languages/wren.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addWren() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "Wren",
		  { "%.wren$" },
		  {
			  { { "//.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "-?%.?%d+" }, "number" },
			  { { "%.%.%.?" }, "operator" },
			  { { "[<>!=]=" }, "operator" },
			  { { "[%+%-=/%*%^%%<>!~|&?:]" }, "operator" },
			  { { "[%a_][%w_]*%s*%f[(\"{]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "false", "literal" },  { "in", "keyword" },		   { "for", "keyword" },
			  { "while", "keyword" },  { "null", "literal" },	   { "true", "literal" },
			  { "else", "keyword" },   { "construct", "keyword" }, { "foreign", "keyword" },
			  { "this", "keyword2" },  { "return", "keyword" },	   { "var", "keyword" },
			  { "import", "keyword" }, { "break", "keyword" },	   { "class", "keyword" },
			  { "is", "keyword" },	   { "if", "keyword" },		   { "static", "keyword" },
			  { "super", "keyword" },

		  },
		  "//",
		  {}

		} );

	sd.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );
	return sd;
}

}}}} // namespace EE::UI::Doc::Language
