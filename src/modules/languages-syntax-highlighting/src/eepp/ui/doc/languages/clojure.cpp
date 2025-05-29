#include <eepp/ui/doc/languages/clojure.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addClojure() {

	return SyntaxDefinitionManager::instance()->add(

		{ "Clojure",
		  { "%.clj$", "%.cljs$", "%.clc$", "%.edn$" },
		  {
			  { { ";;.*" }, "comment" },
			  { { ";.*" }, "comment" },
			  { { "#\"", "\"", "\\" }, "string" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "\"\"\"", "\"\"\"", "\\" }, "string" },
			  { { "(Retention)(%s+)([%a_][%w_/]*)" },
				{ "normal", "keyword", "normal", "literal" } },
			  { { ":[%a_][%w_/%-]*" }, "keyword2" },
			  { { "([%a_][%w_]*)(%.)([%a_][%w_/%-]*)" },
				{ "normal", "keyword", "operator", "keyword2" } },
			  { { "(%()(def)(%s+)([%a_][%w_%-]*)" },
				{ "normal", "normal", "keyword", "literal", "literal" } },
			  { { "(%()(def[%a_][%w_]*)(%s+)([%a_][%w_%-]*)" },
				{ "normal", "normal", "keyword", "literal", "literal" } },
			  { { "(%()(require)(%s+)([%a_][%w_]*)" },
				{ "normal", "normal", "keyword", "literal", "literal" } },
			  { { "(%()([%a_][%w_/]*)" }, { "normal", "normal", "literal" } },
			  { { "-?0x%x+" }, "number" },
			  { { "-?%d+[%d%.eE]*f?" }, "number" },
			  { { "-?%.?%d+f?" }, "number" },
			  { { "[!%#%$%%&*+./%<=>%?@\\%^|%-~:]" }, "operator" },
			  { { "[%a_'][%w_']*" }, "normal" },

		  },
		  {
			  { "int", "literal" },		 { "str", "keyword" },	  { "false", "keyword2" },
			  { "get", "keyword" },		 { "def", "keyword" },	  { "Deprecated", "keyword" },
			  { "catch", "keyword" },	 { "true", "keyword2" },  { "defn", "keyword" },
			  { "apply", "keyword" },	 { "vector", "keyword" }, { "nil", "literal" },
			  { "require", "keyword2" }, { "try", "keyword" },	  { "String", "keyword" },
			  { "fn", "keyword" },		 { "ns", "keyword" },	  { "cond", "keyword" },
			  { "if", "keyword" },		 { "let", "keyword" },	  { "Retention", "keyword" },
			  { "println", "keyword" },

		  },
		  ";;",
		  {}

		} );
}

}}}} // namespace EE::UI::Doc::Language
