#include <eepp/ui/doc/languages/meson.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addMeson() {

	return SyntaxDefinitionManager::instance()->add(

		{ "Meson",
		  { "meson%.build$", "^meson_options%.txt$" },
		  {
			  { { "#", "\n" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "'''", "'''" }, "string" },
			  { { "0x[%da-fA-F]+" }, "number" },
			  { { "-?%d+%d*" }, "number" },
			  { { "[%+%-=/%%%*!]" }, "operator" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "false", "literal" },
			  { "in", "keyword" },
			  { "then", "keyword" },
			  { "and", "keyword" },
			  { "continue", "keyword" },
			  { "elif", "keyword" },
			  { "true", "literal" },
			  { "else", "keyword" },
			  { "not", "keyword" },
			  { "break", "keyword" },
			  { "foreach", "keyword" },
			  { "endif", "keyword" },
			  { "or", "keyword" },
			  { "if", "keyword" },
			  { "endforeach", "keyword" },

		  },
		  "#",
		  {}

		} );
}

}}}} // namespace EE::UI::Doc::Language
