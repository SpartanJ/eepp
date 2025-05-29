#include <eepp/ui/doc/languages/bazel.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addBazel() {

	return SyntaxDefinitionManager::instance()->add(

		{ "Bazel",
		  { "%.bazel$", "%.bzl$", "BUILD" },
		  {
			  { { "\"", "\"", "\\" }, "string" },
			  { { "#.*" }, "comment" },
			  { { "[!%-/*?:=><]" }, "operator" },
			  { { "-?%d+[%d%.eE_]*" }, "number" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "-?%d+[%d%.eE_]*" }, "number" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "int", "literal" },		 { "str", "literal" },		 { "in", "keyword2" },
			  { "continue", "keyword" }, { "enumerate", "literal" }, { "finally", "keyword2" },
			  { "False", "keyword" },	 { "getattr", "literal" },	 { "True", "keyword" },
			  { "len", "literal" },		 { "dict", "literal" },		 { "lambda", "keyword2" },
			  { "bool", "literal" },	 { "del", "keyword2" },		 { "return", "keyword" },
			  { "pass", "keyword" },	 { "import", "keyword2" },	 { "max", "literal" },
			  { "try", "keyword2" },	 { "min", "literal" },		 { "as", "keyword2" },
			  { "class", "keyword2" },	 { "raise", "keyword2" },	 { "reversed", "literal" },
			  { "yield", "keyword2" },	 { "any", "literal" },		 { "sorted", "literal" },
			  { "hasattr", "literal" },	 { "load", "literal" },		 { "with", "keyword2" },
			  { "for", "keyword" },		 { "while", "keyword2" },	 { "assert", "keyword2" },
			  { "tuple", "literal" },	 { "from", "keyword2" },	 { "elif", "keyword" },
			  { "list", "literal" },	 { "else", "keyword" },		 { "global", "keyword2" },
			  { "repr", "literal" },	 { "all", "literal" },		 { "break", "keyword" },
			  { "dir", "literal" },		 { "nonlocal", "keyword2" }, { "is", "keyword2" },
			  { "if", "keyword" },		 { "except", "keyword2" },	 { "hash", "literal" },
			  { "zip", "literal" },		 { "type", "literal" },

		  },
		  "#",
		  {}

		} );
}

}}}} // namespace EE::UI::Doc::Language
