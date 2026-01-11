#include <eepp/ui/doc/languages/go.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addOCaml() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "OCaml",
		  { "%.ml$", "%.mli$" },
		  {
			  // Regex's based on those in the OCaml's VSCode plugin.
			  { { "%(%*", "%*%)" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "0[oO_][0-7]+" }, "number" },
			  { { "-?0x[%x_]+" }, "number" },
			  { { "-?%d+_%d" }, "number" },
			  { { "-?%d+[%d%.eE]*f?" }, "number" },
			  { { "-?%.?%d+f?" }, "number" },
			  { { "[$&*+\\-/=>@^%<][~!?$&*+\\-/=>@^|%<:.]*" }, "operator" },
			  { { "\\|[~!?$&*+\\-/=>@^|%<:.]+" }, "operator" },
			  { { "#[~!?$&*+\\-/=>@^|%<:.]+" }, "operator" },
			  { { "![~!?$&*+\\-/=>@^|%<:.]*" }, "operator" },
			  { { "[?~][~!?$&*+\\-/=>@^|%<:.]+" }, "operator" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "and", "keyword" },		  { "as", "keyword" },		 { "asr", "operator" },
			  { "assert", "keyword" },	  { "begin", "keyword" },	 { "class", "keyword" },
			  { "constraint", "keyword" }, { "do", "keyword" },		 { "done", "keyword" },
			  { "downto", "keyword" },	  { "else", "keyword" },	 { "end", "keyword" },
			  { "exception", "keyword" }, { "external", "keyword" }, { "false", "literal" },
			  { "for", "keyword" },		  { "fun", "keyword" },		 { "function", "keyword" },
			  { "functor", "keyword" },	  { "if", "keyword" },		 { "in", "keyword" },
			  { "include", "keyword" },	  { "inherit", "keyword" },	 { "initialized", "keyword" },
			  { "land", "keyword" },	  { "lazy", "keyword" },	 { "let", "keyword" },
			  { "lor", "operator" },	  { "lsl", "operator" },	 { "lsr", "operator" },
			  { "lxor", "operator" },	  { "match", "keyword" },	 { "method", "keyword" },
			  { "mod", "operator" },	  { "module", "keyword" },	 { "open", "keyword" },
			  { "mutable", "keyword" },	  { "new", "keyword" },		 { "nonrec", "keyword" },
			  { "object", "keyword" },	  { "of", "keyword" },		 { "or", "operator" },
			  { "open", "keyword" },	  { "open!", "keyword" },	 { "private", "keyword" },
			  { "then", "keyword" },	  { "rec", "keyword" },		 { "sig", "keyword" },
			  { "struct", "keyword" },	  { "to", "keyword" },		 { "true", "literal" },
			  { "try", "keyword" },		  { "type", "keyword" },	 { "val", "keyword" },
			  { "virtual", "keyword" },	  { "when", "keyword" },	 { "while", "keyword" },
			  { "with", "keyword" },	  { "float", "type" },		 { "bool", "type" },
			  { "int", "type" },		  { "char", "type" },		 { "string", "type" },
			  { "bytes", "type" },		  { "unit", "type" },		 { "array", "parameter" },
			  { "list", "parameter" },	  { "option", "parameter" }, { "result", "parameter" },

		  },
		  "//",
		  {}

		} );

	sd.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '(', ')' } } );
	return sd;
}

}}}} // namespace EE::UI::Doc::Language
