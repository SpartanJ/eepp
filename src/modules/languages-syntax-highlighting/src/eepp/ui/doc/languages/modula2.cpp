#include <eepp/ui/doc/languages/modula2.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addModula2() {

	SyntaxDefinitionManager::instance()->add(

		{ "Modula2",
		  { "%.def$", "%.mod$" },
		  {
			  { { "\"", "\"", "\\" }, "string" },
			  { { "%(%*", "%*%)" }, "comment" },
			  { { "[%:%;%=%<%>%&%+%-%*%/%.%(%)]" }, "operator" },
			  { { "-?%d+[%d%.eE_]*" }, "number" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {

			  { "ARRAY", "keyword" },
			  { "BEGIN", "keyword" },
			  { "BY", "keyword" },
			  { "CASE", "keyword" },
			  { "CONST", "keyword" },
			  { "DEFINITION", "keyword" },
			  { "DO", "keyword" },
			  { "ELSE", "keyword" },
			  { "ELSIF", "keyword" },
			  { "END", "keyword" },
			  { "EXCEPT", "keyword" },
			  { "EXIT", "keyword" },
			  { "EXPORT", "keyword" },
			  { "FINALLY", "keyword" },
			  { "FOR", "keyword" },
			  { "FORWARD", "keyword" },
			  { "FROM", "keyword" },
			  { "IF", "keyword" },
			  { "IMPLEMENTATION", "keyword" },
			  { "IMPORT", "keyword" },
			  { "IN", "keyword" },
			  { "LOOP", "keyword" },
			  { "MODULE", "keyword" },
			  { "OF", "keyword" },
			  { "PACKEDSET", "keyword" },
			  { "POINTER", "keyword" },
			  { "PROCEDURE", "keyword" },
			  { "QUALIFIED", "keyword" },
			  { "RECORD", "keyword" },
			  { "REPEAT", "keyword" },
			  { "RETRY", "keyword" },
			  { "RETURN", "keyword" },
			  { "SET", "keyword" },
			  { "THEN", "keyword" },
			  { "TO", "keyword" },
			  { "TYPE", "keyword" },
			  { "UNTIL", "keyword" },
			  { "VAR", "keyword" },
			  { "WHILE", "keyword" },
			  { "WITH", "keyword" },
			  { "ABS", "function" },
			  { "ADR", "function" },
			  { "ASH", "function" },
			  { "AND", "function" },
			  { "CAP", "function" },
			  { "DEC", "function" },
			  { "DISPOSE", "function" },
			  { "DIV", "function" },
			  { "EXCL", "function" },
			  { "FLOAT", "function" },
			  { "INC", "function" },
			  { "INCL", "function" },
			  { "HALT", "function" },
			  { "HIGH", "function" },
			  { "NEW", "function" },
			  { "MOD", "function" },
			  { "NOT", "function" },
			  { "ODD", "function" },
			  { "OR", "function" },
			  { "PROC", "function" },
			  { "ROUND", "function" },
			  { "SIZE", "function" },
			  { "TSIZE", "function" },
			  { "BITSET", "keyword2" },
			  { "BOOLEAN", "keyword2" },
			  { "CARDINAL", "keyword2" },
			  { "CHAR", "keyword2" },
			  { "INTEGER", "keyword2" },
			  { "REAL", "keyword2" },
			  { "TRUE", "literal" },
			  { "FALSE", "literal" },
			  { "NIL", "literal" },
			  { "FUNC", "keyword" },

		  },
		  "--",
		  {}

		} );

}

}}}} // namespace EE::UI::Doc::Language
