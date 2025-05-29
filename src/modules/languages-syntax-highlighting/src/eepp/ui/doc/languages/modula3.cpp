#include <eepp/ui/doc/languages/modula3.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addModula3() {

	return SyntaxDefinitionManager::instance()->add(

		{ "Modula3",
		  { "%.m3$", "%.i3$", "%.mg$", "%.ig$" },
		  {
			  { { "\"", "\"", "\\" }, "string" },
			  { { "%(%*", "%*%)" }, "comment" },
			  { { "<%*", "%*>" }, "comment" },
			  { { "[%:%;%=%<%>%&%+%-%*%/%.%(%)]" }, "operator" },
			  { { "-?%d+[%d%.eE_]*" }, "number" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {

			  { "AND", "keyword" },		  { "DO", "keyword" },
			  { "FROM", "keyword" },	  { "NOT", "keyword" },
			  { "REPEAT", "keyword" },	  { "UNTIL", "keyword" },
			  { "ANY", "keyword" },		  { "ELSE", "keyword" },
			  { "GENERIC", "keyword" },	  { "OBJECT", "keyword" },
			  { "RETURN", "keyword" },	  { "UNTRACED", "keyword" },
			  { "ARRAY", "keyword" },	  { "ELSIF", "keyword" },
			  { "IF", "keyword" },		  { "OF", "keyword" },
			  { "REVEAL", "keyword" },	  { "VALUE", "keyword" },
			  { "AS", "keyword" },		  { "END", "keyword" },
			  { "IMPORT", "keyword" },	  { "OR", "keyword" },
			  { "ROOT", "keyword" },	  { "VAR", "keyword" },
			  { "BEGIN", "keyword" },	  { "EVAL", "keyword" },
			  { "IN", "keyword" },		  { "OVERRIDES", "keyword" },
			  { "SET", "keyword" },		  { "WHILE", "keyword" },
			  { "BITS", "keyword" },	  { "EXCEPT", "keyword" },
			  { "INTERFACE", "keyword" }, { "PROCEDURE", "keyword" },
			  { "THEN", "keyword" },	  { "WITH", "keyword" },
			  { "BRANDED", "keyword" },	  { "EXCEPTION", "keyword" },
			  { "LOCK", "keyword" },	  { "RAISE", "keyword" },
			  { "TO", "keyword" },		  { "BY", "keyword" },
			  { "EXIT", "keyword" },	  { "LOOP", "keyword" },
			  { "RAISES", "keyword" },	  { "TRY", "keyword" },
			  { "CASE", "keyword" },	  { "EXPORTS", "keyword" },
			  { "METHODS", "keyword" },	  { "READONLY", "keyword" },
			  { "TYPE", "keyword" },	  { "CONST", "keyword" },
			  { "FINALLY", "keyword" },	  { "MOD", "keyword" },
			  { "RECORD", "keyword" },	  { "TYPECASE", "keyword" },
			  { "DIV", "keyword" },		  { "FOR", "keyword" },
			  { "MODULE", "keyword" },	  { "REF", "keyword" },
			  { "UNSAFE", "keyword" },	  { "ABS", "function" },
			  { "BYTESIZE", "function" }, { "EXTENDED", "function" },
			  { "MAX", "function" },	  { "SUBARRAY", "function" },
			  { "ADDRESS", "function" },  { "ISTYPE", "function" },
			  { "MIN", "function" },	  { "NUMBER", "function" },
			  { "TEXT", "function" },	  { "ADR", "function" },
			  { "CEILING", "function" },  { "FIRST", "function" },
			  { "LAST", "function" },	  { "MUTEX", "function" },
			  { "ORD", "function" },	  { "ADRSIZE", "function" },
			  { "NARROW", "function" },	  { "TRUNC", "function" },
			  { "BITSIZE", "function" },  { "DEC", "function" },
			  { "FLOOR", "function" },	  { "NEW", "function" },
			  { "REFANY", "function" },	  { "TYPECODE", "function" },
			  { "DISPOSE", "function" },  { "INC", "function" },
			  { "LOOPHOLE", "function" }, { "VAL", "function" },
			  { "BITSET", "keyword2" },	  { "BOOLEAN", "keyword2" },
			  { "CARDINAL", "keyword2" }, { "CHAR", "keyword2" },
			  { "INTEGER", "keyword2" },  { "REAL", "keyword2" },
			  { "FLOAT", "keyword2" },	  { "LONGINT", "keyword2" },
			  { "LONGREAL", "keyword2" }, { "BOOLEAN", "keyword2" },
			  { "WIDECHAR", "keyword2" }, { "TRUE", "literal" },
			  { "FALSE", "literal" },	  { "NIL", "literal" },
			  { "NULL", "literal" }

		  },
		  "--",
		  {}

		} );
}

}}}} // namespace EE::UI::Doc::Language
