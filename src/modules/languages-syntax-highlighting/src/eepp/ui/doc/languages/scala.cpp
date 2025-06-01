#include <eepp/ui/doc/languages/scala.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addScala() {

	return SyntaxDefinitionManager::instance()->add(

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
			  { "Nothing", "literal" },	 { "AnyRef", "literal" },  { "new", "keyword" },
			  { "val", "keyword" },		 { "this", "type" },	   { "Array", "type" },
			  { "String", "type" },		 { "Long", "type" },	   { "extends", "keyword" },
			  { "object", "keyword" },	 { "match", "keyword" },   { "with", "keyword" },
			  { "false", "literal" },	 { "for", "keyword" },	   { "forSome", "keyword" },
			  { "while", "keyword" },	 { "else", "keyword" },	   { "final", "keyword" },
			  { "List", "type" },		 { "if", "keyword" },	   { "println", "keyword" },
			  { "Any", "literal" },		 { "Char", "type" },	   { "Float", "type" },
			  { "private", "type" },	 { "finally", "keyword" }, { "abstract", "keyword" },
			  { "Int", "type" },		 { "trait", "keyword" },   { "return", "keyword" },
			  { "var", "keyword" },		 { "import", "keyword" },  { "class", "keyword" },
			  { "Byte", "type" },		 { "lazy", "keyword" },	   { "yield", "keyword" },
			  { "Double", "type" },		 { "super", "type" },	   { "Null", "literal" },
			  { "Short", "type" },		 { "throw", "keyword" },   { "def", "keyword" },
			  { "Try", "keyword" },		 { "Unit", "literal" },	   { "catch", "keyword" },
			  { "true", "literal" },	 { "Boolean", "type" },	   { "package", "keyword" },
			  { "override", "keyword" }, { "sealed", "type" },	   { "case", "keyword" },
			  { "protected", "type" },	 { "do", "keyword" },	   { "implicit", "keyword" },
			  { "type", "keyword" },

		  },
		  "//",
		  {}

		} );
}

}}}} // namespace EE::UI::Doc::Language
