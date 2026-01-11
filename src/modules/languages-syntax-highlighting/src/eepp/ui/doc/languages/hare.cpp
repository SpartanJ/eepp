#include <eepp/ui/doc/languages/hare.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addHare() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "Hare",
		  { "%.ha$" },
		  {
			  { { "//.*" }, "comment" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "-?0x%x+" }, "number" },
			  { { "-?%d+[%d%.eE]*f?" }, "number" },
			  { { "-?%.?%d+f?" }, "number" },
			  { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "[%a_][%w_]*" }, "symbol" },
			  { { "^@" }, "keyword" },

		  },
		  {
			  { "void", "type" },		 { "valist", "type" },	   { "vaend", "keyword" },
			  { "vaarg", "keyword" },	 { "vastart", "keyword" }, { "use", "keyword" },
			  { "i16", "type" },		 { "for", "keyword" },	   { "abort", "keyword" },
			  { "fn", "keyword" },		 { "if", "keyword" },	   { "fini", "keyword" },
			  { "fconst", "type" },		 { "i64", "type" },		   { "free", "keyword" },
			  { "false", "literal" },	 { "def", "keyword" },	   { "u16", "type" },
			  { "f32", "type" },		 { "yield", "keyword" },   { "export", "keyword" },
			  { "len", "keyword" },		 { "delete", "keyword" },  { "align", "keyword" },
			  { "true", "literal" },	 { "fmt", "literal" },	   { "iconst", "type" },
			  { "init", "keyword" },	 { "break", "keyword" },   { "else", "keyword" },
			  { "continue", "keyword" }, { "i32", "type" },		   { "bool", "type" },
			  { "case", "keyword" },	 { "i8", "type" },		   { "int", "type" },
			  { "str", "type" },		 { "is", "keyword" },	   { "defer", "keyword" },
			  { "f64", "type" },		 { "enum", "type" },	   { "null", "literal" },
			  { "u8", "type" },			 { "insert", "keyword" },  { "u64", "type" },
			  { "let", "keyword" },		 { "return", "keyword" },  { "alloc", "keyword" },
			  { "offset", "keyword" },	 { "nullable", "type" },   { "rconst", "type" },
			  { "uintptr", "type" },	 { "rune", "type" },	   { "size", "type" },
			  { "signed", "literal" },	 { "match", "keyword" },   { "static", "keyword" },
			  { "switch", "keyword" },	 { "test", "keyword" },	   { "type", "keyword" },
			  { "const", "keyword" },	 { "union", "type" },	   { "u32", "type" },
			  { "struct", "type" },		 { "uint", "type" },	   { "unsigned", "literal" },
			  { "bufio", "literal" },	 { "io", "literal" },	   { "os", "literal" },
			  { "sort", "literal" },	 { "strings", "literal" }, { "ascii", "literal" },
			  { "bytes", "literal" },	 { "crypto", "literal" },  { "dirs", "literal" },
			  { "encoding", "literal" }, { "endian", "literal" },  { "errors", "literal" },
			  { "fnmatch", "literal" },	 { "format", "literal" },  { "fs", "literal" },
			  { "getopt", "literal" },	 { "glob", "literal" },	   { "hare", "literal" },
			  { "hash", "literal" },	 { "linux", "literal" },   { "log", "literal" },
			  { "math", "literal" },	 { "mime", "literal" },	   { "net", "literal" },
			  { "path", "literal" },	 { "regex", "literal" },   { "rt", "literal" },
			  { "shlex", "literal" },	 { "strconv", "literal" }, { "strio", "literal" },
			  { "temp", "literal" },	 { "test", "literal" },	   { "time", "literal" },
			  { "types", "literal" },	 { "unix", "literal" },	   { "uuid", "literal" },
		  },
		  "//",
		  {}

		} );

	sd.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );
	return sd;
}

}}}} // namespace EE::UI::Doc::Language
