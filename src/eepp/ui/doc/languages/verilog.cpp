#include <eepp/ui/doc/languages/verilog.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addVerilog() {

	SyntaxDefinitionManager::instance()->add(

		{ "Verilog",
		  { "%.V$", "%.vl$", "%.vh$" },
		  {
			  { { "\"", "\"", "\\" }, "string" },
			  { { "-?0x%x+" }, "number" },
			  { { "-?%d+[%d%.eE]*f?" }, "number" },
			  { { "-?%.?%d+f?" }, "number" },
			  { { "[%a_][%w_!?]*%f[(]" }, "function" },
			  { { "[%+%-=/%*<>!|&]" }, "operator" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "xnor", "keyword" },		 { "wor", "keyword" },
			  { "weak1", "keyword" },		 { "tri0", "keyword" },
			  { "tri", "keyword" },			 { "tranif0", "keyword" },
			  { "tran", "keyword" },		 { "supply1", "keyword" },
			  { "tranif1", "keyword" },		 { "supply0", "keyword" },
			  { "trior", "keyword" },		 { "strong0", "keyword" },
			  { "specpa", "keyword" },		 { "time", "keyword" },
			  { "specify", "keyword" },		 { "small", "keyword" },
			  { "signed", "keyword" },		 { "rtranif1", "keyword" },
			  { "rnmos", "keyword" },		 { "repeat", "keyword" },
			  { "release", "keyword" },		 { "reg", "keyword" },
			  { "pull1", "keyword" },		 { "posedge", "keyword" },
			  { "pmos", "keyword" },		 { "endtable", "keyword" },
			  { "wait", "keyword" },		 { "edge", "keyword" },
			  { "endprimitive", "keyword" }, { "endmodule", "keyword" },
			  { "endfunction", "keyword" },	 { "rcmos", "keyword" },
			  { "or", "keyword" },			 { "rpmos", "keyword" },
			  { "nmos", "keyword" },		 { "end", "keyword" },
			  { "defparam", "keyword" },	 { "bufif0", "keyword" },
			  { "larger", "keyword" },		 { "strong1", "keyword" },
			  { "disable", "keyword" },		 { "triand", "keyword" },
			  { "join", "keyword" },		 { "nor", "keyword" },
			  { "endspecify", "keyword" },	 { "while", "keyword" },
			  { "realtime", "keyword" },	 { "always", "keyword" },
			  { "deassign", "keyword" },	 { "event", "keyword" },
			  { "else", "keyword" },		 { "assign", "keyword" },
			  { "wire", "keyword" },		 { "table", "keyword" },
			  { "for", "keyword" },			 { "begin", "keyword" },
			  { "automatic", "keyword" },	 { "scalared", "keyword" },
			  { "real", "keyword" },		 { "endgenerate", "keyword" },
			  { "and", "keyword" },			 { "vectored", "keyword" },
			  { "primitive", "keyword" },	 { "case", "keyword" },
			  { "bufif1", "keyword" },		 { "cmos", "keyword" },
			  { "trireg", "keyword" },		 { "generate", "keyword" },
			  { "weak0", "keyword" },		 { "task", "keyword" },
			  { "casex", "keyword" },		 { "buf", "keyword" },
			  { "casez", "keyword" },		 { "force", "keyword" },
			  { "wand", "keyword" },		 { "forever", "keyword" },
			  { "inout", "keyword" },		 { "default", "keyword" },
			  { "fork", "keyword" },		 { "pull0", "keyword" },
			  { "function", "keyword" },	 { "genvar", "keyword" },
			  { "highz0", "keyword" },		 { "parameter", "keyword" },
			  { "highz1", "keyword" },		 { "module", "keyword" },
			  { "tri1", "keyword" },		 { "ifnone", "keyword" },
			  { "initial", "keyword" },		 { "not", "keyword" },
			  { "notif0", "keyword" },		 { "input", "keyword" },
			  { "localparam", "keyword" },	 { "rtranif0", "keyword" },
			  { "endcase", "keyword" },		 { "nand", "keyword" },
			  { "rtran", "keyword" },		 { "integer", "keyword" },
			  { "macromodule", "keyword" },	 { "xor", "keyword" },
			  { "medium", "keyword" },		 { "negedge", "keyword" },
			  { "notif1", "keyword" },		 { "endtask", "keyword" },
			  { "if", "keyword" },			 { "output", "keyword" },

		  },
		  "//",
		  {}

		} );
}

}}}} // namespace EE::UI::Doc::Language
