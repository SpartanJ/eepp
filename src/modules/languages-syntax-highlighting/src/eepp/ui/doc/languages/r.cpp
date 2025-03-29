#include <eepp/ui/doc/languages/r.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addR() {

	SyntaxDefinitionManager::instance()->add(

		{ "R",
		  { "%.r$", "%.rds$", "%.rda$", "%.rdata$", "%.R$" },
		  {
			  { { "#", "\n" }, "comment" },
			  { { "\"", "\"" }, "string" },
			  { { "'", "'" }, "string" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },
			  { { "[%+%-=/%*%^%%<>!|&]" }, "operator" },
			  { { "0x[%da-fA-F]+" }, "number" },
			  { { "-?%d+[%d%.eE]*" }, "number" },
			  { { "-?%.?%d+" }, "number" },

		  },
		  {
			  { "NULL", "literal" },
			  { "else", "keyword" },
			  { "NA_real", "keyword" },
			  { "NA_complex", "keyword" },
			  { "while", "keyword" },
			  { "repeat", "keyword" },
			  { "function", "keyword" },
			  { "NA", "literal" },
			  { "NA_integer", "keyword" },
			  { "for", "keyword" },
			  { "Inf", "literal" },
			  { "TRUE", "literal" },
			  { "next", "keyword" },
			  { "in", "keyword" },
			  { "if", "keyword" },
			  { "FALSE", "literal" },
			  { "NA_character", "keyword" },
			  { "break", "keyword" },

		  },
		  "#",
		  {}

		} );
}

}}}} // namespace EE::UI::Doc::Language
