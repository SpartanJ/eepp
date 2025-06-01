#include <eepp/ui/doc/languages/lobster.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addLobster() {

	return SyntaxDefinitionManager::instance()->add(

		{ "Lobster",
		  { "%.lobster$" },
		  {
			  { { "//.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "(struct%s)([%a_][%w_]*)" }, { "normal", "keyword", "type" } },
			  { { "(class%s)([%a_][%w_]*)" }, { "normal", "keyword", "type" } },
			  { { "[%w_]+%s*%f[{]" }, "type" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "\"\"\"", "\"\"\"" }, "string" },
			  { { "0x%x+" }, "number" },
			  { { "%d+[%d%.eE]*f?" }, "number" },
			  { { "%.?%d+f?" }, "number" },
			  { { "[%+%-=/%*%^%%<>!~|&%?]" }, "operator" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "int", "type" },
			  { "and", "keyword" },
			  { "private", "keyword" },
			  { "default", "keyword" },
			  { "abstract", "keyword" },
			  { "static_frame", "keyword" },
			  { "switch", "keyword" },
			  { "guard", "keyword" },
			  { "pakfile", "keyword" },
			  { "attribute", "keyword" },
			  { "return", "keyword" },
			  { "var", "keyword" },
			  { "import", "keyword" },
			  { "class", "keyword" },
			  { "namespace", "keyword" },
			  { "void", "type" },
			  { "static", "keyword" },
			  { "any", "type" },
			  { "typeof", "keyword" },
			  { "enum_flags", "keyword" },
			  { "super", "keyword" },
			  { "member", "keyword" },
			  { "for", "keyword" },
			  { "while", "keyword" },
			  { "from", "keyword" },
			  { "member_frame", "keyword" },
			  { "def", "keyword" },
			  { "else", "keyword" },
			  { "constructor", "keyword" },
			  { "enum", "keyword" },
			  { "operator", "keyword" },
			  { "nil", "literal" },
			  { "not", "keyword" },
			  { "float", "type" },
			  { "struct", "keyword" },
			  { "is", "keyword" },
			  { "fn", "keyword" },
			  { "string", "type" },
			  { "or", "keyword" },
			  { "case", "keyword" },
			  { "if", "keyword" },
			  { "resource", "keyword" },
			  { "let", "keyword" },
			  { "program", "keyword" },

		  },
		  "//",
		  {}

		} );
}

}}}} // namespace EE::UI::Doc::Language
