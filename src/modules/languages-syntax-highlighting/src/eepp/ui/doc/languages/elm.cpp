#include <eepp/ui/doc/languages/elm.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addElm() {

	return SyntaxDefinitionManager::instance()->add(

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
			  { "not", "keyword" },		 { "module", "keyword" }, { "number", "type" },
			  { "Bool", "type" },		 { "of", "keyword" },	  { "if", "keyword" },
			  { "or", "keyword" },		 { "as", "keyword" },	  { "then", "keyword" },
			  { "and", "keyword" },		 { "Int", "type" },		  { "else", "keyword" },
			  { "exposing", "keyword" }, { "False", "literal" },  { "True", "literal" },
			  { "String", "type" },		 { "Float", "type" },	  { "Char", "type" },
			  { "xor", "keyword" },		 { "alias", "keyword" },

		  },
		  "%-%-",
		  {}

		} );
}

}}}} // namespace EE::UI::Doc::Language
