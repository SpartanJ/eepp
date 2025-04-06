#include <eepp/system/parsermatcher.hpp>
#include <eepp/ui/doc/languages/cpp.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addCPP() {
	ParserMatcherManager::instance()->registerBaseParsers();

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "C++",
		  { "%.cpp$", "%.cc$", "%.cxx$", "%.c++$", "%.hh$", "%.inl$", "%.hxx$", "%.hpp$", "%.h++$",
			"%.tcc$", "%.C$", "%.H$", "%.h++$", "%.ino$", "%.cu$", "%.cuh$" },
		  {
			  { { "R%\"(xml)%(", "%)(xml)%\"" }, { "string", "keyword2", "keyword2" }, "XML" },
			  { { "R%\"(css)%(", "%)(css)%\"" }, { "string", "keyword2", "keyword2" }, "CSS" },
			  { { "R%\"(html)%(", "%)(html)%\"" }, { "string", "keyword2", "keyword2" }, "HTML" },
			  { { "R%\"(json)%(", "%)(json)%\"" }, { "string", "keyword2", "keyword2" }, "JSON" },
			  { { "R\"[%a-\"]+%(", "%)[%a-\"]+%\"" }, "string" },
			  { { "R\"%(", "%)\"" }, "string" },
			  { { "//.-\n" }, "comment" },
			  { { "/%*", "%*/" }, "comment" },
			  { { "\"", "[\"\n]", "\\" }, "string" },
			  { { "'", "'", "\\" }, "string" },
			  { { "(#%s*include)%s+([<%\"][%w%d%.%\\%/%_%-]+[>%\"])" },
				{ "keyword", "keyword", "literal" } },
			  { { "cpp_number_parser" }, "number", "", SyntaxPatternMatchType::Parser },
			  { { "[%+%-=/%*%^%%<>!~|&]" }, "operator" },
			  { { "[%a_][%w_]*%f[(]" }, "function" },
			  { { "std%:%:[%w_]*" }, "keyword2" },
			  { { "(%[)(%[)(%a[%w_]+)(%])(%])" },
				{ "normal", "keyword", "keyword3", "keyword2", "keyword3", "keyword" } },
			  { { "#[%a_][%w_]*" }, "symbol" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "int", "keyword2" },		 { "then", "keyword" },
			  { "new", "keyword" },			 { "reinterpret_cast", "keyword" },
			  { "this", "keyword" },		 { "long", "keyword2" },
			  { "Uint64", "keyword2" },		 { "Vector2i", "keyword2" },
			  { "String", "keyword2" },		 { "Int8", "keyword2" },
			  { "short", "keyword2" },		 { "nullptr", "keyword" },
			  { "dynamic_cast", "keyword" }, { "for", "keyword" },
			  { "goto", "keyword" },		 { "while", "keyword" },
			  { "const", "keyword" },		 { "enum", "keyword" },
			  { "break", "keyword" },		 { "Uint32", "keyword2" },
			  { "Uint8", "keyword2" },		 { "or", "keyword" },
			  { "compl", "keyword" },		 { "char8_t", "keyword2" },
			  { "friend", "keyword" },		 { "alignof", "keyword" },
			  { "volatile", "keyword" },	 { "noexcept", "keyword" },
			  { "decltype", "keyword" },	 { "auto", "keyword2" },
			  { "constinit", "keyword" },	 { "class", "keyword" },
			  { "constexpr", "keyword" },	 { "void", "keyword" },
			  { "char16_t", "keyword2" },	 { "static", "keyword" },
			  { "inline", "keyword" },		 { "typedef", "keyword" },
			  { "const_cast", "keyword" },	 { "co_return", "keyword" },
			  { "throw", "keyword" },		 { "true", "keyword2" },
			  { "float", "keyword2" },		 { "Vector2f", "keyword2" },
			  { "bitand", "keyword" },		 { "size_t", "keyword2" },
			  { "Uint16", "keyword2" },		 { "case", "keyword" },
			  { "mutable", "keyword" },		 { "protected", "keyword" },
			  { "do", "keyword" },			 { "continue", "keyword" },
			  { "asm", "keyword" },			 { "default", "keyword" },
			  { "Recti", "keyword2" },		 { "char", "keyword2" },
			  { "bool", "keyword2" },		 { "Rectf", "keyword2" },
			  { "requires", "keyword" },	 { "extern", "keyword" },
			  { "not_eq", "keyword" },		 { "static_cast", "keyword" },
			  { "namespace", "keyword" },	 { "union", "keyword" },
			  { "xor", "keyword" },			 { "Int64", "keyword2" },
			  { "false", "keyword2" },		 { "int32_t", "keyword2" },
			  { "int16_t", "keyword2" },	 { "uint16_t", "keyword2" },
			  { "concept", "keyword" },		 { "typename", "keyword" },
			  { "else", "keyword" },		 { "co_yield", "keyword" },
			  { "uint32_t", "keyword2" },	 { "operator", "keyword" },
			  { "Int32", "keyword2" },		 { "struct", "keyword" },
			  { "if", "keyword" },			 { "and_eq", "keyword" },
			  { "Int16", "keyword2" },		 { "xor_eq", "keyword" },
			  { "elseif", "keyword" },		 { "public", "keyword" },
			  { "virtual", "keyword" },		 { "unsigned", "keyword2" },
			  { "and", "keyword" },			 { "Float", "keyword2" },
			  { "private", "keyword" },		 { "or_eq", "keyword" },
			  { "switch", "keyword" },		 { "using", "keyword" },
			  { "double", "keyword2" },		 { "typeid", "keyword" },
			  { "delete", "keyword" },		 { "return", "keyword" },
			  { "NULL", "literal" },		 { "static_assert", "keyword" },
			  { "try", "keyword" },			 { "consteval", "keyword" },
			  { "Color", "keyword2" },		 { "register", "keyword" },
			  { "explicit", "keyword" },	 { "catch", "keyword" },
			  { "co_wait", "keyword" },		 { "override", "keyword" },
			  { "not", "keyword" },			 { "template", "keyword" },
			  { "int64_t", "keyword2" },	 { "wchar_t", "keyword2" },
			  { "bitor", "keyword" },		 { "thread_local", "keyword" },
			  { "uint64_t", "keyword2" },	 { "char32_t", "keyword2" },
			  { "alignas", "keyword" },		 { "export", "keyword" },
			  { "ssize_t", "keyword2" },

			  { "#if", "keyword" },			 { "#ifdef", "keyword" },
			  { "#ifndef", "keyword" },		 { "#else", "keyword" },
			  { "#elif", "keyword" },		 { "#elifdef", "keyword" }, // C++23
			  { "#elifndef", "keyword" },								// C++23
			  { "#endif", "keyword" },		 { "#include", "keyword" },
			  { "#define", "keyword" },		 { "#undef", "keyword" },
			  { "#line", "keyword" },		 { "#error", "keyword" },
			  { "#pragma", "keyword" },

		  },
		  "//",
		  {},
		  "cpp" } );

	sd.setFoldRangeType( FoldRangeType::Braces ).setFoldBraces( { { '{', '}' } } );
}

}}}} // namespace EE::UI::Doc::Language
