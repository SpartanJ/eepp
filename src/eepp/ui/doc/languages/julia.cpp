#include <eepp/ui/doc/languages/julia.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addJulia() {

	SyntaxDefinitionManager::instance()->add(

		{ "Julia",
		  { "%.jl$" },
		  {
			  { { "#=", "=#" }, "comment" },
			  { { "#.*$" }, "comment" },
			  { { "icxx\"\"\"", "\"\"\"" }, "string", "C++" },
			  { { "cxx\"\"\"", "\"\"\"" }, "string", "C++" },
			  { { "py\"\"\"", "\"\"\"" }, "string", "Python" },
			  { { "js\"\"\"", "\"\"\"" }, "string", "JavaScript" },
			  { { "md\"\"\"", "\"\"\"" }, "string", "Markdown" },
			  { { "%d%w*[%.-+*//]" }, "number" },
			  { { "0[oO_][0-7]+" }, "number" },
			  { { "-?0x[%x_]+" }, "number" },
			  { { "-?0b[%x_]+" }, "number" },
			  { { "-?%d+_%d" }, "number" },
			  { { "-?%d+[%d%.eE]*f?" }, "number" },
			  { { "-?%.?%d+f?" }, "number" },
			  { { "[^%d%g]%:%a*" }, "function" },
			  { { "[%+%-=/%*%^%%<>!~|&%:]" }, "operator" },
			  { { "\"\"\".*\"\"\"" }, "string" },
			  { { "\".*\"" }, "string" },
			  { { "[bv]\".*\"" }, "string" },
			  { { "r\".*$" }, "string" },
			  { { "'\\.*'" }, "string" },
			  { { "'.'" }, "string" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "%g*!" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },
			  { { "%s+" }, "normal" },
			  { { "%w+%f[%s]" }, "normal" },

		  },
		  {
			  { "function", "keyword" },  { "import", "keyword" },	   { "for", "keyword" },
			  { "begin", "keyword" },	  { "local", "keyword" },	   { "const", "keyword" },
			  { "mutable", "keyword2" },  { "using", "keyword" },	   { "where", "keyword" },
			  { "finally", "keyword" },	  { "Float64", "keyword2" },   { "UInt8", "keyword2" },
			  { "elseif", "keyword" },	  { "Dict", "keyword" },	   { "global", "keyword" },
			  { "Function", "keyword2" }, { "Ref", "keyword2" },	   { "try", "keyword" },
			  { "UInt128", "keyword2" },  { "Vector", "keyword2" },	   { "module", "keyword" },
			  { "Matrix", "keyword2" },	  { "nothing", "literal" },	   { "struct", "keyword2" },
			  { "Number", "keyword2" },	  { "let", "keyword" },		   { "return", "keyword" },
			  { "UInt32", "keyword2" },	  { "Int8", "keyword2" },	   { "Float16", "keyword2" },
			  { "typeof", "keyword" },	  { "if", "keyword" },		   { "type", "keyword" },
			  { "String", "keyword2" },	  { "baremodule", "keyword" }, { "Int128", "keyword2" },
			  { "Char", "keyword2" },	  { "continue", "keyword" },   { "end", "keyword" },
			  { "while", "keyword" },	  { "quote", "keyword" },	   { "Float32", "keyword2" },
			  { "true", "literal" },	  { "Set", "keyword" },		   { "UInt16", "keyword2" },
			  { "Union", "keyword" },	  { "abstract", "keyword2" },  { "Inf", "literal" },
			  { "Int16", "keyword2" },	  { "primitive", "keyword2" }, { "Int64", "keyword2" },
			  { "missing", "literal" },	  { "NaN", "literal" },		   { "Bool", "keyword2" },
			  { "Int32", "keyword2" },	  { "macro", "keyword" },	   { "UInt64", "keyword2" },
			  { "Int", "keyword2" },	  { "catch", "keyword" },	   { "do", "keyword" },
			  { "export", "keyword" },	  { "false", "literal" },	   { "Integer", "keyword2" },
			  { "else", "keyword" },	  { "in", "keyword" },		   { "break", "keyword" },

		  },
		  "#",
		  {}

		} );
}

}}}} // namespace EE::UI::Doc::Language
