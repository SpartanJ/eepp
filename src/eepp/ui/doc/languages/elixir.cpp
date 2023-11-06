#include <eepp/ui/doc/languages/elixir.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addElixir() {

	SyntaxDefinitionManager::instance()->add(

		{ "Elixir",
		  { "%.ex$", "%.exs$" },
		  {
			  { { "#.*\n" }, "comment" },
			  { { ":\"", "\"", "\\" }, "number" },
			  { { "\"\"\"", "\"\"\"", "\\" }, "string" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "~%a\"\"\"", "\"\"\"" }, "string" },
			  { { "~%a[/\"|'%(%[%{<]", "[/\"|'%)%]%}>]", "\\" }, "string" },
			  { { "-?0x%x+" }, "number" },
			  { { "-?%d+[%d%.eE]*f?" }, "number" },
			  { { "-?%.?%d+f?" }, "number" },
			  { { ":\"?[%a_][%w_]*\"?" }, "number" },
			  { { "[%a][%w_!?]*%f[(]" }, "function" },
			  { { "%u%w+" }, "normal" },
			  { { "@[%a_][%w_]*" }, "keyword2" },
			  { { "_%a[%w_]*" }, "keyword2" },
			  { { "[%+%-=/%*<>!|&]" }, "operator" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "raise", "keyword" },		 { "use", "keyword2" },
			  { "try", "keyword" },			 { "defrecordp", "keyword" },
			  { "unless", "keyword" },		 { "for", "keyword" },
			  { "receive", "keyword" },		 { "or", "operator" },
			  { "if", "keyword" },			 { "fn", "keyword" },
			  { "when", "keyword" },		 { "after", "keyword" },
			  { "case", "keyword" },		 { "and", "operator" },
			  { "rescue", "keyword" },		 { "defcallback", "keyword" },
			  { "defexception", "keyword" }, { "cond", "keyword" },
			  { "defdelegate", "keyword" },	 { "else", "keyword" },
			  { "end", "keyword" },			 { "quote", "keyword" },
			  { "super", "keyword" },		 { "do", "keyword" },
			  { "require", "keyword2" },	 { "unquote_splicing", "keyword" },
			  { "alias", "keyword2" },		 { "nil", "literal" },
			  { "true", "literal" },		 { "unquote", "keyword" },
			  { "defmacro", "keyword" },	 { "def", "keyword" },
			  { "defrecord", "keyword" },	 { "false", "literal" },
			  { "defp", "keyword" },		 { "defoverridable", "keyword" },
			  { "defprotocol", "keyword" },	 { "with", "keyword" },
			  { "defmodule", "keyword" },	 { "defguardp", "keyword" },
			  { "defmacrop", "keyword" },	 { "defimpl", "keyword" },
			  { "defstruct", "keyword" },	 { "catch", "keyword" },
			  { "import", "keyword2" },		 { "defguard", "keyword" },

		  },
		  "#",
		  {}

		} );
}

}}}} // namespace EE::UI::Doc::Language
