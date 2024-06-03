#include <eepp/ui/doc/languages/dart.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addDart() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "Dart",
		  { "%.dart$" },
		  {
			  { { "//.-\n" }, "comment" },
			  { { "///.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "-?0x%x+" }, "number" },
			  { { "-?%d+[%d%.eE]*f?" }, "number" },
			  { { "-?%.?%d+f?" }, "number" },
			  { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
			  { { "%?%?" }, "operator" },
			  { { "%?%." }, "operator" },
			  { { "[%$%@]?\"", "\"", "\\" }, "string" },
			  { { "'\\x%x?%x?%x?%x'" }, "string" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "int", "keyword2" },	 { "in", "keyword" },	   { "then", "keyword" },
			  { "dynamic", "keyword2" }, { "new", "keyword" },	   { "continue", "keyword" },
			  { "finally", "keyword" },	 { "default", "keyword" }, { "switch", "keyword" },
			  { "double", "keyword2" },	 { "bool", "keyword2" },   { "this", "keyword2" },
			  { "return", "keyword" },	 { "var", "keyword" },	   { "part of", "keyword" },
			  { "String", "keyword2" },	 { "class", "keyword" },   { "void", "keyword" },
			  { "static", "keyword" },	 { "false", "literal" },   { "for", "keyword" },
			  { "while", "keyword" },	 { "const", "keyword" },   { "null", "literal" },
			  { "true", "literal" },	 { "else", "keyword" },	   { "final", "keyword" },
			  { "enum", "keyword" },	 { "print", "keyword" },   { "Map", "keyword2" },
			  { "break", "keyword" },	 { "List", "keyword2" },   { "Function", "keyword2" },
			  { "case", "keyword" },	 { "if", "keyword" },	   { "await", "keyword" },
			  { "do", "keyword" },

		  },
		  "//",
		  {}

		} );

	sd.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );
}

}}}} // namespace EE::UI::Doc::Language
