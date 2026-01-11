#include <eepp/ui/doc/languages/curry.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addCurry() {

	return SyntaxDefinitionManager::instance()->add(

		{ "Curry",
		  { "%.curry$" },
		  {
			  { { "%-%-", "\n" }, "comment" },
			  { { "{%-", "%-}" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "-?0x%x+" }, "number" },
			  { { "-?%d+[%d%.eE]*f?" }, "number" },
			  { { "-?%.?%d+f?" }, "number" },
			  { { "[!%#%$%%&*+./%<=>%?@\\%^|%-~:]" }, "operator" },
			  { { "[%a_'][%w_']*" }, "symbol" },

		  },
		  {
			  { "in", "keyword" },		 { "then", "keyword" },	  { "default", "keyword" },
			  { "mdo", "keyword" },		 { "forall", "keyword" }, { "hiding", "keyword" },
			  { "data", "keyword" },	 { "import", "keyword" }, { "as", "keyword" },
			  { "class", "keyword" },	 { "infixl", "keyword" }, { "newtype", "keyword" },
			  { "of", "keyword" },		 { "module", "keyword" }, { "infix", "keyword" },
			  { "deriving", "keyword" }, { "where", "keyword" },  { "else", "keyword" },
			  { "foreign", "keyword" },	 { "infixr", "keyword" }, { "qualified", "keyword" },
			  { "case", "keyword" },	 { "if", "keyword" },	  { "let", "keyword" },
			  { "do", "keyword" },		 { "type", "keyword" },

		  },
		  "--",
		  {}

		} ).setBlockComment( { "{-", "-}" } );
}

}}}} // namespace EE::UI::Doc::Language
