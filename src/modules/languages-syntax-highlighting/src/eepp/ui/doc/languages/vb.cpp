#include <eepp/ui/doc/languages/vb.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addVisualBasic() {

	return SyntaxDefinitionManager::instance()
		->add(

			{ "Visual Basic",
			  { "%.bas$", "%.cls$", "%.ctl$", "%.dob$", "%.dsm$", "%.dsr$", "%.frm$", "%.pag$",
				"%.vb$", "%.vba$", "%.vbs$" },
			  {
				  { { "'.*" }, "comment" },
				  { { "REM%s.*" }, "comment" },
				  { { "\"", "\"" }, "string" },
				  { { "-?&H%x+" }, "number" },
				  { { "-?%d+[%d%.eE]*" }, "number" },
				  { { "-?%.?%d+f?" }, "number" },
				  { { "[=><%+%-%*%^&:%.,_%(%)]" }, "operator" },
				  { { "[%a_][%w_]*" }, "symbol" },

			  },
			  {
				  { "then", "keyword" },	 { "new", "keyword" },		 { "continue", "keyword" },
				  { "on", "keyword" },		 { "default", "keyword" },	 { "begin", "keyword" },
				  { "char", "type" },		 { "set", "keyword" },		 { "wend", "keyword" },
				  { "byref", "keyword" },	 { "loop", "keyword" },		 { "attribute", "keyword" },
				  { "long", "type" },		 { "execute", "keyword" },	 { "xor", "keyword" },
				  { "end", "keyword" },		 { "short", "type" },		 { "date", "type" },
				  { "object", "type" },		 { "variant", "type" },		 { "sub", "keyword" },
				  { "with", "keyword" },	 { "false", "keyword" },	 { "for", "keyword" },
				  { "goto", "keyword" },	 { "while", "keyword" },	 { "const", "keyword" },
				  { "get", "keyword" },		 { "lib", "keyword" },		 { "call", "keyword" },
				  { "null", "keyword" },	 { "select", "keyword" },	 { "function", "keyword" },
				  { "else", "keyword" },	 { "next", "keyword" },		 { "option", "keyword" },
				  { "enum", "keyword" },	 { "is", "keyword" },		 { "declare", "keyword" },
				  { "decimal", "type" },	 { "string", "type" },		 { "or", "keyword" },
				  { "until", "keyword" },	 { "if", "keyword" },		 { "nothing", "keyword" },
				  { "let", "keyword" },		 { "property", "keyword" },	 { "step", "keyword" },
				  { "dim", "keyword" },		 { "elseif", "keyword" },	 { "in", "keyword" },
				  { "single", "type" },		 { "exit", "keyword" },		 { "public", "keyword" },
				  { "and", "keyword" },		 { "private", "keyword" },	 { "each", "keyword" },
				  { "redim", "keyword" },	 { "preserve", "keyword" },	 { "byval", "keyword" },
				  { "double", "type" },		 { "return", "keyword" },	 { "error", "keyword" },
				  { "as", "keyword" },		 { "class", "keyword" },	 { "byte", "type" },
				  { "module", "keyword" },	 { "randomize", "keyword" }, { "boolean", "type" },
				  { "explicit", "keyword" }, { "true", "keyword" },		 { "not", "keyword" },
				  { "integer", "type" },	 { "case", "keyword" },		 { "mod", "keyword" },
				  { "do", "keyword" },		 { "empty", "keyword" },	 { "type", "keyword" },
				  { "to", "keyword" },
			  },
			  "'",
			  {}

			} )
		.setExtensionPriority( true )
		.setCaseInsensitive( true );
}

}}}} // namespace EE::UI::Doc::Language
