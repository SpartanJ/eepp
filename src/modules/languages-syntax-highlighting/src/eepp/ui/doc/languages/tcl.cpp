#include <eepp/ui/doc/languages/tcl.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addTcl() {

	SyntaxDefinitionManager::instance()->add(

		{ "Tcl",
		  { "%.tcl$" },
		  {
			  { { "#.-\n" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "0x%x+" }, "number" },
			  { { "%d+[%d%.eE]*f?" }, "number" },
			  { { "%.?%d+f?" }, "number" },
			  { { "%$[%a_][%w_]*" }, "literal" },
			  { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
			  { { "::[%a_][%w_]*" }, "function" },
			  { { "[%a_][%w_]*%f[:]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },
			  { { "%s+" }, "normal" },
			  { { "%w+%f[%s]" }, "normal" },

		  },
		  {
			  { "elseif", "keyword" },	 { "lsearch", "keyword" }, { "lreverse", "keyword" },
			  { "continue", "keyword" }, { "lset", "keyword" },	   { "finally", "keyword2" },
			  { "on", "keyword2" },		 { "dict", "keyword" },	   { "set", "keyword" },
			  { "lappend", "keyword" },	 { "lrepeat", "keyword" }, { "return", "keyword" },
			  { "error", "keyword2" },	 { "rename", "keyword" },  { "split", "join" },
			  { "try", "keyword2" },	 { "foreach", "keyword" }, { "concat", "keyword" },
			  { "incr", "keyword" },	 { "unset", "keyword" },   { "proc", "keyword" },
			  { "lindex", "keyword" },	 { "for", "keyword" },	   { "array", "keyword" },
			  { "while", "keyword" },	 { "eval", "keyword" },	   { "gets", "keyword" },
			  { "list", "keyword" },	 { "throw", "keyword2" },  { "else", "keyword" },
			  { "package", "keyword" },	 { "linsert", "keyword" }, { "lassign", "keyword" },
			  { "source", "keyword" },	 { "break", "keyword" },   { "puts", "keyword" },
			  { "lreplace", "keyword" }, { "upvar", "keyword" },   { "case", "keyword" },
			  { "llength", "keyword" },	 { "if", "keyword" },	   { "expr", "keyword" },
			  { "lrange", "keyword" },	 { "regexp", "keyword" },  { "lsort", "keyword" },

		  },
		  "#",
		  {}

		} );
}

}}}} // namespace EE::UI::Doc::Language
