#include <eepp/ui/doc/languages/hare.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addHare() {

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
			  { { "%s+" }, "normal" },
			  { { "%w+%f[%s]" }, "normal" },

		  },
		  {
			  { "void", "keyword2" },	 { "valist", "keyword2" },	 { "vaend", "keyword" },
			  { "vaarg", "keyword" },	 { "vastart", "keyword" },	 { "use", "keyword" },
			  { "i16", "keyword2" },	 { "for", "keyword" },		 { "abort", "keyword" },
			  { "fn", "keyword" },		 { "if", "keyword" },		 { "fini", "keyword" },
			  { "fconst", "keyword2" },	 { "i64", "keyword2" },		 { "free", "keyword" },
			  { "false", "literal" },	 { "def", "keyword" },		 { "u16", "keyword2" },
			  { "f32", "keyword2" },	 { "yield", "keyword" },	 { "export", "keyword" },
			  { "len", "keyword" },		 { "delete", "keyword" },	 { "align", "keyword" },
			  { "true", "literal" },	 { "fmt", "literal" },		 { "iconst", "keyword2" },
			  { "init", "keyword" },	 { "break", "keyword" },	 { "else", "keyword" },
			  { "continue", "keyword" }, { "i32", "keyword2" },		 { "bool", "keyword2" },
			  { "case", "keyword" },	 { "i8", "keyword2" },		 { "int", "keyword2" },
			  { "str", "keyword2" },	 { "is", "keyword" },		 { "defer", "keyword" },
			  { "f64", "keyword2" },	 { "enum", "keyword2" },	 { "null", "literal" },
			  { "u8", "keyword2" },		 { "insert", "keyword" },	 { "u64", "keyword2" },
			  { "let", "keyword" },		 { "return", "keyword" },	 { "alloc", "keyword" },
			  { "offset", "keyword" },	 { "nullable", "keyword2" }, { "rconst", "keyword2" },
			  { "uintptr", "keyword2" }, { "rune", "keyword2" },	 { "size", "keyword2" },
			  { "signed", "literal" },	 { "match", "keyword" },	 { "static", "keyword" },
			  { "switch", "keyword" },	 { "test", "keyword" },		 { "type", "keyword" },
			  { "const", "keyword" },	 { "union", "keyword2" },	 { "u32", "keyword2" },
			  { "struct", "keyword2" },	 { "uint", "keyword2" },	 { "unsigned", "literal" },
			  { "bufio", "literal" },	 { "io", "literal" },		 { "os", "literal" },
			  { "sort", "literal" },	 { "strings", "literal" },	 { "ascii", "literal" },
			  { "bytes", "literal" },	 { "crypto", "literal" },	 { "dirs", "literal" },
			  { "encoding", "literal" }, { "endian", "literal" },	 { "errors", "literal" },
			  { "fnmatch", "literal" },	 { "format", "literal" },	 { "fs", "literal" },
			  { "getopt", "literal" },	 { "glob", "literal" },		 { "hare", "literal" },
			  { "hash", "literal" },	 { "linux", "literal" },	 { "log", "literal" },
			  { "math", "literal" },	 { "mime", "literal" },		 { "net", "literal" },
			  { "path", "literal" },	 { "regex", "literal" },	 { "rt", "literal" },
			  { "shlex", "literal" },	 { "strconv", "literal" },	 { "strio", "literal" },
			  { "temp", "literal" },	 { "test", "literal" },		 { "time", "literal" },
			  { "types", "literal" },	 { "unix", "literal" },		 { "uuid", "literal" },
		  },
		  "//",
		  {}

		} );

	sd.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );
}

}}}} // namespace EE::UI::Doc::Language
