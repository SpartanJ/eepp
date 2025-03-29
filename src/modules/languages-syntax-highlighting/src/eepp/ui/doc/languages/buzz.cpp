#include <eepp/ui/doc/languages/buzz.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addBuzz() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "Buzz",
		  { "%.buzz$" },
		  {
			  { { "\"", "\"", "\\" }, "string" },
			  { { "|.*" }, "comment" },
			  { { "[!%-/*?:=><]" }, "operator" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "(const)(%s+)([%a_][%w_]*)" }, { "normal", "keyword", "normal", "literal" } },
			  { { "(object)(%s+)([%a_][%w_]*)" }, { "normal", "keyword", "normal", "literal" } },
			  { { "(var)(%s+)([%a_][%w_]*)" }, { "normal", "keyword", "normal", "literal" } },
			  { { "-?%d+[%d%.eE_]*" }, "number" },
			  { { "-?%.?%d+" }, "number" },
			  { { "[%a_][%w_]*" }, "normal" },

		  },
		  {
			  { "int", "keyword" },		 { "str", "keyword" },		 { "in", "literal" },
			  { "resume", "keyword" },	 { "and", "keyword" },		 { "continue", "keyword" },
			  { "ud", "keyword" },		 { "lambda", "keyword" },	 { "bool", "keyword" },
			  { "this", "keyword2" },	 { "std", "keyword2" },		 { "return", "keyword" },
			  { "var", "keyword" },		 { "extern", "keyword" },	 { "import", "keyword" },
			  { "try", "keyword" },		 { "foreach", "keyword" },	 { "as", "keyword" },
			  { "pat", "keyword" },		 { "namespace", "keyword" }, { "void", "keyword" },
			  { "yield", "keyword" },	 { "static", "keyword2" },	 { "any", "keyword" },
			  { "typeof", "keyword" },	 { "io", "keyword2" },		 { "object", "keyword" },
			  { "match", "keyword" },	 { "false", "literal" },	 { "for", "keyword" },
			  { "test", "keyword" },	 { "while", "keyword" },	 { "const", "keyword" },
			  { "null", "literal" },	 { "zdef", "keyword" },		 { "catch", "keyword" },
			  { "true", "literal" },	 { "else", "keyword" },		 { "global", "keyword" },
			  { "enum", "keyword" },	 { "not", "keyword2" },		 { "float", "keyword" },
			  { "resolve", "keyword" },	 { "is", "keyword" },		 { "or", "keyword" },
			  { "protocol", "keyword" }, { "if", "keyword" },		 { "export", "keyword" },
			  { "do", "keyword" },		 { "fun", "keyword" },		 { "type", "keyword" },

		  },
		  "|",
		  {}

		} );

	sd.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );
}

}}}} // namespace EE::UI::Doc::Language
