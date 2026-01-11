#include <eepp/ui/doc/languages/tcl.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addTcl() {

	return SyntaxDefinitionManager::instance()->add(

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

		  },
		  {
			  { "elseif", "keyword" },	 { "lsearch", "keyword" }, { "lreverse", "keyword" },
			  { "continue", "keyword" }, { "lset", "keyword" },	   { "finally", "type" },
			  { "on", "type" },			 { "dict", "keyword" },	   { "set", "keyword" },
			  { "lappend", "keyword" },	 { "lrepeat", "keyword" }, { "return", "keyword" },
			  { "error", "type" },		 { "rename", "keyword" },  { "split", "join" },
			  { "try", "type" },		 { "foreach", "keyword" }, { "concat", "keyword" },
			  { "incr", "keyword" },	 { "unset", "keyword" },   { "proc", "keyword" },
			  { "lindex", "keyword" },	 { "for", "keyword" },	   { "array", "keyword" },
			  { "while", "keyword" },	 { "eval", "keyword" },	   { "gets", "keyword" },
			  { "list", "keyword" },	 { "throw", "type" },	   { "else", "keyword" },
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
