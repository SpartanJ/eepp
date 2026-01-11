#include <eepp/ui/doc/languages/starlark.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addStarlark() {

	return SyntaxDefinitionManager::instance()->add(

		{ "Starlark",
		  { "%.bazel$", "%.bzl$", "BUILD" },
		  {
			  { { "\"", "\"", "\\" }, "string" },
			  { { "#.*" }, "comment" },
			  { { "[!%-/*?:=><]" }, "operator" },
			  { { "-?%d+[%d%.eE_]*" }, "number" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "int", "literal" },		 { "str", "literal" },		 { "in", "type" },
			  { "continue", "keyword" }, { "enumerate", "literal" }, { "finally", "type" },
			  { "False", "keyword" },	 { "getattr", "literal" },	 { "True", "keyword" },
			  { "len", "literal" },		 { "dict", "literal" },		 { "lambda", "type" },
			  { "bool", "literal" },	 { "del", "type" },			 { "return", "keyword" },
			  { "pass", "keyword" },	 { "import", "type" },		 { "max", "literal" },
			  { "try", "type" },		 { "min", "literal" },		 { "as", "type" },
			  { "class", "type" },		 { "raise", "type" },		 { "reversed", "literal" },
			  { "yield", "type" },		 { "any", "literal" },		 { "sorted", "literal" },
			  { "hasattr", "literal" },	 { "load", "literal" },		 { "with", "type" },
			  { "for", "keyword" },		 { "while", "type" },		 { "assert", "type" },
			  { "tuple", "literal" },	 { "from", "type" },		 { "elif", "keyword" },
			  { "list", "literal" },	 { "else", "keyword" },		 { "global", "type" },
			  { "repr", "literal" },	 { "all", "literal" },		 { "break", "keyword" },
			  { "dir", "literal" },		 { "nonlocal", "type" },	 { "is", "type" },
			  { "if", "keyword" },		 { "except", "type" },		 { "hash", "literal" },
			  { "zip", "literal" },		 { "type", "literal" },

		  },
		  "#",
		  {}

		} );
}

}}}} // namespace EE::UI::Doc::Language
