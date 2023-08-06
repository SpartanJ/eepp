#include <algorithm>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreammemory.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/system/packmanager.hpp>
#include <eepp/ui/doc/languages/angelscript.hpp>
#include <eepp/ui/doc/languages/batchscript.hpp>
#include <eepp/ui/doc/languages/crystal.hpp>
#include <eepp/ui/doc/languages/css.hpp>
#include <eepp/ui/doc/languages/d.hpp>
#include <eepp/ui/doc/languages/elixir.hpp>
#include <eepp/ui/doc/languages/elm.hpp>
#include <eepp/ui/doc/languages/fstab.hpp>
#include <eepp/ui/doc/languages/gdscript.hpp>
#include <eepp/ui/doc/languages/glsl.hpp>
#include <eepp/ui/doc/languages/hare.hpp>
#include <eepp/ui/doc/languages/hlsl.hpp>
#include <eepp/ui/doc/languages/htaccess.hpp>
#include <eepp/ui/doc/languages/html.hpp>
#include <eepp/ui/doc/languages/julia.hpp>
#include <eepp/ui/doc/languages/kotlin.hpp>
#include <eepp/ui/doc/languages/markdown.hpp>
#include <eepp/ui/doc/languages/nelua.hpp>
#include <eepp/ui/doc/languages/objeck.hpp>
#include <eepp/ui/doc/languages/odin.hpp>
#include <eepp/ui/doc/languages/pascal.hpp>
#include <eepp/ui/doc/languages/perl.hpp>
#include <eepp/ui/doc/languages/pico-8.hpp>
#include <eepp/ui/doc/languages/po.hpp>
#include <eepp/ui/doc/languages/postgresql.hpp>
#include <eepp/ui/doc/languages/r.hpp>
#include <eepp/ui/doc/languages/rust.hpp>
#include <eepp/ui/doc/languages/sass.hpp>
#include <eepp/ui/doc/languages/solidity.hpp>
#include <eepp/ui/doc/languages/sql.hpp>
#include <eepp/ui/doc/languages/swift.hpp>
#include <eepp/ui/doc/languages/teal.hpp>
#include <eepp/ui/doc/languages/toml.hpp>
#include <eepp/ui/doc/languages/v.hpp>
#include <eepp/ui/doc/languages/vb.hpp>
#include <eepp/ui/doc/languages/verilog.hpp>
#include <eepp/ui/doc/languages/x86assembly.hpp>
#include <eepp/ui/doc/languages/xml.hpp>
#include <eepp/ui/doc/languages/zig.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>
#include <eepp/ui/uiwidgetcreator.hpp>
#include <nlohmann/json.hpp>

using namespace EE::System;
using namespace EE::UI::Doc::Language;

using json = nlohmann::json;

namespace EE { namespace UI { namespace Doc {

SINGLETON_DECLARE_IMPLEMENTATION( SyntaxDefinitionManager )

static void addPlainText() {
	SyntaxDefinitionManager::instance()->add(
		{ "Plain Text", { "%.txt$" }, {}, {}, "", {}, "plaintext" } );
}

static void addC() {
	SyntaxDefinitionManager::instance()->add(
		{ "C",
		  { "%.c$", "%.C", "%.h$", "%.icc" },
		  {
			  { { "//.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "^%s*(#include)%s+([<%\"][%w%d%.%\\%/%_%-]+[>%\"])" },
				{ "keyword", "keyword", "literal" } },
			  { { "^%s*(#e?l?n?d?ifn?d?e?f?)%s+" }, { "keyword", "keyword", "literal" } },
			  { { "^%s*(#define)%s*" }, { "keyword", "keyword", "literal" } },
			  { { "^%s*(#else)%s*" }, { "keyword", "keyword", "literal" } },
			  { { "^%s*#", "[^\\]\n" }, "comment" },
			  { { "\"", "[\"\n]", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "-?0x%x+" }, "number" },
			  { { "-?%d+[%d%.eE]*f?" }, "number" },
			  { { "-?%.?%d+f?" }, "number" },
			  { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },
		  },
		  { { "if", "keyword" },		{ "then", "keyword" },		{ "else", "keyword" },
			{ "elseif", "keyword" },	{ "do", "keyword" },		{ "while", "keyword" },
			{ "for", "keyword" },		{ "break", "keyword" },		{ "continue", "keyword" },
			{ "return", "keyword" },	{ "goto", "keyword" },		{ "struct", "keyword" },
			{ "union", "keyword" },		{ "typedef", "keyword" },	{ "enum", "keyword" },
			{ "extern", "keyword" },	{ "static", "keyword" },	{ "volatile", "keyword" },
			{ "const", "keyword" },		{ "inline", "keyword" },	{ "switch", "keyword" },
			{ "case", "keyword" },		{ "default", "keyword" },	{ "auto", "keyword" },
			{ "const", "keyword" },		{ "void", "keyword" },		{ "int", "keyword2" },
			{ "short", "keyword2" },	{ "long", "keyword2" },		{ "float", "keyword2" },
			{ "double", "keyword2" },	{ "char", "keyword2" },		{ "unsigned", "keyword2" },
			{ "bool", "keyword2" },		{ "true", "literal" },		{ "false", "literal" },
			{ "NULL", "literal" },		{ "uint64_t", "keyword2" }, { "uint32_t", "keyword2" },
			{ "uint16_t", "keyword2" }, { "uint8_t", "keyword2" },	{ "int64_t", "keyword2" },
			{ "int32_t", "keyword2" },	{ "int16_t", "keyword2" },	{ "int8_t", "keyword2" } },
		  "//" } );
}

static void addLua() {
	SyntaxDefinitionManager::instance()->add(
		{ "Lua",
		  { "%.lua$" },
		  {
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "%[%[", "%]%]" }, "string" },
			  { { "%-%-%[%[", "%]%]" }, "comment" },
			  { { "%-%-.-\n" }, "comment" },
			  { { "-?0x%x+" }, "number" },
			  { { "-?%d+[%d%.eE]*" }, "number" },
			  { { "-?%.?%d+" }, "number" },
			  { { "%.%.%.?" }, "operator" },
			  { { "[<>~=]=" }, "operator" },
			  { { "[%+%-=/%*%^%%#<>]" }, "operator" },
			  { { "[%a_][%w_]*%s*%f[(\"{]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },
			  { { "::[%a_][%w_]*::" }, "function" },
		  },
		  {
			  { "if", "keyword" },		 { "then", "keyword" },	  { "else", "keyword" },
			  { "elseif", "keyword" },	 { "end", "keyword" },	  { "do", "keyword" },
			  { "function", "keyword" }, { "repeat", "keyword" }, { "until", "keyword" },
			  { "while", "keyword" },	 { "for", "keyword" },	  { "break", "keyword" },
			  { "return", "keyword" },	 { "local", "keyword" },  { "in", "keyword" },
			  { "not", "keyword" },		 { "and", "keyword" },	  { "or", "keyword" },
			  { "goto", "keyword" },	 { "self", "keyword2" },  { "true", "literal" },
			  { "false", "literal" },	 { "nil", "literal" },
		  },
		  "--",
		  { "^#!.*[ /]lua" } } );
}

static void addJavaScript() {
	SyntaxDefinitionManager::instance()->add(
		{ "JavaScript",
		  { "%.js$" },
		  {
			  { { "//.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "`", "`", "\\" }, "string" },
			  { { "/[%+%-%*%^%!%=%&%|%?%:%;%,%(%[%{%<%>%\\].*%f[/]",
				  "/[igmsuyd\n]?[igmsuyd\n]?[igmsuyd\n]?", "\\" },
				"string" },
			  { { "0x[%da-fA-F]+" }, "number" },
			  { { "-?%d+[%d%.eE]*" }, "number" },
			  { { "-?%.?%d+" }, "number" },
			  { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
			  { { "([%w_][%w_]+)%.([%w_][%w%d_]*)%s*(=)%s*(function)" },
				{ "normal", "keyword2", "function", "operator", "keyword" } },
			  { { "([%w_][%w_]+)%.([%w_][%w%d_]*)%s*(=)%s*(async%s*function)" },
				{ "normal", "keyword2", "function", "operator", "keyword" } },
			  { { "([%w_][%w_]+)%.([%w_][%w%d_]*)%s*(=)%s*%f[(]" },
				{ "normal", "keyword2", "function", "operator" } },
			  { { "([%w_][%w_]+)%.([%w_][%w%d_]*)%s*(=)%s*(async)%s*%f[(]" },
				{ "normal", "keyword2", "function", "operator", "function" } },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },
		  },
		  { { "arguments", "keyword2" }, { "async", "keyword" },	 { "await", "keyword" },
			{ "break", "keyword" },		 { "case", "keyword" },		 { "catch", "keyword" },
			{ "class", "keyword" },		 { "const", "keyword" },	 { "continue", "keyword" },
			{ "debugger", "keyword" },	 { "default", "keyword" },	 { "delete", "keyword" },
			{ "do", "keyword" },		 { "else", "keyword" },		 { "export", "keyword" },
			{ "extends", "keyword" },	 { "false", "literal" },	 { "finally", "keyword" },
			{ "for", "keyword" },		 { "get", "keyword" },		 { "if", "keyword" },
			{ "import", "keyword" },	 { "in", "keyword" },		 { "Infinity", "keyword2" },
			{ "instanceof", "keyword" }, { "let", "keyword" },		 { "NaN", "keyword2" },
			{ "new", "keyword" },		 { "null", "literal" },		 { "return", "keyword" },
			{ "set", "keyword" },		 { "super", "keyword" },	 { "static", "keyword" },
			{ "switch", "keyword" },	 { "this", "keyword2" },	 { "throw", "keyword" },
			{ "true", "literal" },		 { "try", "keyword" },		 { "typeof", "keyword" },
			{ "undefined", "literal" },	 { "var", "keyword" },		 { "void", "keyword" },
			{ "while", "keyword" },		 { "with", "keyword" },		 { "yield", "keyword" },
			{ "implements", "keyword" }, { "Array", "keyword2" },	 { "any", "keyword" },
			{ "from", "keyword" },		 { "public", "keyword" },	 { "private", "keyword" },
			{ "declare", "keyword" },	 { "namespace", "keyword" }, { "protected", "keyword" },
			{ "enum", "keyword" },		 { "function", "keyword" },	 { "of", "keyword" } },
		  "//" } );
}

static void addJSON() {
	SyntaxDefinitionManager::instance()->add(
		{ "JSON",
		  { "%.json$", "%.cson$", "%.webmanifest" },
		  {
			  { { "(%b\"\")(:)" }, { "normal", "keyword", "operator" } },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "`", "`", "\\" }, "string" },
			  { { "0x[%da-fA-F]+" }, "number" },
			  { { "-?%d+[%d%.eE]*" }, "number" },
			  { { "-?%.?%d+" }, "number" },
			  { { "[%[%]%{%}]" }, "operator" },
			  { { "[%a_][%w_]*" }, "symbol" },
		  },
		  { { "true", "literal" }, { "false", "literal" } },
		  "//" } );
}

static void addTypeScript() {
	SyntaxDefinition& ts = SyntaxDefinitionManager::instance()->add(
		{ "TypeScript",
		  { "%.ts$", "%.d.ts$" },
		  {
			  { { "//.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "`", "`", "\\" }, "string" },
			  { { "/[%+%-%*%^%!%=%&%|%?%:%;%,%(%[%{%<%>%\\].*%f[/]",
				  "/[igmsuyd\n]?[igmsuyd\n]?[igmsuyd\n]?", "\\" },
				"string" },
			  { { "0x[%da-fA-F]+" }, "number" },
			  { { "-?%d+[%d%.eE]*" }, "number" },
			  { { "-?%.?%d+" }, "number" },
			  { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
			  { { "(interface%s)([%a_][%w_]*)" }, { "normal", "keyword", "keyword2" } },
			  { { "(type%s)([%a_][%w_]*)" }, { "normal", "keyword", "keyword2" } },
			  { { "[%a_][%w_$]*%f[(]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },
		  },
		  { { "any", "keyword2" },		 { "arguments", "keyword2" }, { "as", "keyword" },
			{ "async", "keyword" },		 { "await", "keyword" },	  { "boolean", "keyword2" },
			{ "break", "keyword" },		 { "case", "keyword" },		  { "catch", "keyword" },
			{ "class", "keyword" },		 { "const", "keyword" },	  { "constructor", "function" },
			{ "continue", "keyword" },	 { "debugger", "keyword" },	  { "declare", "keyword" },
			{ "default", "keyword" },	 { "delete", "keyword" },	  { "do", "keyword" },
			{ "else", "keyword" },		 { "enum", "keyword" },		  { "export", "keyword" },
			{ "extends", "keyword" },	 { "false", "literal" },	  { "finally", "keyword" },
			{ "for", "keyword" },		 { "from", "keyword" },		  { "function", "keyword" },
			{ "get", "keyword" },		 { "if", "keyword" },		  { "implements", "keyword" },
			{ "import", "keyword" },	 { "in", "keyword" },		  { "Infinity", "keyword2" },
			{ "instanceof", "keyword" }, { "interface", "keyword" },  { "let", "keyword" },
			{ "module", "keyword" },	 { "new", "keyword" },		  { "null", "literal" },
			{ "number", "keyword2" },	 { "of", "keyword" },		  { "package", "keyword" },
			{ "private", "keyword" },	 { "protected", "keyword" },  { "public", "keyword" },
			{ "require", "keyword" },	 { "return", "keyword" },	  { "set", "keyword" },
			{ "static", "keyword" },	 { "string", "keyword2" },	  { "super", "keyword" },
			{ "switch", "keyword" },	 { "symbol", "keyword2" },	  { "this", "keyword2" },
			{ "throw", "keyword" },		 { "true", "literal" },		  { "try", "keyword" },
			{ "type", "keyword2" },		 { "typeof", "keyword" },	  { "undefined", "literal" },
			{ "var", "keyword" },		 { "void", "keyword" },		  { "while", "keyword" },
			{ "with", "keyword" },		 { "yield", "keyword" },	  { "unknown", "keyword2" },
			{ "namespace", "keyword" },	 { "abstract", "keyword" } },
		  "//" } );

	SyntaxDefinitionManager::instance()->add(
		{ "TSX",
		  { "%.tsx$" },
		  {
			  { { "//.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "`", "`", "\\" }, "string" },
			  { { "/[%+%-%*%^%!%=%&%|%?%:%;%,%(%[%{%<%>%\\].*%f[/]",
				  "/[igmsuyd\n]?[igmsuyd\n]?[igmsuyd\n]?", "\\" },
				"string" },
			  { { "%f[^<]![%a_][%w%_%-]*" }, "keyword2" },
			  { { "%f[^<][%a_][%w%_%-]*" }, "function" },
			  { { "%f[^<]/[%a_][%w%_%-]*" }, "function" },
			  { { "([%a_-][%w-_]*)(%\?\?)(=)%f[%{%\"]" },
				{ "normal", "keyword", "normal", "operator" } },
			  { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
			  { { "0x[%da-fA-F]+" }, "number" },
			  { { "-?%d+[%d%.eE]*" }, "number" },
			  { { "-?%.?%d+" }, "number" },
			  { { "(interface%s)([%a_][%w_]*)" }, { "normal", "keyword", "keyword2" } },
			  { { "(type%s)([%a_][%w_]*)" }, { "normal", "keyword", "keyword2" } },
			  { { "[%a_][%w_$]*%f[(]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },
		  },
		  ts.getSymbols(),
		  "//" } );
}

static void addPython() {
	SyntaxDefinitionManager::instance()->add(
		{ "Python",
		  { "%.py$", "%.pyw$" },
		  {
			  { { "#", "\n" }, "comment" },
			  { { "[ruU]?\"", "\"", "\\" }, "string" },
			  { { "[ruU]?'", "'", "\\" }, "string" },
			  { { "\"\"\"", "\"\"\"" }, "string" },
			  { { "0x[%da-fA-F]+" }, "number" },
			  { { "-?%d+[%d%.eE]*" }, "number" },
			  { { "-?%.?%d+" }, "number" },
			  { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },
		  },
		  {
			  { "class", "keyword" },  { "finally", "keyword" },  { "is", "keyword" },
			  { "return", "keyword" }, { "continue", "keyword" }, { "for", "keyword" },
			  { "lambda", "keyword" }, { "try", "keyword" },	  { "def", "keyword" },
			  { "from", "keyword" },   { "nonlocal", "keyword" }, { "while", "keyword" },
			  { "and", "keyword" },	   { "global", "keyword" },	  { "not", "keyword" },
			  { "with", "keyword" },   { "as", "keyword" },		  { "elif", "keyword" },
			  { "if", "keyword" },	   { "or", "keyword" },		  { "else", "keyword" },
			  { "import", "keyword" }, { "pass", "keyword" },	  { "break", "keyword" },
			  { "except", "keyword" }, { "in", "keyword" },		  { "del", "keyword" },
			  { "raise", "keyword" },  { "yield", "keyword" },	  { "assert", "keyword" },
			  { "self", "keyword2" },  { "None", "literal" },	  { "True", "literal" },
			  { "False", "literal" },
		  },
		  "#",
		  { "^#!.*[ /]python", "^#!.*[ /]python3" } } );
}

static void addBash() {
	SyntaxDefinitionManager::instance()->add(

		{ "Shell script",
		  { "%.sh$", "%.bash$", "^%.bashrc$", "^%.bash_profile$", "^%.profile$" },
		  {
			  { { "$[%a_@*#][%w_]*" }, "keyword2" },
			  { { "#.*\n" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "`", "`", "\\" }, "string" },
			  { { "%f[%w_%.%/]%d[%d%.]*%f[^%w_%.]" }, "number" },
			  { { "[!<>|&%[%]:=*]" }, "operator" },
			  { { "%f[%S][%+%-][%w%-_:]+" }, "function" },
			  { { "%f[%S][%+%-][%w%-_]+%f[=]" }, "function" },
			  { { "(%s%-%a[%w_%-]*%s+)(%d[%d%.]+)" }, { "normal", "function", "number" } },
			  { { "(%s%-%a[%w_%-]*%s+)(%a[%a%-_:=]+)" }, { "normal", "function", "symbol" } },
			  { { "[_%a][%w_]+%f[%+=]" }, "keyword2" },
			  { { "${.-}" }, "keyword2" },
			  { { "$[%d$%a_@*][%w_]*" }, "keyword2" },
			  { { "[%a_%-][%w_%-]*[%s]*%f[(]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },
			  { { "%s+" }, "normal" },
			  { { "%w+%f[%s]" }, "normal" },

		  },
		  {
			  { "until", "keyword" },	  { "unset", "keyword" },	 { "unalias", "keyword" },
			  { "type", "keyword" },	  { "time", "keyword" },	 { "test", "keyword" },
			  { "source", "keyword" },	  { "true", "literal" },	 { "shift", "keyword" },
			  { "set", "keyword" },		  { "then", "keyword" },	 { "select", "keyword" },
			  { "readarray", "keyword" }, { "pwd", "keyword" },		 { "mapfile", "keyword" },
			  { "local", "keyword" },	  { "return", "keyword" },	 { "let", "keyword" },
			  { "fi", "keyword" },		  { "getopts", "keyword" },	 { "echo", "keyword" },
			  { "do", "keyword" },		  { "eval", "keyword" },	 { "elif", "keyword" },
			  { "declare", "keyword" },	  { "cd", "keyword" },		 { "case", "keyword" },
			  { "printf", "keyword" },	  { "break", "keyword" },	 { "exec", "keyword" },
			  { "alias", "keyword" },	  { "exit", "keyword" },	 { "esac", "keyword" },
			  { "export", "keyword" },	  { "for", "keyword" },		 { "enable", "keyword" },
			  { "jobs", "keyword" },	  { "function", "keyword" }, { "while", "keyword" },
			  { "read", "keyword" },	  { "hash", "keyword" },	 { "help", "keyword" },
			  { "history", "keyword" },	  { "done", "keyword" },	 { "if", "keyword" },
			  { "false", "literal" },	  { "in", "keyword" },		 { "else", "keyword" },
			  { "continue", "keyword" },  { "kill", "keyword" },

		  },
		  "#",
		  { "^#!.*[ /]bash", "^#!.*[ /]sh" },
		  "shellscript" } );
}

static void addCPP() {
	SyntaxDefinitionManager::instance()->add(
		{ "C++",
		  { "%.cpp$", "%.cc$", "%.cxx$", "%.c++$", "%.hh$", "%.inl$", "%.hxx$", "%.hpp$",
			"%.h++$" },
		  {
			  { { "R%\"xml%(", "%)xml%\"" }, "function", "XML" },
			  { { "R%\"css%(", "%)css%\"" }, "function", "CSS" },
			  { { "R%\"html%(", "%)html%\"" }, "function", "HTML" },
			  { { "R%\"json%(", "%)json%\"" }, "function", "JSON" },
			  { { "R\"[%a-\"]+%(", "%)[%a-\"]+%\"" }, "string" },
			  { { "R\"%(", "%)\"" }, "string" },
			  { { "//.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "\"", "[\"\n]", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "^%s*(#include)%s+([<%\"][%w%d%.%\\%/%_%-]+[>%\"])" },
				{ "keyword", "keyword", "literal" } },
			  { { "^%s*(#e?l?n?d?ifn?d?e?f?)%s+" }, { "keyword", "keyword", "literal" } },
			  { { "^%s*(#define)%s*" }, { "keyword", "keyword", "literal" } },
			  { { "^%s*(#else)%s*" }, { "keyword", "keyword", "literal" } },
			  { { "^%s*#", "[^\\]\n" }, "comment" },
			  { { "-?0x%x+" }, "number" },
			  { { "-?%d+[%d%.eE]*f?" }, "number" },
			  { { "-?%.?%d+f?" }, "number" },
			  { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "std%:%:[%w_]*" }, "keyword2" },
			  { { "[%a_][%w_]*" }, "symbol" },
		  },
		  {
			  { "alignof", "keyword" },
			  { "alignas", "keyword" },
			  { "and", "keyword" },
			  { "and_eq", "keyword" },
			  { "not", "keyword" },
			  { "not_eq", "keyword" },
			  { "or", "keyword" },
			  { "or_eq", "keyword" },
			  { "xor", "keyword" },
			  { "xor_eq", "keyword" },
			  { "private", "keyword" },
			  { "protected", "keyword" },
			  { "public", "keyword" },
			  { "register", "keyword" },
			  { "nullptr", "keyword" },
			  { "operator", "keyword" },
			  { "asm", "keyword" },
			  { "bitand", "keyword" },
			  { "bitor", "keyword" },
			  { "catch", "keyword" },
			  { "throw", "keyword" },
			  { "try", "keyword" },
			  { "class", "keyword" },
			  { "compl", "keyword" },
			  { "explicit", "keyword" },
			  { "export", "keyword" },
			  { "concept", "keyword" },
			  { "consteval", "keyword" },
			  { "constexpr", "keyword" },
			  { "constinit", "keyword" },
			  { "const_cast", "keyword" },
			  { "dynamic_cast", "keyword" },
			  { "reinterpret_cast", "keyword" },
			  { "static_cast", "keyword" },
			  { "static_assert", "keyword" },
			  { "template", "keyword" },
			  { "this", "keyword" },
			  { "thread_local", "keyword" },
			  { "requires", "keyword" },
			  { "co_wait", "keyword" },
			  { "co_return", "keyword" },
			  { "co_yield", "keyword" },
			  { "decltype", "keyword" },
			  { "delete", "keyword" },
			  { "export", "keyword" },
			  { "friend", "keyword" },
			  { "typeid", "keyword" },
			  { "typename", "keyword" },
			  { "mutable", "keyword" },
			  { "virtual", "keyword" },
			  { "using", "keyword" },
			  { "namespace", "keyword" },
			  { "new", "keyword" },
			  { "noexcept", "keyword" },
			  { "if", "keyword" },
			  { "then", "keyword" },
			  { "else", "keyword" },
			  { "elseif", "keyword" },
			  { "do", "keyword" },
			  { "while", "keyword" },
			  { "for", "keyword" },
			  { "break", "keyword" },
			  { "continue", "keyword" },
			  { "return", "keyword" },
			  { "goto", "keyword" },
			  { "struct", "keyword" },
			  { "union", "keyword" },
			  { "typedef", "keyword" },
			  { "enum", "keyword" },
			  { "extern", "keyword" },
			  { "static", "keyword" },
			  { "volatile", "keyword" },
			  { "const", "keyword" },
			  { "inline", "keyword" },
			  { "switch", "keyword" },
			  { "case", "keyword" },
			  { "default", "keyword" },
			  { "auto", "keyword2" },
			  { "const", "keyword" },
			  { "void", "keyword" },
			  { "int", "keyword2" },
			  { "short", "keyword2" },
			  { "long", "keyword2" },
			  { "float", "keyword2" },
			  { "double", "keyword2" },
			  { "char", "keyword2" },
			  { "unsigned", "keyword2" },
			  { "bool", "keyword2" },
			  { "true", "keyword2" },
			  { "false", "keyword2" },
			  { "wchar_t", "keyword2" },
			  { "char8_t", "keyword2" },
			  { "char16_t", "keyword2" },
			  { "char32_t", "keyword2" },
			  { "size_t", "keyword2" },
			  { "int16_t", "keyword2" },
			  { "int32_t", "keyword2" },
			  { "int64_t", "keyword2" },
			  { "uint16_t", "keyword2" },
			  { "uint32_t", "keyword2" },
			  { "uint64_t", "keyword2" },
			  { "String", "keyword2" },
			  { "Int8", "keyword2" },
			  { "Uint8", "keyword2" },
			  { "Int16", "keyword2" },
			  { "Uint16", "keyword2" },
			  { "Int32", "keyword2" },
			  { "Uint32", "keyword2" },
			  { "Int64", "keyword2" },
			  { "Uint64", "keyword2" },
			  { "Float", "keyword2" },
			  { "Color", "keyword2" },
			  { "Vector2f", "keyword2" },
			  { "Vector2i", "keyword2" },
			  { "Recti", "keyword2" },
			  { "Rectf", "keyword2" },
			  { "NULL", "literal" },
		  },
		  "//",
		  {},
		  "cpp" } );
}

static void addPHP() {
	SyntaxDefinitionManager::instance()
		->add( { "PHP",
				 { "%.php$", "%.php3$", "%.php4$", "%.php5$" },
				 {
					 { { "<%s*[sS][cC][rR][iI][pP][tT]%s+[tT][yY][pP][eE]%s*=%s*['\"]%a+/"
						 "[jJ][aA][vV][aA][sS][cC][rR][iI][pP][tT]['\"]%s*>",
						 "<%s*/[sS][cC][rR][iI][pP][tT]>" },
					   "function",
					   "JavaScript" },
					 { { "<%s*[sS][cC][rR][iI][pP][tT]%s*>", "<%s*/%s*[sS][cC][rR][iI][pP][tT]>" },
					   "function",
					   "JavaScript" },
					 { { "<%s*[sS][tT][yY][lL][eE][^>]*>", "<%s*/%s*[sS][tT][yY][lL][eE]%s*>" },
					   "function",
					   "CSS" },
					 { { "<%?p?h?p?", "%?>" }, "function", "PHPCore" },
					 { { "<!%-%-", "%-%->" }, "comment" },
					 { { "%f[^>][^<]", "%f[<]" }, "normal" },
					 { { "\"", "\"", "\\" }, "string" },
					 { { "'", "'", "\\" }, "string" },
					 { { "0x[%da-fA-F]+" }, "number" },
					 { { "-?%d+[%d%.]*f?" }, "number" },
					 { { "-?%.?%d+f?" }, "number" },
					 { { "%f[^<]![%a_][%w%_%-]*" }, "keyword2" },
					 { { "%f[^<][%a_][%w%_%-]*" }, "function" },
					 { { "%f[^<]/[%a_][%w%_%-]*" }, "function" },
					 { { "[%a_][%w_]*" }, "keyword" },
					 { { "[/<>=]" }, "operator" },
				 },
				 {},
				 "",
				 { "^#!.*[ /]php" } } )
		.setAutoCloseXMLTags( true );

	SyntaxDefinitionManager::instance()
		->add( { "PHPCore",
				 {},
				 {
					 { { "<%?p?h?p?" }, "function" },
					 { { "%?>", "<%?p?h?p?" }, "function", "HTML" },
					 { { "//.-\n" }, "comment" },
					 { { "/%*", "%*/" }, "comment" },
					 { { "#.-\n" }, "comment" },
					 { { "\"", "\"", "\\" }, "string" },
					 { { "'", "'", "\\" }, "string" },
					 { { "%\\x[%da-fA-F]+" }, "number" },
					 { { "-?%d+[%d%.eE]*" }, "number" },
					 { { "-?%.?%d+" }, "number" },
					 { { "[%.%+%-=/%*%^%%<>!~|&]" }, "operator" },
					 { { "[%a_][%w_]*%f[(]" }, "function" },
					 { { "[%a_][%w_]*" }, "symbol" },
					 { { "%$[%a][%w_]*" }, "keyword2" },
				 },
				 { { "return", "keyword" },		 { "if", "keyword" },
				   { "else", "keyword" },		 { "elseif", "keyword" },
				   { "endif", "keyword" },		 { "declare", "keyword" },
				   { "enddeclare", "keyword" },	 { "switch", "keyword" },
				   { "endswitch", "keyword" },	 { "as", "keyword" },
				   { "do", "keyword" },			 { "for", "keyword" },
				   { "endfor", "keyword" },		 { "foreach", "keyword" },
				   { "endforeach", "keyword" },	 { "while", "keyword" },
				   { "endwhile", "keyword" },	 { "switch", "keyword" },
				   { "case", "keyword" },		 { "continue", "keyword" },
				   { "default", "keyword" },	 { "break", "keyword" },
				   { "exit", "keyword" },		 { "goto", "keyword" },

				   { "catch", "keyword" },		 { "throw", "keyword" },
				   { "try", "keyword" },		 { "finally", "keyword" },

				   { "class", "keyword" },		 { "trait", "keyword" },
				   { "interface", "keyword" },	 { "public", "keyword" },
				   { "static", "keyword" },		 { "protected", "keyword" },
				   { "private", "keyword" },	 { "abstract", "keyword" },
				   { "final", "keyword" },

				   { "function", "keyword2" },	 { "global", "keyword2" },
				   { "var", "keyword2" },		 { "const", "keyword2" },
				   { "bool", "keyword2" },		 { "boolean", "keyword2" },
				   { "int", "keyword2" },		 { "integer", "keyword2" },
				   { "real", "keyword2" },		 { "double", "keyword2" },
				   { "float", "keyword2" },		 { "string", "keyword2" },
				   { "array", "keyword2" },		 { "object", "keyword2" },
				   { "callable", "keyword2" },	 { "iterable", "keyword2" },

				   { "namespace", "keyword2" },	 { "extends", "keyword2" },
				   { "implements", "keyword2" }, { "instanceof", "keyword2" },
				   { "require", "keyword2" },	 { "require_once", "keyword2" },
				   { "include", "keyword2" },	 { "include_once", "keyword2" },
				   { "use", "keyword2" },		 { "new", "keyword2" },
				   { "clone", "keyword2" },

				   { "true", "literal" },		 { "false", "literal" },
				   { "NULL", "literal" },		 { "parent", "literal" },
				   { "self", "literal" },		 { "echo", "function" } },
				 "//",
				 {},
				 "php" } )
		.setVisible( false );
}

static void addIni() {
	SyntaxDefinitionManager::instance()->add(
		{ "Config File",
		  { "%.ini$", "%.conf$", "%.desktop$", "%.service$", "%.cfg$", "%.properties$",
			"Doxyfile" },
		  { { { "%s?#%x%x%x%x%x%x%x%x" }, "string" },
			{ { "%s?#%x%x%x%x%x%x" }, "string" },
			{ { "^#.-\n" }, "comment" },
			{ { "^;.-\n" }, "comment" },
			{ { "%s#.-\n" }, "comment" },
			{ { "[%a_][%w-+_%s%p]-%f[=]" }, "keyword" },
			{ { "\"", "\"", "\\" }, "string" },
			{ { "'", "'", "\\" }, "string" },
			{ { "^%[.-%]" }, "keyword2" },
			{ { "%s%[.-%]" }, "keyword2" },
			{ { "=" }, "operator" },
			{ { "https?://[%w_.~!*:@&+$/?%%#-]-%w[-.%w]*%.%w%w%w?%w?:?%d*/?[%w_.~!*:@&+$/"
				"?%%#=-]*" },
			  "link" },
			{ { "[a-z]+" }, "symbol" } },
		  { { "true", "literal" }, { "false", "literal" } },
		  "#",
		  { "^%[.-%]%f[^\n]" },
		  "ini" } );
}

static void addMakefile() {
	SyntaxDefinitionManager::instance()->add( { "Makefile",
												{ "Makefile", "makefile", "%.mk$", "%.make$" },
												{
													{ { "#.*\n" }, "comment" },
													{ { "[[.]]}" }, "normal" },
													{ { "$[@^<%%?+|*]" }, "keyword2" },
													{ { "$%(", "%)" }, "keyword" },
													{ { "%f[%w_][%d%.]+%f[^%w_]" }, "number" },
													{ { "%..*:" }, "keyword2" },
													{ { ".*:=" }, "function" },
													{ { ".*+=" }, "function" },
													{ { ".*%s=" }, "function" },
												},
												{},
												"#" } );
}

static void addCSharp() {
	SyntaxDefinitionManager::instance()->add(
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
		  { { "abstract", "keyword" },	{ "as", "keyword" },		{ "await", "keyword" },
			{ "base", "keyword" },		{ "break", "keyword" },		{ "case", "keyword" },
			{ "catch", "keyword" },		{ "checked", "keyword" },	{ "class", "keyword" },
			{ "const", "keyword" },		{ "continue", "keyword" },	{ "default", "keyword" },
			{ "delegate", "keyword" },	{ "do", "keyword" },		{ "else", "keyword" },
			{ "enum", "keyword" },		{ "event", "keyword" },		{ "explicit", "keyword" },
			{ "extern", "keyword" },	{ "finally", "keyword" },	{ "fixed", "keyword" },
			{ "for", "keyword" },		{ "foreach", "keyword" },	{ "get", "keyword" },
			{ "goto", "keyword" },		{ "if", "keyword" },		{ "implicit", "keyword" },
			{ "in", "keyword" },		{ "interface", "keyword" }, { "internal", "keyword" },
			{ "is", "keyword" },		{ "lock", "keyword" },		{ "namespace", "keyword" },
			{ "new", "keyword" },		{ "operator", "keyword" },	{ "out", "keyword" },
			{ "override", "keyword" },	{ "params", "keyword" },	{ "private", "keyword" },
			{ "protected", "keyword" }, { "public", "keyword" },	{ "readonly", "keyword" },
			{ "ref", "keyword" },		{ "return", "keyword" },	{ "sealed", "keyword" },
			{ "set", "keyword" },		{ "sizeof", "keyword" },	{ "stackalloc", "keyword" },
			{ "static", "keyword" },	{ "struct", "keyword" },	{ "switch", "keyword" },
			{ "this", "keyword" },		{ "throw", "keyword" },		{ "try", "keyword" },
			{ "typeof", "keyword" },	{ "unchecked", "keyword" }, { "unsafe", "keyword" },
			{ "using", "keyword" },		{ "var", "keyword" },		{ "virtual", "keyword" },
			{ "void", "keyword" },		{ "volatile", "keyword" },	{ "where", "keyword" },
			{ "while", "keyword" },		{ "yield", "keyword" },		{ "bool", "keyword2" },
			{ "byte", "keyword2" },		{ "char", "keyword2" },		{ "decimal", "keyword2" },
			{ "double", "keyword2" },	{ "float", "keyword2" },	{ "int", "keyword2" },
			{ "long", "keyword2" },		{ "object", "keyword2" },	{ "sbyte", "keyword2" },
			{ "short", "keyword2" },	{ "string", "keyword2" },	{ "uint", "keyword2" },
			{ "ulong", "keyword2" },	{ "ushort", "keyword2" },	{ "true", "literal" },
			{ "false", "literal" },		{ "null", "literal" },		{ "add", "keyword" },
			{ "record", "keyword" },	{ "remove", "keyword" },	{ "partial", "keyword" },
			{ "dynamic", "keyword" },	{ "value", "keyword" },		{ "global", "keyword" },
			{ "when", "keyword" } },
		  "//",
		  {},
		  "csharp" } );
}

static void addGo() {
	SyntaxDefinitionManager::instance()->add(
		{ "Go",
		  { "%.go$" },
		  {
			  { { "//.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "`", "`", "\\" }, "string" },
			  { { "0[oO_][0-7]+" }, "number" },
			  { { "-?0x[%x_]+" }, "number" },
			  { { "-?%d+_%d" }, "number" },
			  { { "-?%d+[%d%.eE]*f?" }, "number" },
			  { { "-?%.?%d+f?" }, "number" },
			  { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
			  { { ":=" }, "operator" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },
		  },
		  {
			  { "if", "keyword" },		 { "else", "keyword" },		  { "elseif", "keyword" },
			  { "for", "keyword" },		 { "continue", "keyword" },	  { "return", "keyword" },
			  { "struct", "keyword" },	 { "switch", "keyword" },	  { "case", "keyword" },
			  { "default", "keyword" },	 { "const", "keyword" },	  { "package", "keyword" },
			  { "import", "keyword" },	 { "func", "keyword" },		  { "var", "keyword" },
			  { "type", "keyword" },	 { "interface", "keyword" },  { "select", "keyword" },
			  { "break", "keyword" },	 { "range", "keyword" },	  { "chan", "keyword" },
			  { "defer", "keyword" },	 { "go", "keyword" },		  { "fallthrough", "keyword" },
			  { "int", "keyword2" },	 { "int64", "keyword2" },	  { "int32", "keyword2" },
			  { "int16", "keyword2" },	 { "int8", "keyword2" },	  { "uint", "keyword2" },
			  { "uint64", "keyword2" },	 { "uint32", "keyword2" },	  { "uint16", "keyword2" },
			  { "uint8", "keyword2" },	 { "uintptr", "keyword2" },	  { "float64", "keyword2" },
			  { "float32", "keyword2" }, { "map", "keyword2" },		  { "string", "keyword2" },
			  { "rune", "keyword2" },	 { "bool", "keyword2" },	  { "byte", "keyword2" },
			  { "error", "keyword2" },	 { "complex64", "keyword2" }, { "complex128", "keyword2" },
			  { "true", "literal" },	 { "false", "literal" },	  { "nil", "literal" },
		  },
		  "//" } );
}

static void addHaskell() {
	SyntaxDefinitionManager::instance()->add(
		{ "Haskell",
		  { "%.hs$" },
		  {
			  { { "%-%-", "\n" }, "comment" },
			  { { "{%-", "%-}" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "-?0x%x+" }, "number" },
			  { { "-?%d+[%d%.eE]*f?" }, "number" },
			  { { "-?%.?%d+f?" }, "number" },
			  { { "[!%#%$%%&*+./%<=>%?@\\%^|%-~:]" }, "operator" },
			  { { "[%a_'][%w_']*" }, "symbol" },
		  },
		  {
			  { "as", "keyword" },		 { "case", "keyword" },	   { "of", "keyword" },
			  { "class", "keyword" },	 { "data", "keyword" },	   { "default", "keyword" },
			  { "deriving", "keyword" }, { "do", "keyword" },	   { "forall", "keyword" },
			  { "foreign", "keyword" },	 { "hiding", "keyword" },  { "if", "keyword" },
			  { "then", "keyword" },	 { "else", "keyword" },	   { "import", "keyword" },
			  { "infix", "keyword" },	 { "infixl", "keyword" },  { "infixr", "keyword" },
			  { "let", "keyword" },		 { "in", "keyword" },	   { "mdo", "keyword" },
			  { "module", "keyword" },	 { "newtype", "keyword" }, { "qualified", "keyword" },
			  { "type", "keyword" },	 { "where", "keyword" },
		  },
		  "%-%-" } );
}

static void addLatex() {
	SyntaxDefinitionManager::instance()->add( { "LaTeX",
												{ "%.tex$" },
												{
													{ { "%%", "\n" }, "comment" },
													{ { "&" }, "operator" },
													{ { "\\\\" }, "operator" },
													{ { "%$", "%$" }, "operator" },
													{ { "\\%[", "\\]" }, "operator" },
													{ { "{", "}" }, "keyword" },
													{ { "\\%w*" }, "keyword2" },
												},
												{},
												"%%" } );
}

static void addMeson() {
	SyntaxDefinitionManager::instance()->add( { "Meson",
												{ "meson.build$" },
												{
													{ { "#", "\n" }, "comment" },
													{ { "\"", "\"", "\\" }, "string" },
													{ { "'", "'", "\\" }, "string" },
													{ { "'''", "'''" }, "string" },
													{ { "0x[%da-fA-F]+" }, "number" },
													{ { "-?%d+%d*" }, "number" },
													{ { "[%+%-=/%%%*!]" }, "operator" },
													{ { "[%a_][%w_]*%f[(]" }, "function" },
													{ { "[%a_][%w_]*" }, "symbol" },
												},
												{
													{ "if", "keyword" },
													{ "then", "keyword" },
													{ "else", "keyword" },
													{ "elif", "keyword" },
													{ "endif", "keyword" },
													{ "foreach", "keyword" },
													{ "endforeach", "keyword" },
													{ "break", "keyword" },
													{ "continue", "keyword" },
													{ "and", "keyword" },
													{ "not", "keyword" },
													{ "or", "keyword" },
													{ "in", "keyword" },
													{ "true", "literal" },
													{ "false", "literal" },
												},
												"#" } );
}

static void addDiff() {
	SyntaxDefinitionManager::instance()->add( { "Diff File",
												{ "%.diff$", "%.patch$" },
												{
													{ { "^%+%+%+%s.-\n" }, "keyword" },
													{ { "^%-%-%-%s.-\n" }, "keyword" },
													{ { "^diff%s.-\n" }, "string" },
													{ { "^index%s.-\n" }, "comment" },
													{ { "^@@.-\n" }, "number" },
													{ { "^%+.-\n" }, "function" },
													{ { "^%-.-\n" }, "keyword2" },
												},
												{},
												"",
												{},
												"diff" } );
}

static void addJava() {
	SyntaxDefinitionManager::instance()->add(
		{ "Java",
		  { "%.java$" },
		  {
			  { { "//.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "'\\x%x?%x?%x?%x'" }, "string" },
			  { { "'\\u%x%x%x%x'" }, "string" },
			  { { "'\\?.'" }, "string" },
			  { { "-?0x%x+" }, "number" },
			  { { "-?%d+[%d%.eE]*f?" }, "number" },
			  { { "-?%.?%d+f?" }, "number" },
			  { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },
		  },
		  {
			  { "if", "keyword" },		 { "then", "keyword" },			{ "else", "keyword" },
			  { "elseif", "keyword" },	 { "do", "keyword" },			{ "while", "keyword" },
			  { "for", "keyword" },		 { "new", "keyword" },			{ "break", "keyword" },
			  { "continue", "keyword" }, { "return", "keyword" },		{ "goto", "keyword" },
			  { "class", "keyword" },	 { "implements", "keyword" },	{ "extends", "keyword" },
			  { "private", "keyword" },	 { "protected", "keyword" },	{ "public", "keyword" },
			  { "abstract", "keyword" }, { "interface", "keyword" },	{ "assert", "keyword" },
			  { "import", "keyword" },	 { "native", "keyword" },		{ "package", "keyword" },
			  { "super", "keyword" },	 { "synchronized", "keyword" }, { "instanceof", "keyword" },
			  { "enum", "keyword" },	 { "catch", "keyword" },		{ "throw", "keyword" },
			  { "throws", "keyword" },	 { "try", "keyword" },			{ "transient", "keyword" },
			  { "finally", "keyword" },	 { "static", "keyword" },		{ "volatile", "keyword" },
			  { "final", "keyword" },	 { "switch", "keyword" },		{ "case", "keyword" },
			  { "default", "keyword" },	 { "void", "keyword" },			{ "int", "keyword2" },
			  { "short", "keyword2" },	 { "byte", "keyword2" },		{ "long", "keyword2" },
			  { "float", "keyword2" },	 { "double", "keyword2" },		{ "char", "keyword2" },
			  { "boolean", "keyword2" }, { "true", "literal" },			{ "false", "literal" },
			  { "null", "literal" },
		  },
		  "//" } );
}

static void addYAML() {
	SyntaxDefinitionManager::instance()->add(
		{ "YAML",
		  { "%.yml$", "%.yaml$" },
		  {
			  { { "#", "\n" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "%-?%.inf" }, "number" },
			  { { "%.NaN" }, "number" },
			  { { "(%&)(%g+)" }, { "keyword", "literal", "" } },
			  { { "!%g+" }, "keyword" },
			  { { "<<" }, "literal" },
			  { { "https?://[%w_.~!*:@&+$/?%%#-]-%w[-.%w]*%.%w%w%w?%w?:?%d*/?[%w_.~!*:@&+$/"
				  "?%%#=-]*" },
				"link" },
			  { { "([%s]%*)([%w%d_]+)" }, { "keyword", "keyword", "keyword2" } },
			  { { "(%*)([%w%d_]+)" }, { "keyword", "keyword", "literal" } },
			  { { "([%[%{])(%s*[%w%d]+%g+%s*)(:%s)" },
				{ "keyword", "operator", "operator", "keyword" } },
			  { { "([%s][%w%d]+%g+%s*)(:%s)" }, { "keyword", "keyword", "operator" } },
			  { { "([%w%d]+%g+%s*)(:%s)" }, { "keyword", "keyword", "operator" } },
			  { { "0%d+" }, "number" },
			  { { "0x%x+" }, "number" },
			  { { "[%+%-]?%d+[,%.eE:%+%d]*%d+" }, "number" },
			  { { "[%*%|%!>%%]" }, "keyword" },
			  { { "[%-:%?%*%{%}%[%]]" }, "operator" },
			  { { "([%d%a_][%g_]*)([%]%},])" }, { "string", "operator", "operator" } },
			  { { "[%d%a$/_][%g_]*" }, "string" },
		  },
		  { { "true", "number" }, { "false", "number" }, { "y", "number" }, { "n", "number" } },
		  "#" } );
}

static void addObjetiveC() {
	SyntaxDefinitionManager::instance()->add(
		{ "Objective-C",
		  { "%.m$" },
		  { { { "//.-\n" }, "comment" },
			{ { "/%*", "%*/" }, "comment" },
			{ { "#", "[^\\]\n" }, "comment" },
			{ { "\"", "\"", "\\" }, "string" },
			{ { "'", "'", "\\" }, "string" },
			{ { "-?0x%x+" }, "number" },
			{ { "-?%d+[%d%.eE]*f?" }, "number" },
			{ { "-?%.?%d+f?" }, "number" },
			{ { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
			{ { "[%a_][%w_]*%f[(]" }, "function" },
			{ { "@[%a_][%w_]*" }, "keyword2" },
			{ { "[%a_][%w_]*" }, "symbol" } },
		  { { "if", "keyword" },	  { "then", "keyword" },	{ "else", "keyword" },
			{ "elseif", "keyword" },  { "do", "keyword" },		{ "while", "keyword" },
			{ "for", "keyword" },	  { "break", "keyword" },	{ "continue", "keyword" },
			{ "return", "keyword" },  { "goto", "keyword" },	{ "struct", "keyword" },
			{ "union", "keyword" },	  { "typedef", "keyword" }, { "enum", "keyword" },
			{ "extern", "keyword" },  { "static", "keyword" },	{ "volatile", "keyword" },
			{ "const", "keyword" },	  { "inline", "keyword" },	{ "switch", "keyword" },
			{ "case", "keyword" },	  { "default", "keyword" }, { "auto", "keyword" },
			{ "const", "keyword" },	  { "void", "keyword" },	{ "int", "keyword2" },
			{ "short", "keyword2" },  { "long", "keyword2" },	{ "float", "keyword2" },
			{ "double", "keyword2" }, { "char", "keyword2" },	{ "unsigned", "keyword2" },
			{ "bool", "keyword2" },	  { "true", "literal" },	{ "false", "literal" },
			{ "NULL", "literal" },	  { "nil", "literal" } },
		  "//" } );
}

static void addDart() {
	SyntaxDefinitionManager::instance()->add(
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
		  { { "await", "keyword" },	   { "bool", "keyword2" },	  { "break", "keyword" },
			{ "case", "keyword" },	   { "class", "keyword" },	  { "const", "keyword" },
			{ "continue", "keyword" }, { "default", "keyword" },  { "do", "keyword" },
			{ "double", "keyword2" },  { "dynamic", "keyword2" }, { "else", "keyword" },
			{ "enum", "keyword" },	   { "false", "literal" },	  { "final", "keyword" },
			{ "finally", "keyword" },  { "for", "keyword" },	  { "Function", "keyword2" },
			{ "if", "keyword" },	   { "in", "keyword" },		  { "int", "keyword2" },
			{ "List", "keyword2" },	   { "Map", "keyword2" },	  { "new", "keyword" },
			{ "null", "literal" },	   { "part of", "keyword" },  { "print", "keyword" },
			{ "return", "keyword" },   { "static", "keyword" },	  { "String", "keyword2" },
			{ "switch", "keyword" },   { "then", "keyword" },	  { "this", "keyword2" },
			{ "true", "literal" },	   { "void", "keyword" },	  { "while", "keyword" },
			{ "var", "keyword" } },
		  "//" } );
}

static void addNim() {
	std::vector<SyntaxPattern> nim_patterns;
	std::unordered_map<std::string, std::string> nim_symbols;

	const std::vector<std::string> nim_number_patterns = {
		"0[bB][01][01_]*",	  "0o[0-7][0-7_]*",
		"0[xX]%x[%x_]*",	  "%d[%d_]*%.%d[%d_]*[eE][-+]?%d[%d_]*",
		"%d[%d_]*%.%d[%d_]*", "%d[%d_]*",
	};

	std::vector<std::string> nim_type_suffix_patterns;
	const std::vector<std::string> nim_num_number_list = { "", "8", "16", "32", "64" };
	for ( auto& num : nim_num_number_list )
		nim_type_suffix_patterns.push_back( "'?[fuiFUI]" + num );

	for ( const auto& pattern : nim_number_patterns ) {
		for ( const auto& suffix : nim_type_suffix_patterns )
			nim_patterns.push_back( { { pattern + suffix }, "literal" } );
		nim_patterns.push_back( { { pattern }, "literal" } );
	}

	const std::vector<std::string> nim_keywords{
		"addr",		 "and",		"as",	 "asm",		 "bind",	  "block",	"break",   "case",
		"cast",		 "concept", "const", "continue", "converter", "defer",	"discard", "distinct",
		"div",		 "do",		"elif",	 "else",	 "end",		  "enum",	"except",  "export",
		"finally",	 "for",		"from",	 "func",	 "if",		  "import", "in",	   "include",
		"interface", "is",		"isnot", "iterator", "let",		  "macro",	"method",  "mixin",
		"mod",		 "not",		"notin", "object",	 "of",		  "or",		"out",	   "proc",
		"ptr",		 "raise",	"ref",	 "return",	 "shl",		  "shr",	"static",  "template",
		"try",		 "tuple",	"type",	 "using",	 "var",		  "when",	"while",   "xor",
		"yield",
	};

	for ( const auto& keyword : nim_keywords )
		nim_symbols[keyword] = "keyword";

	const std::vector<std::string> nim_standard_types{
		"bool",	   "byte",		  "int",	 "int8",	"int16",   "int32",		 "int64",
		"uint",	   "uint8",		  "uint16",	 "uint32",	"uint64",  "float",		 "float32",
		"float64", "char",		  "string",	 "cstring", "pointer", "typedesc",	 "void",
		"auto",	   "any",		  "untyped", "typed",	"clong",   "culong",	 "cchar",
		"cschar",  "cshort",	  "cint",	 "csize",	"csize_t", "clonglong",	 "cfloat",
		"cdouble", "clongdouble", "cuchar",	 "cushort", "cuint",   "culonglong", "cstringArray",
	};

	for ( const auto& keyword : nim_standard_types )
		nim_symbols[keyword] = "keyword2";

	const std::vector<std::string> nim_standard_generic_types{
		"range", "array", "open[aA]rray", "varargs", "seq", "set", "sink", "lent", "owned",
	};

	for ( const auto& type : nim_standard_generic_types ) {
		nim_patterns.push_back( { { type + "%f[%[]" }, "keyword2" } );
		nim_patterns.push_back( { { type + "+%f[%w]" }, "keyword2" } );
	}

	const std::vector<SyntaxPattern> nim_user_patterns{
		{ { "##?%[", "]##?" }, "comment" },
		{ { "##?.-\n" }, "comment" },
		{ { "\"", "\"", "\\" }, "string" },
		{ { "\"\"\"", "\"\"\"[^\"]" }, "string" },
		{ { "'", "'", "\\" }, "literal" },
		{ { "[a-zA-Z][a-zA-Z0-9_]*%f[(]" }, "function" },
		{ { "[A-Z][a-zA-Z0-9_]*" }, "keyword2" },
		{ { "[a-zA-Z][a-zA-Z0-9_]*" }, "symbol" },
		{ { "%.%f[^.]" }, "normal" },
		{ { ":%f[ ]" }, "normal" },
		{ { "[=+%-*/<>@$~&%%|!?%^&.:\\]+" }, "operator" },
	};

	nim_patterns.insert( nim_patterns.end(), nim_user_patterns.begin(), nim_user_patterns.end() );

	SyntaxDefinitionManager::instance()->add( {
		"Nim",
		{ "%.nim$", "%.nims$", "%.nimble$" },
		nim_patterns,
		nim_symbols,
		"#",
	} );
}

static void addCMake() {
	std::unordered_map<std::string, std::string> cmake_symbols;
	const std::vector<std::string> cmake_keywords{
		"ANDROID",		  "APPLE",	   "BORLAND",		 "CACHE",	   "CYGWIN",	  "ENV",
		"GHSMULTI",		  "IOS",	   "MINGW",			 "MSVC",	   "MSVC10",	  "MSVC11",
		"MSVC12",		  "MSVC14",	   "MSVC60",		 "MSVC70",	   "MSVC71",	  "MSVC80",
		"MSVC90",		  "MSYS",	   "UNIX",			 "WIN32",	   "WINCE",		  "XCODE",
		"ABSTRACT",		  "ADVANCED",  "AUTOMOC",		 "AUTORCC",	   "AUTOUIC",	  "BUNDLE",
		"COST",			  "DEPENDS",   "DEPRECATION",	 "DISABLED",   "ENVIRONMENT", "EchoString",
		"FOLDER",		  "FRAMEWORK", "GENERATED",		 "GNUtoMS",	   "HELPSTRING",  "IMPORTED",
		"KEEP_EXTENSION", "LABELS",	   "LANGUAGE",		 "LOCATION",   "MACROS",	  "MEASUREMENT",
		"MODIFIED",		  "NAME",	   "PREFIX",		 "PROCESSORS", "RESOURCE",	  "SOURCES",
		"SOVERSION",	  "STRINGS",   "SUBDIRECTORIES", "SUFFIX",	   "SYMBOLIC",	  "TESTS",
		"TIMEOUT",		  "TYPE",	   "VALUE",			 "VARIABLES",  "VERSION",	  "XCTEST" };

	for ( const auto& keyword : cmake_keywords )
		cmake_symbols[keyword] = "keyword2";

	const std::vector<std::string> cmake_operators{ "AND", "OR", "NOT", "EQUAL", "MATCHES" };

	for ( const auto& keyword : cmake_operators )
		cmake_symbols[keyword] = "operator";

	const std::vector<std::string> cmake_literals{
		"TRUE",		   "FALSE",		"INTERFACE", "C",		"CXX",	   "EXPR",	  "LIBTYPE",
		"ON",		   "OFF",		"WARNING",	 "REPLACE", "REGEX",   "APPEND",  "DEFINED",
		"TOUPPER",	   "TOLOWER",	"STREQUAL",	 "GLOB",	"LIBRARY", "RUNTIME", "ARCHIVE",
		"DESTINATION", "IMMEDIATE", "TARGET",	 "COMMAND", "STATUS" };

	for ( const auto& keyword : cmake_literals )
		cmake_symbols[keyword] = "literal";

	SyntaxDefinitionManager::instance()->add( { "CMake",
												{ "%.cmake$", "CMakeLists.txt$" },
												{
													{ { "#", "[^\\]\n" }, "comment" },
													{ { "\"", "\"", "\\" }, "string" },
													{ { "'", "'", "\\" }, "string" },
													{ { "[%a_][%w_]*%s?%f[(]" }, "function" },
													{ { "CMAKE_[%w%d_]+" }, "keyword" },
													{ { "CTEST_[%w%d_]+" }, "keyword" },
													{ { "%u[%u%d_]*_[%u%d_]+" }, "keyword" },
													{ { "%${[%a_][%w_]*%}" }, "keyword2" },
													{ { "[%a_][%w_]*" }, "symbol" },
												},
												cmake_symbols,
												"//" } );
}

static void addJSX() {
	SyntaxDefinitionManager::instance()->add(
		{ "JSX",
		  { "%.jsx$" },
		  {
			  { { "//.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "`", "`", "\\" }, "string" },
			  { { "/[%+%-%*%^%!%=%&%|%?%:%;%,%(%[%{%<%>%\\].*%f[/]",
				  "/[igmsuyd\n]?[igmsuyd\n]?[igmsuyd\n]?", "\\" },
				"string" },
			  { { "%f[^<]![%a_][%w%_%-]*" }, "keyword2" },
			  { { "%f[^<][%a_][%w%_%-]*" }, "function" },
			  { { "%f[^<]/[%a_][%w%_%-]*" }, "function" },
			  { { "([%a_-][%w-_]*)(%\?\?)(=)%f[%{%\"]" },
				{ "normal", "keyword", "normal", "operator" } },
			  { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
			  { { "0x[%da-fA-F]+" }, "number" },
			  { { "-?%d+[%d%.eE]*" }, "number" },
			  { { "-?%.?%d+" }, "number" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },
		  },
		  {
			  { "async", "keyword" },	   { "await", "keyword" },		{ "break", "keyword" },
			  { "case", "keyword" },	   { "catch", "keyword" },		{ "class", "keyword" },
			  { "const", "keyword" },	   { "continue", "keyword" },	{ "debugger", "keyword" },
			  { "default", "keyword" },	   { "delete", "keyword" },		{ "do", "keyword" },
			  { "else", "keyword" },	   { "export", "keyword" },		{ "extends", "keyword" },
			  { "finally", "keyword" },	   { "for", "keyword" },		{ "function", "keyword" },
			  { "get", "keyword" },		   { "if", "keyword" },			{ "import", "keyword" },
			  { "in", "keyword" },		   { "instanceof", "keyword" }, { "let", "keyword" },
			  { "new", "keyword" },		   { "return", "keyword" },		{ "set", "keyword" },
			  { "static", "keyword" },	   { "super", "keyword" },		{ "switch", "keyword" },
			  { "throw", "keyword" },	   { "try", "keyword" },		{ "typeof", "keyword" },
			  { "var", "keyword" },		   { "void", "keyword" },		{ "while", "keyword" },
			  { "with", "keyword" },	   { "yield", "keyword" },		{ "true", "literal" },
			  { "false", "literal" },	   { "null", "literal" },		{ "undefined", "literal" },
			  { "arguments", "keyword2" }, { "Infinity", "keyword2" },	{ "NaN", "keyword2" },
			  { "this", "keyword2" },
		  },
		  "//" } );
}

static void addContainerfile() {
	SyntaxDefinitionManager::instance()->add( { "Containerfile",
												{ "^[Cc]ontainerfile$", "^[dD]ockerfile$" },
												{ { { "#.*\n" }, "comment" },
												  { { "%[", "%]" }, "string" },
												  { { "%sas%s" }, "literal" },
												  { { "--platform=" }, "literal" },
												  { { "--chown=" }, "literal" },
												  { { "[%a_][%w_]*" }, "symbol" } },
												{
													{ "FROM", "keyword" },
													{ "ARG", "keyword2" },
													{ "ENV", "keyword2" },
													{ "RUN", "keyword2" },
													{ "ADD", "keyword2" },
													{ "COPY", "keyword2" },
													{ "WORKDIR", "keyword2" },
													{ "USER", "keyword2" },
													{ "LABEL", "keyword2" },
													{ "EXPOSE", "keyword2" },
													{ "VOLUME", "keyword2" },
													{ "ONBUILD", "keyword2" },
													{ "STOPSIGNAL", "keyword2" },
													{ "HEALTHCHECK", "keyword2" },
													{ "SHELL", "keyword2" },
													{ "ENTRYPOINT", "function" },
													{ "CMD", "function" },
												},
												"#",
												{},
												"dockerfile" } );
}

static void addIgnore() {
	SyntaxDefinitionManager::instance()->add( { ".ignore file",
												{ "%..*ignore$" },
												{
													{ { "^%s*#.*$" }, "comment" },
													{ { "^%!.*$" }, "keyword" },
												},
												{},
												"#" } );
}

static void addPowerShell() {
	SyntaxDefinitionManager::instance()->add(
		{ "PowerShell",
		  { "%.ps1$", "%.psm1$", "%.psd1$", "%.ps1xml$", "%.pssc$", "%.psrc$", "%.cdxml$" },
		  { { { "#.*\n" }, "comment" },
			{ { "[[\\.]]" }, "normal" },
			{ { "\"", "\"" }, "string" },
			{ { "'", "'" }, "string" },
			{ { "%f[%w_][%d%.]+%f[^%w_]" }, "number" },
			{ { "[%+=/%*%^%%<>!~|&,:]+" }, "operator" },
			{ { "%f[%S]%-[%w%-%_]+" }, "function" },
			{ { "[%u][%a]+[%-][%u][%a]+" }, "function" },
			{ { "${.*}" }, "symbol" },
			{ { "$[%a_@*][%w_]*" }, "keyword2" },
			{ { "$[%$][%a]+" }, "keyword2" },
			{ { "[%a_][%w_]*" }, "symbol" } },
		  { { "if", "keyword" },
			{ "else", "keyword" },
			{ "elseif", "keyword" },
			{ "switch", "keyword" },
			{ "default", "keyword" },
			{ "function", "keyword" },
			{ "filter", "keyword" },
			{ "workflow", "keyword" },
			{ "configuration", "keyword" },
			{ "class", "keyword" },
			{ "enum", "keyword" },
			{ "Parameter", "keyword" },
			{ "ValidateScript", "keyword" },
			{ "CmdletBinding", "keyword" },
			{ "try", "keyword" },
			{ "catch", "keyword" },
			{ "finally", "keyword" },
			{ "throw", "keyword" },
			{ "while", "keyword" },
			{ "for", "keyword" },
			{ "do", "keyword" },
			{ "until", "keyword" },
			{ "break", "keyword" },
			{ "continue", "keyword" },
			{ "foreach", "keyword" },
			{ "in", "keyword" },
			{ "return", "keyword" },
			{ "where", "function" },
			{ "select", "function" },
			{ "filter", "keyword" },
			{ "trap", "keyword" },
			{ "param", "keyword" },
			{ "data", "keyword" },
			{ "dynamicparam", "keyword" },
			{ "begin", "function" },
			{ "process", "function" },
			{ "end", "function" },
			{ "exit", "function" },
			{ "inlinescript", "function" },
			{ "parallel", "function" },
			{ "sequence", "function" },
			{ "true", "literal" },
			{ "false", "literal" },
			{ "TODO", "comment" },
			{ "FIXME", "comment" },
			{ "XXX", "comment" },
			{ "TBD", "comment" },
			{ "HACK", "comment" },
			{ "NOTE", "comment" } },
		  "#" } );
}

static void addWren() {
	SyntaxDefinitionManager::instance()->add(
		{ "Wren",
		  { "%.wren$" },
		  {
			  { { "//.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "-?%.?%d+" }, "number" },
			  { { "%.%.%.?" }, "operator" },
			  { { "[<>!=]=" }, "operator" },
			  { { "[%+%-=/%*%^%%<>!~|&?:]" }, "operator" },
			  { { "[%a_][%w_]*%s*%f[(\"{]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },
		  },
		  {
			  { "break", "keyword" }, { "class", "keyword" },  { "construct", "keyword" },
			  { "else", "keyword" },  { "for", "keyword" },	   { "foreign", "keyword" },
			  { "if", "keyword" },	  { "import", "keyword" }, { "in", "keyword" },
			  { "is", "keyword" },	  { "return", "keyword" }, { "static", "keyword" },
			  { "super", "keyword" }, { "var", "keyword" },	   { "while", "keyword" },
			  { "this", "keyword2" }, { "true", "literal" },   { "false", "literal" },
			  { "null", "literal" },
		  },
		  "//" } );
}

static void addEnv() {
	SyntaxDefinitionManager::instance()->add(
		{ "Environment File",
		  { "%.env$", "%.env.[%w%-%_]*$" },
		  { { { "^#.-\n" }, "comment" },
			{ { "%s#.-\n" }, "comment" },
			{ { "\\[nrtfb\\\"']" }, "literal" },
			{ { "'?\\u%x%x%x%x'?" }, "literal" },
			{ { "(%${)([%w]+[%w_]*)(})" }, { "keyword", "keyword", "keyword2", "keyword" } },
			{ { "%$[%w]+[%w_]*" }, "keyword2" },
			{ { "[%a_][%w-+_%s%p]-%f[=]" }, "keyword" },
			{ { "\"", "\"", "\\" }, "string" },
			{ { "'", "'", "\\" }, "string" },
			{ { "^%[.-%]" }, "keyword2" },
			{ { "%s%[.-%]" }, "keyword2" },
			{ { "=" }, "operator" },
			{ { "https?://[%w_.~!*:@&+$/?%%#-]-%w[-.%w]*%.%w%w%w?%w?:?%d*/?[%w_.~!*:@&+$/"
				"?%%#=-]*" },
			  "link" },
			{ { "[a-z]+" }, "symbol" } },
		  { { "true", "literal" },
			{ "false", "literal" },
			{ "export", "literal" },
			{ "null", "literal" } },
		  "#" } );
}

static void addRuby() {
	SyntaxDefinitionManager::instance()->add( {
		"Ruby",
		{ "%.rb", "%.gemspec", "%.ruby" },
		{
			{ { "\"", "\"", "\\" }, "string" },
			{ { "'", "'", "\\" }, "string" },
			{ { "-?0x%x+" }, "number" },
			{ { "%#.-\n" }, "comment" },
			{ { "-?%d+[%d%.eE]*f?" }, "number" },
			{ { "-?%.?%d+f?" }, "number" },
			{ { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
			{ { "[%a_][%w_]*%f[(]" }, "function" },
			{ { "@?@[%a_][%w_]*" }, "keyword2" },
			{ { "::[%w_]*" }, "symbol" },
			{ { ":[%w_]*" }, "keyword2" },
			{ { "[%a_][%w_]*:[^:]" }, "keyword2" },
			{ { "[%a_][%w_]*" }, "symbol" },
		},
		{
			{ "nil", "literal" },
			{ "end", "literal" },
			{ "true", "literal" },
			{ "false", "literal" },
			{ "private", "keyword" },
			{ "extend", "keyword" },
			{ "include", "keyword" },
			{ "require", "keyword" },
			{ "require_dependency", "keyword" },
			{ "__ENCODING__", "keyword" },
			{ "__LINE__", "keyword" },
			{ "__FILE__", "keyword" },
			{ "BEGIN", "keyword" },
			{ "END", "keyword" },
			{ "alias", "keyword" },
			{ "and", "keyword" },
			{ "begin", "keyword" },
			{ "break", "keyword" },
			{ "case", "keyword" },
			{ "class", "keyword" },
			{ "def", "keyword" },
			{ "defined?", "keyword" },
			{ "do", "keyword" },
			{ "else", "keyword" },
			{ "elsif", "keyword" },
			{ "ensure", "keyword" },
			{ "for", "keyword" },
			{ "if", "keyword" },
			{ "in", "keyword" },
			{ "module", "keyword" },
			{ "next", "keyword" },
			{ "not", "keyword" },
			{ "or", "keyword" },
			{ "redo", "keyword" },
			{ "rescue", "keyword" },
			{ "retry", "keyword" },
			{ "return", "keyword" },
			{ "self", "keyword" },
			{ "super", "keyword" },
			{ "then", "keyword" },
			{ "undef", "keyword" },
			{ "unless", "keyword" },
			{ "until", "keyword" },
			{ "when", "keyword" },
			{ "while", "keyword" },
			{ "yield", "keyword" },
		},
		"#",
		{ "^#!.*[ /]ruby" },
	} );
}

static void addScala() {
	SyntaxDefinitionManager::instance()->add( {
		"Scala",
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
			{ "abstract", "keyword" },	 { "case", "keyword" },		{ "catch", "keyword" },
			{ "class", "keyword" },		 { "finally", "keyword" },	{ "final", "keyword" },
			{ "do", "keyword" },		 { "extends", "keyword" },	{ "forSome", "keyword" },
			{ "implicit", "keyword" },	 { "lazy", "keyword" },		{ "match", "keyword" },
			{ "new", "keyword" },		 { "override", "keyword" }, { "package", "keyword" },
			{ "throw", "keyword" },		 { "trait", "keyword" },	{ "type", "keyword" },
			{ "var", "keyword" },		 { "val", "keyword" },		{ "println", "keyword" },
			{ "return", "keyword" },	 { "for", "keyword" },		{ "Try", "keyword" },
			{ "def", "keyword" },		 { "while", "keyword" },	{ "with", "keyword" },
			{ "if", "keyword" },		 { "else", "keyword" },		{ "import", "keyword" },
			{ "object", "keyword" },	 { "yield", "keyword" },	{ "private", "keyword2" },
			{ "protected", "keyword2" }, { "sealed", "keyword2" },	{ "super", "keyword2" },
			{ "this", "keyword2" },		 { "Byte", "keyword2" },	{ "Short", "keyword2" },
			{ "Int", "keyword2" },		 { "Long", "keyword2" },	{ "Float", "keyword2" },
			{ "Double", "keyword2" },	 { "Char", "keyword2" },	{ "String", "keyword2" },
			{ "List", "keyword2" },		 { "Array", "keyword2" },	{ "Boolean", "keyword2" },
			{ "Null", "literal" },		 { "Any", "literal" },		{ "AnyRef", "literal" },
			{ "Nothing", "literal" },	 { "Unit", "literal" },		{ "true", "literal" },
			{ "false", "literal" },
		},
		"//",
	} );
}

static void addxit() {
	SyntaxDefinitionManager::instance()->add( {
		"[x]it!",
		{ "%.xit$" },
		{
			{ { "%f[^%s%(]%-%>%s%d%d%d%d%-%d%d%-%d%d%f[\n%s%!%?%)]" }, "number" },
			{ { "%f[^%s%(]%-%>%s%d%d%d%d%/%d%d%/%d%d%f[\n%s%!%?%)]" }, "number" },
			{ { "%f[^%s%(]%-%>%s%d%d%d%d%-[wWqQ]?%d%d?%f[\n%s%!%?%)]" }, "number" },
			{ { "%f[^%s%(]%-%>%s%d%d%d%d%/[wWqQ]?%d%d?%f[\n%s%!%?%)]" }, "number" },
			{ { "%f[^%s%(]%-%>%s%d%d%d%d%f[\n%s%!%?%)]" }, "number" },
			{ { "^(%[%s%]%s)([%.!]+)%s" }, { "operator", "operator", "red" } },
			{ { "^(%[x%]%s)([%.!]+)%s" }, { "function", "function", "red" } },
			{ { "^(%[@%]%s)([%.!]+)%s" }, { "keyword", "keyword", "red" } },
			{ { "^(%[~%]%s)([%.!]+)%s" }, { "comment", "comment", "red" } },
			{ { "%#[%wñàáâãäåèéêëìíîïòóôõöùúûüýÿÑÀÁÂÃÄÅÈÉÊËÌÍÎÏÒÓÔÕÖÙÚÛÜÝ%-%_]+%=\"", "\"" },
			  "string" },
			{ { "%#[%wñàáâãäåèéêëìíîïòóôõöùúûüýÿÑÀÁÂÃÄÅÈÉÊËÌÍÎÏÒÓÔÕÖÙÚÛÜÝ%-%_]+%='", "'" },
			  "string" },
			{ { "%#[%wñàáâãäåèéêëìíîïòóôõöùúûüýÿÑÀÁÂÃÄÅÈÉÊËÌÍÎÏÒÓÔÕÖÙÚÛÜÝ%-%_]+%=[%w%-%_]*" },
			  "string" },
			{ { "%#[%wñàáâãäåèéêëìíîïòóôõöùúûüýÿÑÀÁÂÃÄÅÈÉÊËÌÍÎÏÒÓÔÕÖÙÚÛÜÝ%-%_]+" }, "string" },
			{ { "^%[%s%]%s" }, "operator" },
			{ { "^%[x%]%s" }, "function" },
			{ { "^%[@%]%s" }, "keyword" },
			{ { "^%[~%]%s" }, "comment" },
			{ { "^%[%?%]%s" }, "warning" },
			{ { "^[%wñàáâãäåèéêëìíîïòóôõöùúûüýÿÑÀÁÂÃÄÅÈÉÊËÌÍÎÏÒÓÔÕÖÙÚÛÜÝ][%w"
				"ñàáâãäåèéêëìíîïòóôõöùúûüýÿÑÀÁÂÃÄÅÈÉÊËÌÍÎÏÒÓÔÕÖÙÚÛÜÝ%s%p]*%f[\n]" },
			  "underline" },
			{ { "https?://[%w_.~!*:@&+$/?%%#-]-%w[-.%w]*%.%w%w%w?%w?:?%d*/?[%w_.~!*:@&+$/"
				"?%%#=-]*" },
			  "link" },
		},
		{},
		"",
	} );
}

static void addVue() {
	SyntaxDefinitionManager::instance()
		->add( { "Vue-HTML",
				 {},
				 {
					 { { "%{%{%{", "%}%}%}" }, "function", "JavaScript" },
					 { { "%{%{", "%}%}" }, "function", "JavaScript" },
					 { { "<!%-%-", "%-%->" }, "comment" },
					 { { "%f[^>][^<]", "%f[<]" }, "normal" },
					 { { "\"", "\"", "\\" }, "string" },
					 { { "'", "'", "\\" }, "string" },
					 { { "0x[%da-fA-F]+" }, "number" },
					 { { "-?%d+[%d%.]*f?" }, "number" },
					 { { "-?%.?%d+f?" }, "number" },
					 { { "%f[^<]![%a_][%w%_%-]*" }, "keyword2" },
					 { { "%f[^<][%a_][%w%_%-]*" }, "function" },
					 { { "%f[^<]/[%a_][%w%_%-]*" }, "function" },
					 { { "[%a_][%w_]*" }, "keyword" },
					 { { "[/<>=]" }, "operator" },
				 },
				 {},
				 "",
				 {} } )
		.setVisible( false )
		.setAutoCloseXMLTags( true );

	SyntaxDefinitionManager::instance()->add(
		{ "Vue",
		  { "%.vue?$" },
		  {
			  { { "<%s*[sS][cC][rR][iI][pP][tT]%s*>", "<%s*/%s*[sS][cC][rR][iI][pP][tT]>" },
				"function",
				"JavaScript" },
			  { { "<%s*[sS][tT][yY][lL][eE][^>]*>", "<%s*/%s*[sS][tT][yY][lL][eE]%s*>" },
				"function",
				"CSS" },
			  { { "<%s*[tT][eE][mM][pP][lL][aA][tT][eE][^>]*>",
				  "<%s*/%s*[tT][eE][mM][pP][lL][aA][tT][eE]%s*>" },
				"function",
				"Vue-HTML" },
			  { { "<!%-%-", "%-%->" }, "comment" },
			  { { "%f[^>][^<]", "%f[<]" }, "normal" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "0x[%da-fA-F]+" }, "number" },
			  { { "-?%d+[%d%.]*f?" }, "number" },
			  { { "-?%.?%d+f?" }, "number" },
			  { { "%f[^<]![%a_][%w%_%-]*" }, "keyword2" },
			  { { "%f[^<][%a_][%w%_%-]*" }, "function" },
			  { { "%f[^<]/[%a_][%w%_%-]*" }, "function" },
			  { { "[%a_][%w_]*" }, "keyword" },
			  { { "[/<>=]" }, "operator" },
		  },
		  {},
		  "",
		  {} } );
}

static void addHaxe() {
	SyntaxDefinitionManager::instance()->add(
		{ "Haxe Compiler Arguments",
		  { "%.hxml$" },
		  {
			  { { "#.*" }, "comment" },
			  { { "%-[%-%w_]*" }, "keyword" },
			  { { "(%.)(%u[%w_]*)" }, { "normal", "normal", "keyword2" } },
			  { { "%s+" }, "normal" },
			  { { "%w+%f[%s]" }, "normal" },
		  },
		  {},
		  "#" } );

	SyntaxDefinitionManager::instance()
		->add( { "HaxeStringInterpolation",
				 {},
				 {
					 { { "%${", "}", "\\" }, "keyword", ".hx" },
					 { { "%$", "%s", "\\" }, "keyword", ".hx" },
					 { { "[^ ]" }, "string" },
					 { { "%s+" }, "normal" },
					 { { "%w+%f[%s]" }, "normal" },
				 },
				 {} } )
		.setVisible( false );

	SyntaxDefinitionManager::instance()
		->add( { "HaxeRegularExpressions",
				 {},
				 {
					 { { "[%[%]%(%)]" }, "string" },
					 { { "[%.%*%+%?%^%$%|%-]" }, "operator" },
					 { { "%s+" }, "normal" },
					 { { "%w+%f[%s]" }, "normal" },
				 },
				 {} } )
		.setVisible( false );

	SyntaxDefinitionManager::instance()->add(
		{ "Haxe",
		  { "%.hx$" },
		  {
			  { { "%~%/", "%/[igmsu]*" }, "keyword2", "HaxeRegularExpressions" },
			  { { "%.%.%." }, "operator" },
			  { { "(%<)(%u[%w_]*)(%>*)" }, { "normal", "operator", "keyword2", "operator" } },
			  { { "(%#%s*[%a_]*)(.*\n)" }, { "normal", "keyword", "normal" } },
			  { { "(import%s+)(%u[%w]*)" }, { "normal", "keyword", "keyword2" } },
			  { { "(import%s+)([%w%.]*%.)(%u[%w]*)" },
				{ "normal", "keyword", "normal", "keyword2" } },
			  { { "(abstract%s+)(%u[%w_]*%s*%()(%s*%u[%w_]*)" },
				{ "normal", "keyword2", "normal", "keyword2" } },
			  { { "(from%s+)(%u[%w_]*%s+)(to%s+)(%u[%w_]*)" },
				{ "normal", "keyword", "keyword2", "keyword", "keyword2" } },
			  { { "//.*\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string", "HaxeStringInterpolation" },
			  { { "-?%.?%d+" }, "number" },
			  { { "-?0x%x+" }, "number" },
			  { { "-?%d+%.[%deE]+" }, "number" },
			  { { "-?%d+[%deE]+" }, "number" },
			  { { "[%+%-%.=/%*%^%%<>!~|&]" }, "operator" },
			  { { "([%a_][%w_]*)(%s*%f[(])" }, { "normal", "function", "normal" } },
			  { { "[%a_][%w_]*" }, "symbol" },
			  { { "(:)(%u[%a_][%w_]*)" }, { "normal", "normal", "keyword2" } },
			  { { "@:[%a_][%w_]*%f[(]" }, "keyword" },
			  { { "%$type" }, "keyword" },
			  { { "%s+" }, "normal" },
			  { { "%w+%f[%s]" }, "normal" },
		  },
		  {
			  { "using", "keyword2" },	  { "true", "literal" },	 { "trace", "keyword" },
			  { "throw", "keyword" },	  { "typedef", "keyword2" }, { "switch", "keyword" },
			  { "try", "keyword" },		  { "static", "keyword" },	 { "set", "keyword" },
			  { "return", "keyword" },	  { "public", "keyword" },	 { "package", "keyword" },
			  { "do", "keyword" },		  { "default", "keyword" },	 { "new", "keyword" },
			  { "private", "keyword" },	  { "macro", "keyword2" },	 { "cast", "keyword" },
			  { "class", "keyword" },	  { "case", "keyword" },	 { "this", "keyword" },
			  { "continue", "keyword" },  { "else", "keyword" },	 { "extern", "keyword2" },
			  { "break", "keyword" },	  { "extends", "keyword2" }, { "interface", "keyword" },
			  { "abstract", "keyword2" }, { "for", "keyword" },		 { "override", "keyword" },
			  { "function", "keyword2" }, { "never", "keyword" },	 { "get", "keyword" },
			  { "final", "keyword" },	  { "if", "keyword" },		 { "implements", "keyword2" },
			  { "var", "keyword2" },	  { "catch", "keyword" },	 { "import", "keyword" },
			  { "false", "literal" },	  { "in", "keyword" },		 { "while", "keyword" },
			  { "inline", "keyword" },	  { "enum", "keyword" },	 { "null", "literal" },

		  },
		  "//",
		  {} } );
}

// Syntax definitions can be directly converted from the lite (https://github.com/rxi/lite) and
// lite-plugins (https://github.com/rxi/lite-plugins) supported languages.

SyntaxDefinitionManager::SyntaxDefinitionManager() {
	if ( ms_singleton == nullptr )
		ms_singleton = this;

	mDefinitions.reserve( 66 );

	// Register some languages support.
	addPlainText();
	addAngelScript();
	addBash();
	addBatchScript();
	addC();
	addCMake();
	addContainerfile();
	addCPP();
	addCrystal();
	addCSharp();
	addCSS();
	addD();
	addDart();
	addDiff();
	addElixir();
	addElm();
	addEnv();
	addFstab();
	addGDScript();
	addGLSL();
	addGo();
	addHaskell();
	addHare();
	addHaxe();
	addHLSL();
	addHtaccessFile();
	addHTML();
	addIgnore();
	addIni();
	addJava();
	addJavaScript();
	addJulia();
	addJSON();
	addJSX();
	addKotlin();
	addLatex();
	addLua();
	addMakefile();
	addMarkdown();
	addMeson();
	addNelua();
	addNim();
	addObjeck();
	addObjetiveC();
	addOdin();
	addPascal();
	addPerl();
	addPICO8();
	addPHP();
	addPO();
	addPostgreSQL();
	addPowerShell();
	addPython();
	addR();
	addRuby();
	addRust();
	addSass();
	addScala();
	addSolidity();
	addSQL();
	addSwift();
	addTeal();
	addToml();
	addTypeScript();
	addV();
	addVerilog();
	addVisualBasic();
	addVue();
	addWren();
	addX86Assembly();
	addxit();
	addXML();
	addYAML();
	addZig();
}

const std::vector<SyntaxDefinition>& SyntaxDefinitionManager::getDefinitions() const {
	return mDefinitions;
}

static json toJson( const SyntaxDefinition& def ) {
	json j;
	j["name"] = def.getLanguageName();
	if ( def.getLSPName() != String::toLower( def.getLanguageName() ) )
		j["lsp_name"] = def.getLSPName();
	j["files"] = def.getFiles();
	if ( !def.getComment().empty() )
		j["comment"] = def.getComment();
	if ( !def.getPatterns().empty() ) {
		j["patterns"] = json::array();
		for ( const auto& ptrn : def.getPatterns() ) {
			json pattern;
			if ( ptrn.patterns.size() == 1 ) {
				pattern["pattern"] = ptrn.patterns[0];
			} else {
				pattern["pattern"] = ptrn.patterns;
			}
			if ( ptrn.types.size() == 1 ) {
				pattern["type"] = ptrn.types[0];
			} else {
				pattern["type"] = ptrn.types;
			}
			if ( !ptrn.syntax.empty() )
				pattern["syntax"] = ptrn.syntax;
			j["patterns"].emplace_back( std::move( pattern ) );
		}
	}
	if ( !def.getSymbols().empty() ) {
		j["symbols"] = json::array();
		for ( const auto& sym : def.getSymbols() )
			j["symbols"].emplace_back( json{ json{ sym.first, sym.second } } );
	}

	if ( !def.getHeaders().empty() )
		j["headers"] = def.getHeaders();

	if ( def.getAutoCloseXMLTags() )
		j["auto_close_xml_tags"] = true;

	if ( !def.isVisible() )
		j["visible"] = false;

	return j;
}

bool SyntaxDefinitionManager::save( const std::string& path,
									const std::vector<SyntaxDefinition>& def ) {
	if ( def.size() == 1 ) {
		return FileSystem::fileWrite( path, toJson( def[0] ).dump( 2 ) );
	} else if ( !def.empty() ) {
		json j = json::array();
		for ( const auto& d : def )
			j.emplace_back( toJson( d ) );
		return FileSystem::fileWrite( path, j.dump( 2 ) );
	} else {
		json j = json::array();
		for ( const auto& d : mDefinitions )
			j.emplace_back( toJson( d ) );
		return FileSystem::fileWrite( path, j.dump( 2 ) );
	}
	return false;
}

std::optional<size_t> SyntaxDefinitionManager::getLanguageIndex( const std::string& langName ) {
	size_t pos = 0;
	for ( const auto& def : mDefinitions ) {
		if ( def.getLanguageName() == langName ) {
			return pos;
		}
		++pos;
	}
	return {};
}

static std::string str( std::string s, const std::string& prepend = "",
						const std::string& append = "", bool allowEmptyString = true ) {
	if ( s.empty() && !allowEmptyString )
		return "";
	String::replaceAll( s, "\\", "\\\\" );
	String::replaceAll( s, "\"", "\\\"" );
	return prepend + "\"" + String::escape( s ) + "\"" + append;
}

static std::string join( std::vector<std::string> const& vec, bool createCont = true,
						 bool allowReduce = false, std::string delim = ", " ) {
	if ( vec.empty() )
		return "{}";
	if ( vec.size() == 1 && allowReduce )
		return str( vec[0] );
	std::string accum = std::accumulate(
		vec.begin() + 1, vec.end(), str( vec[0] ),
		[&delim]( const std::string& a, const std::string& b ) { return a + delim + str( b ); } );
	return createCont ? "{ " + accum + " }" : accum;
}

static std::string funcName( std::string name ) {
	if ( name.empty() )
		return "";
	String::replaceAll( name, " ", "" );
	String::replaceAll( name, "+", "p" );
	String::replaceAll( name, "#", "sharp" );
	name[0] = std::toupper( name[0] );
	return name;
}

std::pair<std::string, std::string> SyntaxDefinitionManager::toCPP( const SyntaxDefinition& def ) {
	std::string lang( def.getLanguageNameForFileSystem() );
	std::string func( funcName( lang ) );
	std::string header = "#ifndef EE_UI_DOC_" + func + "\n#define EE_UI_DOC_" + func +
						 "\n\nnamespace EE { namespace UI { namespace "
						 "Doc { namespace Language {\n\nextern void add" +
						 func + "();\n\n}}}}\n\n#endif\n";
	std::string buf = String::format( R"cpp(#include <eepp/ui/doc/languages/%s.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {
)cpp",
									  lang.c_str() );
	buf += "\nvoid add" + func + "() {\n";
	buf += "\nSyntaxDefinitionManager::instance()->add(\n\n{";
	// lang name
	buf += str( def.getLanguageName() ) + ",\n";
	// file types
	buf += join( def.getFiles() ) + ",\n";
	// patterns
	buf += "{\n";
	for ( const auto& pattern : def.getPatterns() )
		buf += "{ " + join( pattern.patterns ) + ", " + join( pattern.types, true, true ) +
			   str( pattern.syntax, ", ", "", false ) + " },\n";
	buf += "\n},\n";
	// symbols
	buf += "{\n";
	for ( const auto& symbol : def.getSymbols() )
		buf += "{ " + str( symbol.first ) + " , " + str( symbol.second ) + " },\n";
	buf += "\n},\n";
	buf += str( def.getComment(), "", "", true ) + ",\n";
	std::string lspName =
		def.getLSPName().empty() || def.getLSPName() == String::toLower( def.getLanguageName() )
			? ""
			: def.getLSPName();
	// headers
	buf += join( def.getHeaders() ) + ( lspName.empty() ? "" : "," ) + "\n";
	// lsp
	buf += lspName.empty() ? "" : str( def.getLSPName() );
	buf += "\n}";
	buf += ")";
	if ( !def.isVisible() )
		buf += ".setVisible( false )";
	if ( def.getAutoCloseXMLTags() )
		buf += ".setAutoCloseXMLTags( true )";
	buf += ";\n}\n";
	buf += "\n}}}} // namespace EE::UI::Doc::Language\n";
	return std::make_pair( std::move( header ), std::move( buf ) );
}

SyntaxDefinition& SyntaxDefinitionManager::add( SyntaxDefinition&& syntaxStyle ) {
	mDefinitions.emplace_back( std::move( syntaxStyle ) );
	return mDefinitions.back();
}

const SyntaxDefinition& SyntaxDefinitionManager::getPlainStyle() const {
	return mDefinitions[0];
}

SyntaxDefinition& SyntaxDefinitionManager::getByExtensionRef( const std::string& filePath ) {
	return const_cast<SyntaxDefinition&>( getByExtension( filePath ) );
}

const SyntaxDefinition&
SyntaxDefinitionManager::getByLanguageName( const std::string& name ) const {
	for ( auto& style : mDefinitions ) {
		if ( style.getLanguageName() == name )
			return style;
	}
	return mDefinitions[0];
}

const SyntaxDefinition& SyntaxDefinitionManager::getByLSPName( const std::string& name ) const {
	for ( auto& style : mDefinitions ) {
		if ( style.getLSPName() == name )
			return style;
	}
	return mDefinitions[0];
}

const SyntaxDefinition&
SyntaxDefinitionManager::getByLanguageId( const String::HashType& id ) const {
	for ( auto& style : mDefinitions ) {
		if ( style.getLanguageId() == id )
			return style;
	}
	return mDefinitions[0];
}

SyntaxDefinition& SyntaxDefinitionManager::getByLanguageNameRef( const std::string& name ) {
	return const_cast<SyntaxDefinition&>( getByLanguageName( name ) );
}

std::vector<std::string> SyntaxDefinitionManager::getLanguageNames() const {
	std::vector<std::string> names;
	for ( auto& style : mDefinitions ) {
		if ( style.isVisible() )
			names.push_back( style.getLanguageName() );
	}
	std::sort( names.begin(), names.end() );
	return names;
}

std::vector<std::string> SyntaxDefinitionManager::getExtensionsPatternsSupported() const {
	std::vector<std::string> exts;
	for ( auto& style : mDefinitions )
		for ( auto& pattern : style.getFiles() )
			exts.emplace_back( pattern );
	return exts;
}

const SyntaxDefinition*
SyntaxDefinitionManager::getPtrByLanguageName( const std::string& name ) const {
	return &getByLanguageName( name );
}

const SyntaxDefinition*
SyntaxDefinitionManager::getPtrByLanguageId( const String::HashType& id ) const {
	return &getByLanguageId( id );
}

static SyntaxDefinition loadLanguage( const nlohmann::json& json ) {
	SyntaxDefinition def;
	try {
		def.setLanguageName( json.value( "name", "" ) );
		if ( json.contains( "lsp_name" ) && json["lsp_name"].is_string() )
			def.setLSPName( json["lsp_name"].get<std::string>() );
		if ( json.contains( "files" ) ) {
			if ( json["files"].is_array() ) {
				const auto& files = json["files"];
				for ( const auto& file : files ) {
					def.addFileType( file );
				}
			} else if ( json["files"].is_string() ) {
				def.addFileType( json["files"].get<std::string>() );
			}
		}
		def.setComment( json.value( "comment", "" ) );
		if ( json.contains( "patterns" ) && json["patterns"].is_array() ) {
			const auto& patterns = json["patterns"];
			for ( const auto& pattern : patterns ) {
				std::vector<std::string> type;
				if ( pattern.contains( "type" ) ) {
					if ( pattern["type"].is_array() ) {
						for ( const auto& t : pattern["type"] ) {
							if ( t.is_string() )
								type.push_back( t.get<std::string>() );
						}
					} else if ( pattern["type"].is_string() ) {
						type.push_back( pattern["type"] );
					}
				} else {
					type.push_back( "normal" );
				}
				auto syntax = !pattern.contains( "syntax" ) || !pattern["syntax"].is_string()
								  ? ""
								  : pattern.value( "syntax", "" );
				std::vector<std::string> ptrns;
				if ( pattern.contains( "pattern" ) ) {
					if ( pattern["pattern"].is_array() ) {
						const auto& ptrnIt = pattern["pattern"];
						for ( const auto& ptrn : ptrnIt )
							ptrns.emplace_back( ptrn );
					} else if ( pattern["pattern"].is_string() ) {
						ptrns.emplace_back( pattern["pattern"] );
					}
				}
				def.addPattern( SyntaxPattern( ptrns, type, syntax ) );
			}
		}
		if ( json.contains( "symbols" ) ) {
			if ( json["symbols"].is_array() ) {
				const auto& symbols = json["symbols"];
				for ( const auto& symbol : symbols ) {
					for ( auto& el : symbol.items() ) {
						def.addSymbol( el.key(), el.value() );
					}
				}
			} else if ( json["symbols"].is_object() ) {
				for ( const auto& [key, value] : json["symbols"].items() ) {
					def.addSymbol( key, value );
				}
			}
		}
		if ( json.contains( "headers" ) && json["headers"].is_array() ) {
			const auto& headers = json["headers"];
			std::vector<std::string> hds;
			if ( headers.is_array() ) {
				for ( const auto& header : headers ) {
					if ( header.is_string() )
						hds.emplace_back( header.get<std::string>() );
				}
			} else if ( headers.is_string() ) {
				hds.push_back( headers.get<std::string>() );
			}
			if ( !hds.empty() )
				def.setHeaders( hds );
		}
		if ( json.contains( "visible" ) && json["visible"].is_boolean() )
			def.setVisible( json["visible"].get<bool>() );
		if ( json.contains( "auto_close_xml_tags" ) && json["auto_close_xml_tags"].is_boolean() )
			def.setAutoCloseXMLTags( json["auto_close_xml_tags"].get<bool>() );
	} catch ( const json::exception& e ) {
		Log::error( "SyntaxDefinition loadLanguage failed:\n%s", e.what() );
	}
	return def;
}

bool SyntaxDefinitionManager::loadFromStream( IOStream& stream,
											  std::vector<std::string>* addedLangs ) {
	if ( stream.getSize() == 0 )
		return false;
	std::string buffer;
	buffer.resize( stream.getSize() );
	stream.read( buffer.data(), buffer.size() );

	nlohmann::json j = nlohmann::json::parse( buffer );

	if ( j.is_array() ) {
		for ( const auto& lang : j ) {
			auto res = loadLanguage( lang );
			if ( !res.getLanguageName().empty() ) {
				auto pos = getLanguageIndex( res.getLanguageName() );
				if ( pos.has_value() ) {
					if ( addedLangs )
						addedLangs->push_back( res.getLanguageName() );
					mDefinitions[pos.value()] = std::move( res );
				} else {
					if ( addedLangs )
						addedLangs->push_back( res.getLanguageName() );
					mDefinitions.emplace_back( std::move( res ) );
				}
			}
		}
	} else {
		auto res = loadLanguage( j );
		if ( !res.getLanguageName().empty() ) {
			auto pos = getLanguageIndex( res.getLanguageName() );
			if ( pos.has_value() ) {
				if ( addedLangs )
					addedLangs->push_back( res.getLanguageName() );
				mDefinitions[pos.value()] = std::move( res );
			} else {
				if ( addedLangs )
					addedLangs->push_back( res.getLanguageName() );
				mDefinitions.emplace_back( std::move( res ) );
			}
		}
	}

	return true;
}

bool SyntaxDefinitionManager::loadFromStream( IOStream& stream ) {
	return loadFromStream( stream, nullptr );
}

bool SyntaxDefinitionManager::loadFromFile( const std::string& fpath ) {
	if ( FileSystem::fileExists( fpath ) ) {
		IOStreamFile IOS( fpath );

		return loadFromStream( IOS );
	} else if ( PackManager::instance()->isFallbackToPacksActive() ) {
		std::string tgPath( fpath );

		Pack* tPack = PackManager::instance()->exists( tgPath );

		if ( NULL != tPack ) {
			return loadFromPack( tPack, tgPath );
		}
	}
	return false;
}

bool SyntaxDefinitionManager::loadFromMemory( const Uint8* data, const Uint32& dataSize ) {
	IOStreamMemory IOS( (const char*)data, dataSize );
	return loadFromStream( IOS );
}

bool SyntaxDefinitionManager::loadFromPack( Pack* Pack, const std::string& filePackPath ) {
	if ( NULL != Pack && Pack->isOpen() && -1 != Pack->exists( filePackPath ) ) {
		ScopedBuffer buffer;
		Pack->extractFileToMemory( filePackPath, buffer );
		return loadFromMemory( buffer.get(), buffer.length() );
	}
	return false;
}

void SyntaxDefinitionManager::loadFromFolder( const std::string& folderPath ) {
	if ( !FileSystem::isDirectory( folderPath ) )
		return;
	auto files = FileSystem::filesInfoGetInPath( folderPath );
	if ( files.empty() )
		return;
	for ( const auto& file : files ) {
		if ( file.isRegularFile() && file.isReadable() && file.getExtension() == "json" )
			loadFromFile( file.getFilepath() );
	}
}

const SyntaxDefinition& SyntaxDefinitionManager::getByExtension( const std::string& filePath,
																 bool hFileAsCPP ) const {
	std::string extension( FileSystem::fileExtension( filePath ) );
	std::string fileName( FileSystem::fileNameFromPath( filePath ) );

	// Use the filename instead
	if ( extension.empty() )
		extension = FileSystem::fileNameFromPath( filePath );

	if ( !extension.empty() ) {
		for ( const auto& style : mDefinitions ) {
			for ( const auto& ext : style.getFiles() ) {
				if ( String::startsWith( ext, "%." ) || String::startsWith( ext, "^" ) ||
					 String::endsWith( ext, "$" ) ) {
					LuaPattern words( ext );
					int start, end;
					if ( words.find( fileName, start, end ) ) {
						if ( hFileAsCPP && style.getLSPName() == "c" && ext == "%.h$" )
							return getByLSPName( "cpp" );
						return style;
					}
				} else if ( extension == ext ) {
					if ( hFileAsCPP && style.getLSPName() == "c" && ext == ".h" )
						return getByLSPName( "cpp" );
					return style;
				}
			}
		}
	}
	return mDefinitions[0];
}

const SyntaxDefinition& SyntaxDefinitionManager::getByHeader( const std::string& header,
															  bool /*hFileAsCPP*/ ) const {
	if ( !header.empty() ) {
		for ( auto style = mDefinitions.rbegin(); style != mDefinitions.rend(); ++style ) {
			for ( const auto& hdr : style->getHeaders() ) {
				LuaPattern words( hdr );
				int start, end;
				if ( words.find( header, start, end ) ) {
					return *style;
				}
			}
		}
	}
	return mDefinitions[0];
}

const SyntaxDefinition& SyntaxDefinitionManager::find( const std::string& filePath,
													   const std::string& header,
													   bool hFileAsCPP ) {
	const SyntaxDefinition& def = getByHeader( header );
	if ( def.getLanguageName() == mDefinitions[0].getLanguageName() )
		return getByExtension( filePath, hFileAsCPP );
	return def;
}

}}} // namespace EE::UI::Doc
