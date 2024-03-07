#include <eepp/ui/doc/languages/vb.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addVisualBasic() {

	SyntaxDefinitionManager::instance()->add(

		{ "Visual Basic",
		  { "%.bas$", "%.cls$", "%.ctl$", "%.dob$", "%.dsm$", "%.dsr$", "%.frm$", "%.pag$", "%.vb$",
			"%.vba$", "%.vbs$" },
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

			  { "If", "keyword" },		 { "Then", "keyword" },		 { "Else", "keyword" },
			  { "ElseIf", "keyword" },	 { "While", "keyword" },	 { "Wend", "keyword" },
			  { "For", "keyword" },		 { "To", "keyword" },		 { "Each", "keyword" },
			  { "In", "keyword" },		 { "Step", "keyword" },		 { "Case", "keyword" },
			  { "Select", "keyword" },	 { "Return", "keyword" },	 { "Continue", "keyword" },
			  { "Do", "keyword" },		 { "Until", "keyword" },	 { "Loop", "keyword" },
			  { "Next", "keyword" },	 { "With", "keyword" },		 { "Exit", "keyword" },
			  { "Mod", "keyword" },		 { "And", "keyword" },		 { "Not", "keyword" },
			  { "Or", "keyword" },		 { "Xor", "keyword" },		 { "Is", "keyword" },
			  { "Call", "keyword" },	 { "Class", "keyword" },	 { "Const", "keyword" },
			  { "Enum", "keyword" },	 { "ENUM", "keyword" },		 { "Dim", "keyword" },
			  { "ReDim", "keyword" },	 { "Preserve", "keyword" },	 { "Function", "keyword" },
			  { "Sub", "keyword" },		 { "Property", "keyword" },	 { "End", "keyword" },
			  { "Set", "keyword" },		 { "Let", "keyword" },		 { "Get", "keyword" },
			  { "New", "keyword" },		 { "Randomize", "keyword" }, { "Option", "keyword" },
			  { "Explicit", "keyword" }, { "On", "keyword" },		 { "As", "keyword" },
			  { "AS", "keyword" },		 { "Declare", "keyword" },	 { "Error", "keyword" },
			  { "Execute", "keyword" },	 { "Module", "keyword" },	 { "Private", "keyword" },
			  { "Public", "keyword" },	 { "Default", "keyword" },	 { "Empty", "keyword" },
			  { "GoTo", "keyword" },	 { "False", "keyword" },	 { "Nothing", "keyword" },
			  { "Type", "keyword" },	 { "Lib", "keyword" },		 { "ByVal", "keyword" },
			  { "ByRef", "keyword" },	 { "Null", "keyword" },		 { "Attribute", "keyword" },
			  { "True", "keyword" },	 { "IF", "keyword" },		 { "THEN", "keyword" },
			  { "Begin", "keyword" },	 { "BEGIN", "keyword" },	 { "ELSE", "keyword" },
			  { "ELSEIF", "keyword" },	 { "WHILE", "keyword" },	 { "WEND", "keyword" },
			  { "FOR", "keyword" },		 { "TO", "keyword" },		 { "EACH", "keyword" },
			  { "IN", "keyword" },		 { "STEP", "keyword" },		 { "CASE", "keyword" },
			  { "SELECT", "keyword" },	 { "RETURN", "keyword" },	 { "CONTINUE", "keyword" },
			  { "DO", "keyword" },		 { "UNTIL", "keyword" },	 { "LOOP", "keyword" },
			  { "NEXT", "keyword" },	 { "WITH", "keyword" },		 { "EXIT", "keyword" },
			  { "MOD", "keyword" },		 { "AND", "keyword" },		 { "NOT", "keyword" },
			  { "OR", "keyword" },		 { "XOR", "keyword" },		 { "IS", "keyword" },
			  { "CALL", "keyword" },	 { "CLASS", "keyword" },	 { "CONST", "keyword" },
			  { "DIM", "keyword" },		 { "REDIM", "keyword" },	 { "PRESERVE", "keyword" },
			  { "FUNCTION", "keyword" }, { "SUB", "keyword" },		 { "PROPERTY", "keyword" },
			  { "END", "keyword" },		 { "SET", "keyword" },		 { "LET", "keyword" },
			  { "GET", "keyword" },		 { "NEW", "keyword" },		 { "RANDOMIZE", "keyword" },
			  { "OPTION", "keyword" },	 { "EXPLICIT", "keyword" },	 { "ON", "keyword" },
			  { "ERROR", "keyword" },	 { "EXECUTE", "keyword" },	 { "MODULE", "keyword" },
			  { "PRIVATE", "keyword" },	 { "PUBLIC", "keyword" },	 { "DEFAULT", "keyword" },
			  { "EMPTY", "keyword" },	 { "FALSE", "keyword" },	 { "NOTHING", "keyword" },
			  { "NULL", "keyword" },	 { "TRUE", "keyword" },		 { "Boolean", "keyword2" },
			  { "Byte", "keyword2" },	 { "Char", "keyword2" },	 { "Date", "keyword2" },
			  { "Decimal", "keyword2" }, { "Integer", "keyword2" },	 { "Double", "keyword2" },
			  { "Long", "keyword2" },	 { "Object", "keyword2" },	 { "Short", "keyword2" },
			  { "Single", "keyword2" },	 { "String", "keyword2" },	 { "BOOLEAN", "keyword2" },
			  { "BYTE", "keyword2" },	 { "CHAR", "keyword2" },	 { "DATE", "keyword2" },
			  { "DECIMAL", "keyword2" }, { "DOUBLE", "keyword2" },	 { "LONG", "keyword2" },
			  { "OBJECT", "keyword2" },	 { "SHORT", "keyword2" },	 { "INTEGER", "keyword2" },
			  { "SINGLE", "keyword2" },	 { "STRING", "keyword2" },	 { "Variant", "keyword2" } },
		  "'",
		  {}

		} ).setExtensionPriority( true );
}

}}}} // namespace EE::UI::Doc::Language
