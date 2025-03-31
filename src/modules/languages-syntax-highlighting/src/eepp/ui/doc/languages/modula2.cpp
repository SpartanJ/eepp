#include <eepp/ui/doc/languages/modula2.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addModula2() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

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

			  { "array", "keyword" },
			  { "begin", "keyword" },
			  { "by", "keyword" },
			  { "case", "keyword" },
			  { "const", "keyword" },
			  { "definition", "keyword" },
			  { "do", "keyword" },
			  { "else", "keyword" },
			  { "elsif", "keyword" },
			  { "end", "keyword" },
			  { "except", "keyword" },
			  { "exit", "keyword" },
			  { "export", "keyword" },
			  { "finally", "keyword" },
			  { "for", "keyword" },
			  { "forward", "keyword" },
			  { "from", "keyword" },
			  { "if", "keyword" },
			  { "implementation", "keyword" },
			  { "import", "keyword" },
			  { "in", "keyword" },
			  { "loop", "keyword" },
			  { "module", "keyword" },
			  { "of", "keyword" },
			  { "packedset", "keyword" },
			  { "pointer", "keyword" },
			  { "procedure", "keyword" },
			  { "qualified", "keyword" },
			  { "record", "keyword" },
			  { "repeat", "keyword" },
			  { "retry", "keyword" },
			  { "return", "keyword" },
			  { "set", "keyword" },
			  { "then", "keyword" },
			  { "to", "keyword" },
			  { "type", "keyword" },
			  { "until", "keyword" },
			  { "var", "keyword" },
			  { "while", "keyword" },
			  { "with", "keyword" },
			  { "abs", "function" },
			  { "adr", "function" },
			  { "ash", "function" },
			  { "and", "function" },
			  { "cap", "function" },
			  { "dec", "function" },
			  { "dispose", "function" },
			  { "div", "function" },
			  { "excl", "function" },
			  { "float", "function" },
			  { "inc", "function" },
			  { "incl", "function" },
			  { "halt", "function" },
			  { "high", "function" },
			  { "new", "function" },
			  { "mod", "function" },
			  { "not", "function" },
			  { "odd", "function" },
			  { "or", "function" },
			  { "proc", "function" },
			  { "round", "function" },
			  { "size", "function" },
			  { "tsize", "function" },
			  { "bitset", "keyword2" },
			  { "boolean", "keyword2" },
			  { "cardinal", "keyword2" },
			  { "char", "keyword2" },
			  { "integer", "keyword2" },
			  { "real", "keyword2" },
			  { "true", "literal" },
			  { "false", "literal" },
			  { "nil", "literal" },
			  { "func", "keyword" },

		  },
		  "--",
		  {}

		} );

	sd.setCaseInsensitive( true );
}

}}}} // namespace EE::UI::Doc::Language
