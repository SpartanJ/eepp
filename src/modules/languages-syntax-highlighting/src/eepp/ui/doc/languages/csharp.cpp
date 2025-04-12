#include <eepp/ui/doc/languages/csharp.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addCSharp() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "C#",
		  { "%.cs$", "%.csx$" },
		  {
			  { { "//.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "[%$%@]?\"", "\"", "\\" }, "string" },
			  { { "'\\x%x?%x?%x?%x'" }, "string" },
			  { { "'\\u%x%x%x%x'" }, "string" },
			  { { "'\\?.'" }, "string" },
			  { { "-?0x%x+" }, "number" },
			  { { "-?%d+[%d%.eE]*f?" }, "number" },
			  { { "-?%.?%d+f?" }, "number" },
			  { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
			  { { "%?%?" }, "operator" },
			  { { "%?%." }, "operator" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "internal", "keyword" },  { "int", "keyword2" },	   { "new", "keyword" },
			  { "continue", "keyword" },  { "default", "keyword" },	   { "remove", "keyword" },
			  { "sizeof", "keyword" },	  { "char", "keyword2" },	   { "set", "keyword" },
			  { "bool", "keyword2" },	  { "this", "keyword" },	   { "extern", "keyword" },
			  { "long", "keyword2" },	  { "foreach", "keyword" },	   { "namespace", "keyword" },
			  { "partial", "keyword" },	  { "short", "keyword2" },	   { "typeof", "keyword" },
			  { "object", "keyword2" },	  { "false", "literal" },	   { "for", "keyword" },
			  { "goto", "keyword" },	  { "while", "keyword" },	   { "const", "keyword" },
			  { "get", "keyword" },		  { "delegate", "keyword" },   { "add", "keyword" },
			  { "null", "literal" },	  { "unsafe", "keyword" },	   { "fixed", "keyword" },
			  { "else", "keyword" },	  { "stackalloc", "keyword" }, { "global", "keyword" },
			  { "record", "keyword" },	  { "enum", "keyword" },	   { "value", "keyword" },
			  { "lock", "keyword" },	  { "operator", "keyword" },   { "when", "keyword" },
			  { "struct", "keyword" },	  { "break", "keyword" },	   { "is", "keyword" },
			  { "decimal", "keyword2" },  { "string", "keyword2" },	   { "if", "keyword" },
			  { "await", "keyword" },	  { "in", "keyword" },		   { "public", "keyword" },
			  { "virtual", "keyword" },	  { "dynamic", "keyword" },	   { "private", "keyword" },
			  { "finally", "keyword" },	  { "abstract", "keyword" },   { "unchecked", "keyword" },
			  { "volatile", "keyword" },  { "switch", "keyword" },	   { "using", "keyword" },
			  { "checked", "keyword" },	  { "double", "keyword2" },	   { "event", "keyword" },
			  { "ulong", "keyword2" },	  { "return", "keyword" },	   { "var", "keyword" },
			  { "out", "keyword" },		  { "try", "keyword" },		   { "as", "keyword" },
			  { "class", "keyword" },	  { "void", "keyword" },	   { "byte", "keyword2" },
			  { "yield", "keyword" },	  { "static", "keyword" },	   { "uint", "keyword2" },
			  { "explicit", "keyword" },  { "sbyte", "keyword2" },	   { "throw", "keyword" },
			  { "where", "keyword" },	  { "catch", "keyword" },	   { "true", "literal" },
			  { "ushort", "keyword2" },	  { "override", "keyword" },   { "sealed", "keyword" },
			  { "float", "keyword2" },	  { "params", "keyword" },	   { "readonly", "keyword" },
			  { "base", "keyword" },	  { "case", "keyword" },	   { "interface", "keyword" },
			  { "protected", "keyword" }, { "do", "keyword" },		   { "implicit", "keyword" },
			  { "ref", "keyword" },

		  },
		  "//",
		  {},
		  "csharp" } );

	sd.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );
}

}}}} // namespace EE::UI::Doc::Language
