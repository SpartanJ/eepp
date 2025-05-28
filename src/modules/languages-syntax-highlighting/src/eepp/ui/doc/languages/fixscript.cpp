#include <eepp/ui/doc/languages/fixscript.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addFixScript() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "FixScript",
		  { "%.fix$" },
		  {
			  { { "//.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "common_number_parser" }, "number", "", SyntaxPatternMatchType::Parser },
			  { { "(class)%s+([@%a_][%w_]+)(%s*%:%s*)([%a_][%w_]+)" },
				{ "keyword", "keyword", "keyword2", "operator", "keyword2" } },
			  { { "(class)%s+([@%a_][%w_]+)" }, { "keyword", "keyword", "keyword2" } },
			  { { "(struct)%s+([@%a_][%w_]+)" }, { "keyword", "keyword", "keyword2" } },
			  { { "([@%a_][%w_]+)::([%a_][%w_]+)%f[(]" }, { "operator", "keyword2", "function" } },
			  { { "[%+%-=/%*%^%%<>!~|&@]" }, "operator" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {

			  { "do", "keyword" },		  { "if", "keyword" },			{ "for", "keyword" },
			  { "use", "keyword" },		  { "var", "keyword" },			{ "case", "keyword" },
			  { "else", "keyword" },	  { "break", "keyword" },		{ "const", "keyword" },
			  { "while", "keyword" },	  { "import", "keyword" },		{ "return", "keyword" },
			  { "switch", "keyword" },	  { "default", "keyword" },		{ "continue", "keyword" },
			  { "function", "keyword" },

			  { "macro", "keyword" },	  { "generate", "keyword" },	{ "output", "keyword" },

			  { "foreach", "keyword" },	  { "class", "keyword" },		{ "struct", "keyword" },
			  { "static", "keyword" },	  { "virtual", "keyword" },		{ "override", "keyword" },
			  { "new", "keyword" },		  { "in", "keyword" },			{ "as", "keyword" },
			  { "throw", "keyword" },	  { "constructor", "keyword" }, { "operator", "keyword" },
			  { "super", "keyword" },	  { "this", "keyword" },

			  { "optional", "keyword" },  { "transaction", "keyword" },

			  { "Dynamic", "keyword2" },  { "Void", "keyword2" },		{ "Byte", "keyword2" },
			  { "Short", "keyword2" },	  { "Integer", "keyword2" },	{ "Float", "keyword2" },
			  { "Boolean", "keyword2" },  { "String", "keyword2" },		{ "Long", "keyword2" },
			  { "Double", "keyword2" },

			  { "true", "literal" },	  { "false", "literal" },		{ "null", "literal" },

		  },
		  "//",
		  {}

		} );

	sd.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );
}

}}}} // namespace EE::UI::Doc::Language
