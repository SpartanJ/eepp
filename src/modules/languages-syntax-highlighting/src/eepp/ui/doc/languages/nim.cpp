#include <eepp/ui/doc/languages/nim.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addNim() {
	std::vector<SyntaxPattern> nim_patterns;
	SyntaxDefMap<std::string, std::string> nim_symbols;

	const std::vector<std::string> nim_number_patterns = {
		"0[bB][01][01_]*",	  "0o[0-7][0-7_]*",
		"0[xX]%x[%x_]*",	  "%d[%d_]*%.%d[%d_]*[eE][-+]?%d[%d_]*",
		"%d[%d_]*%.%d[%d_]*", "%d[%d_]*",
	};

	std::vector<std::string> nim_type_suffix_patterns;
	const std::vector<std::string> nim_num_number_list = { "", "8", "16", "32", "64" };
	nim_type_suffix_patterns.reserve( nim_num_number_list.size() );
	for ( auto& num : nim_num_number_list )
		nim_type_suffix_patterns.push_back( "'?[fuiFUI]" + num );

	nim_patterns.reserve( nim_number_patterns.size() * nim_type_suffix_patterns.size() +
						  nim_number_patterns.size() );
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

	auto& sd = SyntaxDefinitionManager::instance()->add( {
		"Nim",
		{ "%.nim$", "%.nims$", "%.nimble$" },
		std::move( nim_patterns ),
		std::move( nim_symbols ),
		"#",
	} );

	sd.setFoldRangeType( FoldRangeType::Indentation );
	return sd;
}

}}}} // namespace EE::UI::Doc::Language
