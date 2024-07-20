#include <eepp/ui/doc/languages/bend.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addBend() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "Bend",
		  { "%.bend$" },
		  {
			  { { "#.*" }, "comment" },
			  { { "\"", "\"" }, "string" },
			  { { "'", "'" }, "string" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },
			  { { "[%+%-=/%*%^%%<>!|&]" }, "operator" },
			  { { "(def)(%s+)([%a_][%w_]*)" }, { "normal", "keyword", "normal", "literal" } },
			  { { "(data)(%s+)([%a_][%w_]*)" }, { "normal", "keyword", "normal", "literal" } },
			  { { "(let)(%s+)([%a_][%w_]*)" }, { "normal", "keyword", "normal", "literal" } },
			  { { "(Some)(%s+)([%a_][%w_]*)" }, { "normal", "keyword", "normal", "literal" } },
			  { { "(bend)(%s+)([%a_][%w_]*)" }, { "normal", "keyword", "normal", "literal" } },
			  { { "(object)(%s+)([%a_][%w_]*)" }, { "normal", "keyword", "normal", "literal" } },
			  { { "(fold)(%s+)([%a_][%w_]*)" }, { "normal", "keyword", "normal", "literal" } },
			  { { "(open)(%s+)([%a_][%w_]*)" }, { "normal", "keyword", "normal", "literal" } },
			  { { "(do)(%s+)([%a_][%w_]*)" }, { "normal", "keyword", "normal", "literal" } },
			  { { "(identity)(%s+)([%a_][%w_]*)" }, { "normal", "keyword", "normal", "literal" } },
			  { { "(lambda)(%s+)([%a_][%w_]*)" }, { "normal", "keyword", "normal", "literal" } },
			  { { "0x[%da-fA-F]+" }, "number" },
			  { { "-?%d+[%d%.eE]*" }, "number" },
			  { { "-?%.?%d+" }, "number" },
			  { { "%s+" }, "normal" },
			  { { "%w+%f[%s]" }, "normal" },

		  },
		  {
			  { "Nil", "keyword2" },   { "Name", "keyword" },	  { "bind", "keyword" },
			  { "switch", "keyword" }, { "data", "keyword" },	  { "None", "keyword2" },
			  { "return", "keyword" }, { "identity", "keyword" }, { "ask", "keyword" },
			  { "object", "keyword" }, { "match", "keyword" },	  { "with", "keyword" },
			  { "fold", "keyword" },   { "false", "literal" },	  { "def", "keyword" },
			  { "true", "literal" },   { "else", "keyword" },	  { "λ", "keyword" },
			  { "Bool", "keyword" },   { "when", "keyword" },	  { "bend", "keyword2" },
			  { "use", "keyword" },	   { "case", "keyword" },	  { "if", "keyword" },
			  { "let", "keyword" },	   { "Result", "keyword2" },  { "do", "keyword" },
			  { "open", "keyword" },   { "Some", "keyword" },	  { "type", "keyword2" },

		  },
		  "#",
		  {}

		} );

	sd.setFoldRangeType( FoldRangeType::Indentation );
}

}}}} // namespace EE::UI::Doc::Language
