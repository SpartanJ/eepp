#include <eepp/ui/doc/languages/elm.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addElm() {

	SyntaxDefinitionManager::instance()->add(

		{ "Elm",
		  { "%.elm$" },
		  {
			  { { "%-%-", "\n" }, "comment" },
			  { { "{%-", "%-}" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "\"\"\"", "\"\"\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "-?0x%x+" }, "number" },
			  { { "-?%d+[%d%.eE]*f?" }, "number" },
			  { { "-?%.?%d+f?" }, "number" },
			  { { "%.%." }, "operator" },
			  { { "[=:|&<>%+%-%*\\/%^%%]" }, "operator" },
			  { { "[%a_'][%w_']*" }, "symbol" },

		  },
		  {
			  { "import", "keyword" },	 { "port", "keyword" },	  { "type", "keyword" },
			  { "case", "keyword" },	 { "in", "keyword" },	  { "let", "keyword" },
			  { "not", "keyword" },		 { "module", "keyword" }, { "number", "keyword2" },
			  { "Bool", "keyword2" },	 { "of", "keyword" },	  { "if", "keyword" },
			  { "or", "keyword" },		 { "as", "keyword" },	  { "then", "keyword" },
			  { "and", "keyword" },		 { "Int", "keyword2" },	  { "else", "keyword" },
			  { "exposing", "keyword" }, { "False", "literal" },  { "True", "literal" },
			  { "String", "keyword2" },	 { "Float", "keyword2" }, { "Char", "keyword2" },
			  { "xor", "keyword" },		 { "alias", "keyword" },

		  },
		  "%-%-",
		  {}

		} );
}

}}}} // namespace EE::UI::Doc::Language
