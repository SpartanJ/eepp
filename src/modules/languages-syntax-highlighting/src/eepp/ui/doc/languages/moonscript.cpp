#include <eepp/ui/doc/languages/moonscript.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addMoonscript() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "MoonScript",
		  { "%.moon$" },
		  {
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "%[%[", "%]%]" }, "string" },
			  { { "%-%-.-\n" }, "comment" },
			  { { "-?0x%x+" }, "number" },
			  { { "-?%d+[%d%.eE]*" }, "number" },
			  { { "-?%.?%d+" }, "number" },
			  { { "%.%.%.?" }, "keyword2" },
			  { { "[<>~=]=" }, "keyword2" },
			  { { "[%+%-=/%*%^%%#<>]" }, "keyword2" },
			  { { "[%a_][%w_]*%s*%f[(\"{]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },
			  { { "\\", "[%a_][%w_]*" }, "function" },
			  { { "%.", "[%a_][%w_]*" }, "function" },
			  { { "@", "[%a_][%w_]*" }, "keyword2" },
			  { { "!" }, "keyword2" },
			  { { "[%p]" }, "keyword" },

		  },
		  {
			  { "elseif", "keyword" }, { "in", "keyword" },		  { "then", "keyword" },
			  { "and", "keyword" },	   { "continue", "keyword" }, { "unless", "keyword" },
			  { "switch", "keyword" }, { "using", "keyword" },	  { "return", "keyword" },
			  { "import", "keyword" }, { "as", "keyword" },		  { "class", "keyword" },
			  { "super", "keyword2" }, { "extends", "keyword" },  { "with", "keyword" },
			  { "false", "literal" },  { "for", "keyword" },	  { "while", "keyword" },
			  { "from", "keyword" },   { "self", "keyword2" },	  { "true", "literal" },
			  { "else", "keyword" },   { "nil", "literal" },	  { "not", "keyword" },
			  { "when", "keyword" },   { "break", "keyword" },	  { "#", "keyword2" },
			  { "or", "keyword" },	   { "if", "keyword" },		  { "export", "keyword" },
			  { "->", "keyword" },	   { "do", "keyword" },

		  },
		  "--",
		  {}

		} );

	sd.setFoldRangeType( FoldRangeType::Indentation );
	return sd;
}

}}}} // namespace EE::UI::Doc::Language
