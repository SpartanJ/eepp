#include <eepp/ui/doc/languages/fortran.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addFortran() {

	SyntaxDefinitionManager::instance()->add(

		{ "Fortran",
		  { "%.f$", "%.f90$", "%.f95$" },
		  {
			  { { "'", "'", "\\" }, "string" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "!.*" }, "comment" },
			  { { "%.[%a_][%w_]+%." }, "normal" },
			  { { "[!%-/*?:=><+]" }, "operator" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "(program)(%s+)([%a_][%w_]*)" }, { "normal", "keyword", "normal", "literal" } },
			  { { "(module)(%s+)([%a_][%w_]*)" }, { "normal", "keyword", "normal", "literal" } },
			  { { "(use)(%s+)([%a_][%w_]*)" }, { "normal", "keyword", "normal", "literal" } },
			  { { "(struct)(%s+)([%a_][%w_]*)" }, { "normal", "keyword", "normal", "literal" } },
			  { { "-?%d+[%d%.eE_]*" }, "number" },
			  { { "-?%.?%d+" }, "number" },
			  { { "[%a_][%w_]*" }, "normal" },
			  { { "%s+" }, "normal" },
			  { { "%w+%f[%s]" }, "normal" },

		  },
		  {
			  { "then", "keyword" },	  { "public", "keyword" },	   { "allocatable", "keyword" },
			  { "private", "keyword" },	  { "concurrent", "keyword" }, { "only", "keyword" },
			  { "none", "keyword2" },	  { ".or.", "keyword2" },	   { "cycle", "keyword" },
			  { "len", "keyword2" },	  { ".le.", "keyword2" },	   { ".not.", "keyword2" },
			  { "sequence", "keyword" },  { ".false.", "keyword2" },   { ".eq.", "keyword2" },
			  { ".lt.", "keyword2" },	  { "end", "keyword" },		   { "subroutine", "keyword" },
			  { "extends", "keyword" },	  { ".neqv.", "keyword2" },	   { "complex", "keyword" },
			  { "module", "keyword" },	  { "result", "keyword" },	   { "call", "keyword" },
			  { ".true.", "keyword2" },	  { "function", "keyword" },   { "logical", "keyword" },
			  { "else", "keyword" },	  { ".ge", "keyword2" },	   { "parameter", "keyword" },
			  { "contains", "keyword" },  { "print", "keyword" },	   { "write", "keyword" },
			  { ".eqv.", "keyword2" },	  { ".and.", "keyword2" },	   { "struct", "keyword" },
			  { "character", "keyword" }, { "integer", "keyword" },	   { ".ne.", "keyword2" },
			  { "real", "keyword" },	  { "use", "keyword" },		   { "if", "keyword" },
			  { "protected", "keyword" }, { "do", "keyword" },		   { ".gt.", "keyword2" },
			  { "program", "keyword" },	  { "implicit", "keyword" },   { "stop", "keyword" },
			  { "type", "keyword" },

		  },
		  "!",
		  {}

		} );
}

}}}} // namespace EE::UI::Doc::Language
