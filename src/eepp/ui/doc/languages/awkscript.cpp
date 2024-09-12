#include <eepp/ui/doc/languages/awkscript.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addAwkScript() {

	SyntaxDefinitionManager::instance()->add(

		{ "Awk Script",
		  { "%.awk$" },
		  {
			  { { "%$[%a_@*#][%w_]*" }, "keyword2" },
			  { { "#.*" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "`", "`", "\\" }, "string" },
			  { { "%f[%w_%.%/]%d[%d%.]*%f[^%w_%.]" }, "number" },
			  { { "[!<>|&%[%]:=*]" }, "operator" },
			  { { "%f[%S][%+%-][%w%-_:]+" }, "function" },
			  { { "%f[%S][%+%-][%w%-_]+%f[=]" }, "function" },
			  { { "(%s%-%a[%w_%-]*)(%s+)(%d[%d%.]+)" },
				{ "normal", "function", "normal", "number" } },
			  { { "(%s%-%a[%w_%-]*)(%s+)(%a[%a%-_:=]+)" },
				{ "normal", "function", "normal", "symbol" } },
			  { { "[_%a][%w_]+%f[%+=]" }, "keyword2" },
			  { { "%${.-}" }, "keyword2" },
			  { { "%$[%d%$%a_@*][%w_]*" }, "keyword2" },
			  { { "([%a_%-][%w_%-]*)(%s*%f[(])" }, { "normal", "function", "normal" } },
			  { { "[%a_][%w_]*" }, "symbol" },
			  { { "%s+" }, "normal" },
			  { { "%w+%f[%s]" }, "normal" },

		  },
		  {
			  { "SUBSEP", "keyword" },	   { "int", "keyword" },
			  { "NF", "keyword" },		   { "RS", "keyword" },
			  { "continue", "keyword" },   { "OFMT", "keyword" },
			  { "CONVFMT", "keyword" },	   { "exp", "keyword" },
			  { "ARGV", "keyword" },	   { "BEGIN", "keyword" },
			  { "ERRNO", "keyword" },	   { "FNR", "keyword" },
			  { "nextfile", "keyword" },   { "printf", "keyword" },
			  { "sub", "keyword" },		   { "match", "keyword" },
			  { "for", "keyword" },		   { "while", "keyword" },
			  { "RLENGTH", "keyword" },	   { "function", "keyword" },
			  { "else", "keyword" },	   { "next", "keyword" },
			  { "print", "keyword" },	   { "toupper", "keyword" },
			  { "RT", "keyword" },		   { "srand", "keyword" },
			  { "tolower", "keyword" },	   { "break", "keyword" },
			  { "sin", "keyword" },		   { "if", "keyword" },
			  { "IGNORECASE", "keyword" }, { "ARGC", "keyword" },
			  { "sprintf", "keyword" },	   { "exit", "keyword" },
			  { "atan2", "keyword" },	   { "TEXTDOMAIN", "keyword" },
			  { "rand", "keyword" },	   { "FIELDWIDTHS", "keyword" },
			  { "OFS", "keyword" },		   { "delete", "keyword" },
			  { "NR", "keyword" },		   { "return", "keyword" },
			  { "RSTART", "keyword" },	   { "split", "keyword" },
			  { "END", "keyword" },		   { "ORS", "keyword" },
			  { "length", "keyword" },	   { "LINT", "keyword" },
			  { "ENVIRON", "keyword" },	   { "BINMODE", "keyword" },
			  { "FILENAME", "keyword" },   { "getline", "keyword" },
			  { "FS", "keyword" },		   { "substr", "keyword" },
			  { "sqrt", "keyword" },	   { "PROCINFO", "keyword" },
			  { "cos", "keyword" },		   { "index", "keyword" },
			  { "ARGIND", "keyword" },	   { "log", "keyword" },
			  { "gsub", "keyword" },	   { "do", "keyword" },

		  },
		  "#",
		  {}

		} );
}

}}}} // namespace EE::UI::Doc::Language
