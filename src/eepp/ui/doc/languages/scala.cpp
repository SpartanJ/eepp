#include <eepp/ui/doc/languages/scala.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addScala() {

	SyntaxDefinitionManager::instance()->add(

		{ "Scala",
		  { "%.sc$", "%.scala$" },
		  {
			  { { "//.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "[ruU]?\"", "\"", "\\" }, "string" },
			  { { "[ruU]?'", "'", "\\" }, "string" },
			  { { "0x[%da-fA-F]+" }, "number" },
			  { { "-?%d+[%d%.eE]*" }, "number" },
			  { { "-?%.?%d+" }, "number" },
			  { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
			  { { "[%a_][%w_]*\"\"\"*[%a_][%w_]*\"\"\"" }, "string" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "Nothing", "literal" },	   { "AnyRef", "literal" },	  { "new", "keyword" },
			  { "val", "keyword" },		   { "this", "keyword2" },	  { "Array", "keyword2" },
			  { "String", "keyword2" },	   { "Long", "keyword2" },	  { "extends", "keyword" },
			  { "object", "keyword" },	   { "match", "keyword" },	  { "with", "keyword" },
			  { "false", "literal" },	   { "for", "keyword" },	  { "forSome", "keyword" },
			  { "while", "keyword" },	   { "else", "keyword" },	  { "final", "keyword" },
			  { "List", "keyword2" },	   { "if", "keyword" },		  { "println", "keyword" },
			  { "Any", "literal" },		   { "Char", "keyword2" },	  { "Float", "keyword2" },
			  { "private", "keyword2" },   { "finally", "keyword" },  { "abstract", "keyword" },
			  { "Int", "keyword2" },	   { "trait", "keyword" },	  { "return", "keyword" },
			  { "var", "keyword" },		   { "import", "keyword" },	  { "class", "keyword" },
			  { "Byte", "keyword2" },	   { "lazy", "keyword" },	  { "yield", "keyword" },
			  { "Double", "keyword2" },	   { "super", "keyword2" },	  { "Null", "literal" },
			  { "Short", "keyword2" },	   { "throw", "keyword" },	  { "def", "keyword" },
			  { "Try", "keyword" },		   { "Unit", "literal" },	  { "catch", "keyword" },
			  { "true", "literal" },	   { "Boolean", "keyword2" }, { "package", "keyword" },
			  { "override", "keyword" },   { "sealed", "keyword2" },  { "case", "keyword" },
			  { "protected", "keyword2" }, { "do", "keyword" },		  { "implicit", "keyword" },
			  { "type", "keyword" },

		  },
		  "//",
		  {}

		} );
}

}}}} // namespace EE::UI::Doc::Language
