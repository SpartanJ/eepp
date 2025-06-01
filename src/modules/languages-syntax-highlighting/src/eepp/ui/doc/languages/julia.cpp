#include <eepp/ui/doc/languages/julia.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addJulia() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

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

		  },
		  {
			  { "function", "keyword" }, { "import", "keyword" },	  { "for", "keyword" },
			  { "begin", "keyword" },	 { "local", "keyword" },	  { "const", "keyword" },
			  { "mutable", "type" },	 { "using", "keyword" },	  { "where", "keyword" },
			  { "finally", "keyword" },	 { "Float64", "type" },		  { "UInt8", "type" },
			  { "elseif", "keyword" },	 { "Dict", "keyword" },		  { "global", "keyword" },
			  { "Function", "type" },	 { "Ref", "type" },			  { "try", "keyword" },
			  { "UInt128", "type" },	 { "Vector", "type" },		  { "module", "keyword" },
			  { "Matrix", "type" },		 { "nothing", "literal" },	  { "struct", "type" },
			  { "Number", "type" },		 { "let", "keyword" },		  { "return", "keyword" },
			  { "UInt32", "type" },		 { "Int8", "type" },		  { "Float16", "type" },
			  { "typeof", "keyword" },	 { "if", "keyword" },		  { "type", "keyword" },
			  { "String", "type" },		 { "baremodule", "keyword" }, { "Int128", "type" },
			  { "Char", "type" },		 { "continue", "keyword" },	  { "end", "keyword" },
			  { "while", "keyword" },	 { "quote", "keyword" },	  { "Float32", "type" },
			  { "true", "literal" },	 { "Set", "keyword" },		  { "UInt16", "type" },
			  { "Union", "keyword" },	 { "abstract", "type" },	  { "Inf", "literal" },
			  { "Int16", "type" },		 { "primitive", "type" },	  { "Int64", "type" },
			  { "missing", "literal" },	 { "NaN", "literal" },		  { "Bool", "type" },
			  { "Int32", "type" },		 { "macro", "keyword" },	  { "UInt64", "type" },
			  { "Int", "type" },		 { "catch", "keyword" },	  { "do", "keyword" },
			  { "export", "keyword" },	 { "false", "literal" },	  { "Integer", "type" },
			  { "else", "keyword" },	 { "in", "keyword" },		  { "break", "keyword" },

		  },
		  "#",
		  {}

		} );

	sd.setFoldRangeType( FoldRangeType::Indentation );
	return sd;
}

}}}} // namespace EE::UI::Doc::Language
