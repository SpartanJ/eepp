#include <eepp/ui/doc/languages/python.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addPython() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "Python",
		  { "%.py$", "%.pyw$", "%.bry$", "^SConstruct$" },
		  {
			  { { "#", "\n" }, "comment" },
			  { { "[ruU]?\"", "\"", "\\" }, "string" },
			  { { "[ruU]?'", "'", "\\" }, "string" },
			  { { "\"\"\"", "\"\"\"" }, "string" },
			  { { "0x[%da-fA-F]+" }, "number" },
			  { { "-?%d+[%d%.eE]*" }, "number" },
			  { { "-?%.?%d+" }, "number" },
			  { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "in", "keyword" },		{ "and", "keyword" },	{ "continue", "keyword" },
			  { "finally", "keyword" }, { "False", "literal" }, { "True", "literal" },
			  { "lambda", "keyword" },	{ "None", "literal" },	{ "del", "keyword" },
			  { "return", "keyword" },	{ "pass", "keyword" },	{ "import", "keyword" },
			  { "try", "keyword" },		{ "as", "keyword" },	{ "class", "keyword" },
			  { "raise", "keyword" },	{ "yield", "keyword" }, { "with", "keyword" },
			  { "for", "keyword" },		{ "while", "keyword" }, { "assert", "keyword" },
			  { "from", "keyword" },	{ "elif", "keyword" },	{ "def", "keyword" },
			  { "self", "type" },	{ "else", "keyword" },	{ "global", "keyword" },
			  { "not", "keyword" },		{ "break", "keyword" }, { "nonlocal", "keyword" },
			  { "is", "keyword" },		{ "or", "keyword" },	{ "if", "keyword" },
			  { "except", "keyword" },

		  },
		  "#",
		  { "^#!.*[ /]python", "^#!.*[ /]python3" }

		} );

	sd.setFoldRangeType( FoldRangeType::Indentation );
	sd.setBlockComment( { "\"\"\"", "\"\"\"" } );
}

}}}} // namespace EE::UI::Doc::Language
