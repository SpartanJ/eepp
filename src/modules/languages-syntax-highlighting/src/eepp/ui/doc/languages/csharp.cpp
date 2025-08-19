#include <eepp/ui/doc/languages/csharp.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addCSharp() {

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
			  { "internal", "keyword" },  { "int", "type" },		   { "new", "keyword" },
			  { "continue", "keyword" },  { "default", "keyword" },	   { "remove", "keyword" },
			  { "sizeof", "keyword" },	  { "char", "type" },		   { "set", "keyword" },
			  { "bool", "type" },		  { "this", "keyword" },	   { "extern", "keyword" },
			  { "long", "type" },		  { "foreach", "keyword" },	   { "namespace", "keyword" },
			  { "partial", "keyword" },	  { "short", "type" },		   { "typeof", "keyword" },
			  { "object", "type" },		  { "false", "literal" },	   { "for", "keyword" },
			  { "goto", "keyword" },	  { "while", "keyword" },	   { "const", "keyword" },
			  { "get", "keyword" },		  { "delegate", "keyword" },   { "add", "keyword" },
			  { "null", "literal" },	  { "unsafe", "keyword" },	   { "fixed", "keyword" },
			  { "else", "keyword" },	  { "stackalloc", "keyword" }, { "global", "keyword" },
			  { "record", "keyword" },	  { "enum", "keyword" },	   { "value", "keyword" },
			  { "lock", "keyword" },	  { "operator", "keyword" },   { "when", "keyword" },
			  { "struct", "keyword" },	  { "break", "keyword" },	   { "is", "keyword" },
			  { "decimal", "type" },	  { "string", "type" },		   { "if", "keyword" },
			  { "await", "keyword" },	  { "in", "keyword" },		   { "public", "keyword" },
			  { "virtual", "keyword" },	  { "dynamic", "keyword" },	   { "private", "keyword" },
			  { "finally", "keyword" },	  { "abstract", "keyword" },   { "unchecked", "keyword" },
			  { "volatile", "keyword" },  { "switch", "keyword" },	   { "using", "keyword" },
			  { "checked", "keyword" },	  { "double", "type" },		   { "event", "keyword" },
			  { "ulong", "type" },		  { "return", "keyword" },	   { "var", "keyword" },
			  { "out", "keyword" },		  { "try", "keyword" },		   { "as", "keyword" },
			  { "class", "keyword" },	  { "void", "keyword" },	   { "byte", "type" },
			  { "yield", "keyword" },	  { "static", "keyword" },	   { "uint", "type" },
			  { "explicit", "keyword" },  { "sbyte", "type" },		   { "throw", "keyword" },
			  { "where", "keyword" },	  { "catch", "keyword" },	   { "true", "literal" },
			  { "ushort", "type" },		  { "override", "keyword" },   { "sealed", "keyword" },
			  { "float", "type" },		  { "params", "keyword" },	   { "readonly", "keyword" },
			  { "base", "keyword" },	  { "case", "keyword" },	   { "interface", "keyword" },
			  { "protected", "keyword" }, { "do", "keyword" },		   { "implicit", "keyword" },
			  { "ref", "keyword" },

		  },
		  "//",
		  {},
		  "csharp" } );

	sd.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );
	sd.setBlockComment( { "/*", "*/" } );
	return sd;
}

}}}} // namespace EE::UI::Doc::Language
