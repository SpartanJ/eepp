#include <eepp/ui/doc/languages/modula3.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addModula3() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

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

			  { "and", "keyword" },		  { "do", "keyword" },
			  { "from", "keyword" },	  { "not", "keyword" },
			  { "repeat", "keyword" },	  { "until", "keyword" },
			  { "any", "keyword" },		  { "else", "keyword" },
			  { "generic", "keyword" },	  { "object", "keyword" },
			  { "return", "keyword" },	  { "untraced", "keyword" },
			  { "array", "keyword" },	  { "elsif", "keyword" },
			  { "if", "keyword" },		  { "of", "keyword" },
			  { "reveal", "keyword" },	  { "value", "keyword" },
			  { "as", "keyword" },		  { "end", "keyword" },
			  { "import", "keyword" },	  { "or", "keyword" },
			  { "root", "keyword" },	  { "var", "keyword" },
			  { "begin", "keyword" },	  { "eval", "keyword" },
			  { "in", "keyword" },		  { "overrides", "keyword" },
			  { "set", "keyword" },		  { "while", "keyword" },
			  { "bits", "keyword" },	  { "except", "keyword" },
			  { "interface", "keyword" }, { "procedure", "keyword" },
			  { "then", "keyword" },	  { "with", "keyword" },
			  { "branded", "keyword" },	  { "exception", "keyword" },
			  { "lock", "keyword" },	  { "raise", "keyword" },
			  { "to", "keyword" },		  { "by", "keyword" },
			  { "exit", "keyword" },	  { "loop", "keyword" },
			  { "raises", "keyword" },	  { "try", "keyword" },
			  { "case", "keyword" },	  { "exports", "keyword" },
			  { "methods", "keyword" },	  { "readonly", "keyword" },
			  { "type", "keyword" },	  { "const", "keyword" },
			  { "finally", "keyword" },	  { "mod", "keyword" },
			  { "record", "keyword" },	  { "typecase", "keyword" },
			  { "div", "keyword" },		  { "for", "keyword" },
			  { "module", "keyword" },	  { "ref", "keyword" },
			  { "unsafe", "keyword" },	  { "abs", "function" },
			  { "bytesize", "function" }, { "extended", "function" },
			  { "max", "function" },	  { "subarray", "function" },
			  { "address", "function" },  { "istype", "function" },
			  { "min", "function" },	  { "number", "function" },
			  { "text", "function" },	  { "adr", "function" },
			  { "ceiling", "function" },  { "first", "function" },
			  { "last", "function" },	  { "mutex", "function" },
			  { "ord", "function" },	  { "adrsize", "function" },
			  { "narrow", "function" },	  { "trunc", "function" },
			  { "bitsize", "function" },  { "dec", "function" },
			  { "floor", "function" },	  { "new", "function" },
			  { "refany", "function" },	  { "typecode", "function" },
			  { "dispose", "function" },  { "inc", "function" },
			  { "loophole", "function" }, { "val", "function" },
			  { "bitset", "keyword2" },	  { "boolean", "keyword2" },
			  { "cardinal", "keyword2" }, { "char", "keyword2" },
			  { "integer", "keyword2" },  { "real", "keyword2" },
			  { "float", "keyword2" },	  { "longint", "keyword2" },
			  { "longreal", "keyword2" }, { "boolean", "keyword2" },
			  { "widechar", "keyword2" }, { "true", "literal" },
			  { "false", "literal" },	  { "nil", "literal" },
			  { "null", "literal" }

		  },
		  "--",
		  {}

		} );

	sd.setCaseInsensitive( true );
}

}}}} // namespace EE::UI::Doc::Language
