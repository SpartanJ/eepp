#include <eepp/ui/doc/languages/koka.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addKoka() {

	return SyntaxDefinitionManager::instance()
		->add(

			{ "Koka",
			  { "%.kk$", "%.kki$", "%.kkc$" },
			  {
				  { { "include", "#line_comment" },
					"normal",
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#line_directive" },
					"normal",
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#block_comment" },
					"normal",
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#string" }, "normal", "", SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#rawstring" }, "normal", "", SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#rawstring1" },
					"normal",
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#rawstring2" },
					"normal",
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#character" }, "normal", "", SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#characteresc" },
					"normal",
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#type_app" }, "normal", "", SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#top_type" }, "normal", "", SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#top_type_type" },
					"normal",
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#top_type_alias" },
					"normal",
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#top_type_struct_args" },
					"normal",
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#top_type_struct" },
					"normal",
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#top_type_quantifier" },
					"normal",
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#decl_function" },
					"normal",
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#decl_external_import" },
					"normal",
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#decl_external" },
					"normal",
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#decl_toplevel_val" },
					"normal",
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#decl_val" }, "normal", "", SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#decl_var" }, "normal", "", SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#decl_hover_expr" },
					"normal",
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#decl_hover_implicit" },
					"normal",
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#decl_param" },
					"normal",
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#module_id" }, "normal", "", SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#import_id" }, "normal", "", SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#import_id2" },
					"normal",
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#branch" }, "normal", "", SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#dot" }, "normal", "", SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#reservedid" },
					"normal",
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#reservedcontrol" },
					"normal",
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#reservedop" },
					"normal",
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#libraryid" }, "normal", "", SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#externid" }, "normal", "", SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#qconstructor" },
					"normal",
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#qoperator" }, "normal", "", SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#kidentifier" },
					"normal",
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#qidentifier" },
					"normal",
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#identifier" },
					"normal",
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#constructor" },
					"normal",
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#special" }, "normal", "", SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#minus" }, "normal", "", SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#operator" }, "normal", "", SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#wildcard" }, "normal", "", SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#number" }, "normal", "", SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#inv_character" },
					"normal",
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#whitespace" },
					"normal",
					"",
					SyntaxPatternMatchType::LuaPattern },

			  },
			  {

			  },
			  "",
			  {}

			} )
		.addRepositories( {

			{ "module_id",
			  {
				  { { "(module)\\s*((interface)?)\\s*(([a-z][\\w\\-]*/)*[a-z][\\w\\-]*)" },
					{ "normal", "keyword", "normal", "literal", "literal" },
					{},
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "operator",
			  {
				  { { "[$%&\\*\\+@!/\\\\\\^~=\\.:\\-\\?\\|<>]+" },
					"operator",
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "libraryop",
			  {
				  { { "(!)(?![$%&\\*\\+@!/\\\\\\^~=\\.:\\-\\?\\|<>])" },
					"keyword",
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "libraryid",
			  {
				  { { "(resume|resume-shallow|rcontext)(?![\\w\\-?'])" },
					"keyword",
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "line_directive",
			  {
				  { { "^\\s*#.*$" }, "comment", "", SyntaxPatternMatchType::RegEx },

			  } },
			{ "top_type_quantifier",
			  {
				  { { "(exists|forall|some)(\\s*)(<)",
					  "(>)|(?=[\\)\\{\\}\\[\\]=;\"`]|(infix|infixr|infixl|inline|noinline|fip|fbip|"
					  "tail|type|co|lazy|rec|effect|ambient|alias|extern|fn|fun|function|val|var|"
					  "if|then|else|elif|match|inject|mask|named|handle|handler|return|module|"
					  "import|as|pub|abstract)(?![\\w\\-?']))" },
					{ "normal", "keyword", "normal", "operator" },
					{ "normal", "operator", "normal" },
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "#type_content" },
						  "normal",
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },

			  } },
			{ "type_content",
			  {
				  { { "include", "#type_implicit_parameter" },
					"normal",
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#type_parameter" },
					"normal",
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#type_content_top" },
					"normal",
					"",
					SyntaxPatternMatchType::LuaPattern },

			  } },
			{ "identifier",
			  {
				  { { "@?[a-z][\\w\\-@]*[\\']*" }, "normal", "", SyntaxPatternMatchType::RegEx },

			  } },
			{ "kidentifier",
			  {
				  { { "(public|external|inline)" }, "keyword", "", SyntaxPatternMatchType::RegEx },

			  } },
			{ "top_type",
			  {
				  { { "(:(?![$%&\\*\\+@!\\\\\\^~=\\.:\\-\\|<>]))|(where|iff|when)(?![\\w\\-])",
					  "(?=[,\\)\\{\\}\\[\\]=;\"`A-Z]|  "
					  "|(infix|infixr|infixl|inline|noinline|fip|fbip|tail|value|reference|open|"
					  "extend|rec|co|lazy|type|linear|effect|ambient|alias|extern|fn|fun|function|"
					  "val|raw|final|ctl|var|con|if|then|else|elif|match|inject|mask|named|handle|"
					  "handler|return|module|import|as|pub|abstract)(?![\\w\\-?']))" },
					{ "normal", "operator", "keyword" },
					{ "normal" },
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "#type_content_top" },
						  "normal",
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },

			  } },
			{ "externid",
			  {
				  { { "(?:c|cs|js|inline)\\s+(?:inline\\s+)?(?:(?:file|header-file|header-end-file)"
					  "\\s+)?(?=[\\\"\\{]|r#*\")" },
					"keyword",
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "decl_var",
			  {
				  { { "(var)\\s+([a-z][\\w\\-]*[\\']*|\\([$%&\\*\\+@!/"
					  "\\\\\\^~=\\.:\\-\\?\\|<>]+\\))" },
					{ "normal", "keyword", "normal" },
					{},
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "reservedid",
			  {
				  { { "(return(?=(?:\\(|\\s+\\(?)\\w[\\w\\-]*\\s*(?:\\)\\s*(?:[^;])))|infix|infixr|"
					  "infixl|type|co|lazy(?:\\s+tail)?(?:\\s+(?:fip|fbip)(?:\\(\\d+\\))?)?|rec|"
					  "struct|alias|forall|exists|some|extern|fun|fn|val|var|con|with(?:\\s+"
					  "override)?|module|import|as|in|ctx|hole|pub|abstract|effect|named|(?:raw\\s+"
					  "|final\\s+)ctl|break|continue|unsafe|mask(?:\\s+behind)?|handle|handler)(?!["
					  "\\w\\-'])" },
					"keyword",
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "import_id2",
			  {
				  { { "(import)(\\s+(([a-z][\\w\\-]*/)*[a-z][\\w\\-]*))" },
					{ "normal", "keyword", "literal", "literal" },
					{},
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "block_comment",
			  {
				  { { "/\\*", "\\*/" },
					{ "comment" },
					{ "comment" },
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "#block_comment" },
						  "normal",
						  "",
						  SyntaxPatternMatchType::LuaPattern },
						{ { "^```+.*$", "^```+\\s*$" },
						  { "comment" },
						  {},
						  "",
						  SyntaxPatternMatchType::RegEx },
						{ { "`(:[^\\`\\n]+)`" },
						  { "normal", "type" },
						  {},
						  "",
						  SyntaxPatternMatchType::RegEx },
						{ { "`(module [^\\`\\n]+)`" },
						  { "normal", "keyword" },
						  {},
						  "",
						  SyntaxPatternMatchType::RegEx },
						{ { "`+([^\\`\\n]*)`+" },
						  { "normal", "string" },
						  {},
						  "",
						  SyntaxPatternMatchType::RegEx },
						{ { "\\*([^\\*]*)\\*" },
						  { "normal", "operator" },
						  {},
						  "",
						  SyntaxPatternMatchType::RegEx },
						{ { "_([^_]*)_" },
						  { "normal", "type" },
						  {},
						  "",
						  SyntaxPatternMatchType::RegEx },

					} },

			  } },
			{ "minus",
			  {
				  { { "-(?![$%&\\*\\+@!/\\\\\\^~=\\.:\\-\\?\\|<>])" },
					"operator",
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "number",
			  {
				  { { "-?(?:0[xX][\\da-fA-F]+(?:_[\\da-fA-F]+)*(\\.[\\da-fA-F]+(?:_[\\da-fA-F]+)*)?"
					  "([pP][\\-+]?\\d+)?|0[bB][01][01_]*|(?:0|[1-9]\\d*)(?:_\\d+)*(\\.\\d+(?:_\\d+"
					  ")*([eE][\\-+]?\\d+)?)?)" },
					"number",
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "dot",
			  {
				  { { "\\." }, "normal", "", SyntaxPatternMatchType::RegEx },

			  } },
			{ "qconstructor",
			  {
				  { { "((?:[@a-z][\\w\\-@]*/#?)+)(@?[A-Z][\\w\\-@]*[\\']*)" },
					{ "normal", "literal", "type" },
					{},
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "type_kind",
			  {
				  { { "[A-Z](?![\\w\\-])" }, "normal", "", SyntaxPatternMatchType::RegEx },

			  } },
			{ "decl_hover_implicit",
			  {
				  { { "(\\?(?:[@a-z][\\w\\-@]*/#?)*)([@a-z][\\w\\-@]*[\\']*|\\([$%&\\*\\+@!/"
					  "\\\\\\^~=\\.:\\-\\?\\|<>]+\\))(?=\\s*[=])" },
					{ "normal", "normal", "normal" },
					{},
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "rawstring1",
			  {
				  { { "r#\"", "\"#" },
					{ "operator" },
					{ "operator" },
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "[^\"]+" }, "string", "", SyntaxPatternMatchType::RegEx },
						{ { "\"(?!#)" }, "string", "", SyntaxPatternMatchType::RegEx },
						{ { "." }, "normal", "", SyntaxPatternMatchType::RegEx },

					} },

			  } },
			{ "constructor",
			  {
				  { { "[@A-Z][\\w\\-@]*[\\']*" }, "type", "", SyntaxPatternMatchType::RegEx },

			  } },
			{ "special",
			  {
				  { { "[{}\\(\\)\\[\\];,]" }, "operator", "", SyntaxPatternMatchType::RegEx },

			  } },
			{ "top_type_struct_args",
			  {
				  { { "((?:(?:value|ref)\\s*)?struct)\\s+([a-z][\\w\\-]*|\\(,*\\))\\s*(<)",
					  "(>)|(?=[\\)\\{\\}\\[\\]=;\"`]|(infix|infixr|infixl|inline|noinline|fip|fbip|"
					  "tail|type|co|lazy|rec|effect|ambient|alias|extern|fn|fun|function|val|var|"
					  "if|then|else|elif|match|inject|mask|named|handle|handler|return|module|"
					  "import|as|pub|abstract)(?![\\w\\-?']))" },
					{ "normal", "keyword", "type", "operator" },
					{ "normal", "operator", "normal" },
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "#type_content" },
						  "normal",
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },

			  } },
			{ "decl_external_import",
			  {
				  { { "(extern\\s+import)" }, "keyword", "", SyntaxPatternMatchType::RegEx },

			  } },
			{ "line_comment",
			  {
				  { { "//", "$" },
					{ "comment" },
					{},
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "`(:[^\\`\\n]+)`" },
						  { "normal", "type" },
						  {},
						  "",
						  SyntaxPatternMatchType::RegEx },
						{ { "`(module [^\\`\\n]+)`" },
						  { "normal", "keyword" },
						  {},
						  "",
						  SyntaxPatternMatchType::RegEx },
						{ { "`+([^\\`\\n]*)`+" },
						  { "normal", "string" },
						  {},
						  "",
						  SyntaxPatternMatchType::RegEx },
						{ { "\\*([^\\*]*)\\*" },
						  { "normal", "operator" },
						  {},
						  "",
						  SyntaxPatternMatchType::RegEx },
						{ { "_([^_]*)_" },
						  { "normal", "type" },
						  {},
						  "",
						  SyntaxPatternMatchType::RegEx },

					} },

			  } },
			{ "decl_function",
			  {
				  { { "((?:(?:inline|noinline)\\s+)?(?:tail\\s+)?(?:(?:fip|fbip)(?:\\(\\d+\\))?\\s+"
					  ")?(?:fun|fn|ctl|ret))\\s+((?:[@a-z][\\w\\-@]*/"
					  "#?)*)([@a-z][\\w\\-@]*[\\']*|\\([$%&\\*\\+@!/"
					  "\\\\\\^~=\\.:\\-\\?\\|<>]+\\)|\\[\\]|\\\"[^\\s\\\"]+\\\")" },
					{ "normal", "keyword", "function", "function" },
					{},
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "qidentifier",
			  {
				  { { "([\\?]?(?:[@a-z][\\w\\-@]*/#?)+)(@?[a-z][\\w\\-@]*[\\']*)" },
					{ "normal", "literal", "normal" },
					{},
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "import_id",
			  {
				  { { "(import)(\\s+(([a-z][\\w\\-]*/"
					  ")*[a-z][\\w\\-]*)(\\s+(=)(\\s+(([a-z][\\w\\-]*/)*[a-z][\\w\\-]*))?))" },
					{ "normal", "keyword", "normal", "literal", "normal", "normal", "keyword",
					  "normal", "literal" },
					{},
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "whitespace",
			  {
				  { { "[ \\t]+" }, "normal", "", SyntaxPatternMatchType::RegEx },

			  } },
			{ "decl_param",
			  {
				  { { "([a-z][\\w\\-]*[\\']*)\\s*(?=:)" },
					"parameter",
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "decl_hover_expr",
			  {
				  { { "^expr" }, "keyword", "", SyntaxPatternMatchType::RegEx },

			  } },
			{ "decl_toplevel_val",
			  {
				  { { "(^(?:(?:inline|noinline)\\s+)?val)\\s+((?:[@a-z][\\w\\-@]*/"
					  "#?)*)([@a-z][\\w\\-@]*[\\']*|\\([$%&\\*\\+@!/"
					  "\\\\\\^~=\\.:\\-\\?\\|<>]+\\))?" },
					{ "normal", "keyword", "literal", "normal" },
					{},
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "type_implicit_parameter",
			  {
				  { { "(\\?(?:[@a-z][\\w\\-@]*/#?)*)([@a-z][\\w\\-@]*[\\']*|\\([$%&\\*\\+@!/"
					  "\\\\\\^~=\\.:\\-\\?\\|<>]+\\))\\s*(?=:(?!:))" },
					{ "normal", "parameter", "parameter" },
					{},
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "inv_character",
			  {
				  { { "'([^'\\\\\\n]|\\\\(.|x..|u....|U......))'|'$|''?" },
					"normal",
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "decl_val",
			  {
				  { { "((?:(?:inline|noinline)\\s+)?val)\\s+((?:[@a-z][\\w\\-@]*/"
					  "#?)*)([@a-z][\\w\\-@]*[\\']*|\\([$%&\\*\\+@!/"
					  "\\\\\\^~=\\.:\\-\\?\\|<>]+\\))?" },
					{ "normal", "keyword", "literal", "normal" },
					{},
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "decl_external",
			  {
				  { { "((?:(?:inline|noinline)\\s+)?(?:(?:fip|fbip)\\s+)?extern)\\s+((?:[@a-z]["
					  "\\w\\-@]*/#?)*)([@a-z][\\w\\-@]*[\\']*|\\([$%&\\*\\+@!/"
					  "\\\\\\^~=\\.:\\-\\?\\|<>]+\\)|\\[\\]|\\\"[^\\s\\\"]+\\\")?" },
					{ "normal", "keyword", "literal", "normal" },
					{},
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "param_identifier",
			  {
				  { { "([^]\\s+)?(\\?[\\?]?\\s*)?([@a-z][\\w\\-@]*[\\']*)\\s*(?=[:,\\)])" },
					{ "normal", "keyword", "keyword", "parameter" },
					{},
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "qoperator",
			  {
				  { { "([\\?]?(?:[@a-z][\\w\\-@]*/#?)+)(\\([^\\n\\r\\)]+\\))" },
					{ "normal", "literal", "operator" },
					{},
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "rawstring",
			  {
				  { { "r\"", "\"" },
					{ "operator" },
					{ "operator" },
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "[^\"]+" }, "string", "", SyntaxPatternMatchType::RegEx },
						{ { "." }, "normal", "", SyntaxPatternMatchType::RegEx },

					} },

			  } },
			{ "rawstring2",
			  {
				  { { "r##\"", "\"##" },
					{ "operator" },
					{ "operator" },
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "[^\"]+" }, "string", "", SyntaxPatternMatchType::RegEx },
						{ { "\"(?!##)" }, "string", "", SyntaxPatternMatchType::RegEx },
						{ { "." }, "normal", "", SyntaxPatternMatchType::RegEx },

					} },

			  } },
			{ "reservedcontrol",
			  {
				  { { "(if|then|else|elif|match|return)(?![\\w\\-'])" },
					"keyword",
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "type_app",
			  {
				  { { "<(?![%&\\*\\+@!/\\\\\\^~=\\.:\\-\\?\\|\\s\\d])", ">|\\n|  " },
					{ "operator" },
					{ "operator" },
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "#type_content" },
						  "normal",
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },

			  } },
			{ "string",
			  {
				  { { "\"", "\"|$" },
					{ "operator" },
					{ "operator" },
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "([^\"\\\\]|\\\\.)+$" }, "normal", "", SyntaxPatternMatchType::RegEx },
						{ { "[^\"\\\\]+" }, "string", "", SyntaxPatternMatchType::RegEx },
						{ { "\\\\([abfnrtvz0\\\\\"'\\?]|x[\\da-fA-F]{2}|u[\\da-fA-F]{4}|U[\\da-fA-"
							"F]{6})" },
						  "string",
						  "",
						  SyntaxPatternMatchType::RegEx },
						{ { "." }, "normal", "", SyntaxPatternMatchType::RegEx },

					} },

			  } },
			{ "reservedop",
			  {
				  { { "(=|=>|\\->|<\\-|\\||\\.|:|:=)(?![$%&\\*\\+!/\\\\\\^~=\\.:\\-\\?\\|<>])" },
					"operator",
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "top_type_alias",
			  {
				  { { "(alias)\\s+([a-z][\\w\\-]+)",
					  "(?=[,\\)\\{\\}\\[\\];\"`A-Z]|(infix|infixr|infixl|inline|noinline|fip|fbip|"
					  "tail|type|co|lazy|rec|linear|alias|effect|ambient|extern|fn|fun|function|"
					  "val|var|con|if|then|else|elif|match|inject|mask|named|handle|handler|return|"
					  "module|import|as|pub|abstract)(?![\\w\\-?']))" },
					{ "normal", "keyword", "type" },
					{ "normal" },
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "=" }, "operator", "", SyntaxPatternMatchType::RegEx },
						{ { "include", "#type_content_top" },
						  "normal",
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },

			  } },
			{ "top_type_struct",
			  {
				  { { "((?:(?:value|ref)\\s*)?struct)\\s+((?:[@a-z][\\w\\-@]*/"
					  "#?)*)(@?[a-z][\\w\\-@]*[\\']*)" },
					{ "normal", "keyword", "literal", "type" },
					{},
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "top_type_type",
			  {
				  { { "((?:(?:value|reference|open|extend|rec|co|lazy(?:\\s+tail)?(?:\\s+(?:fip|"
					  "fbip)(?:\\(\\d+\\))?)?)?\\s*type)|(?:named\\s+)?(?:scoped\\s+)?(?:linear\\s+"
					  ")?(?:rec\\s+)?(?:effect|ambient))\\s+(?!fn|fun|val|raw|final|ctl|ret)((?:[@"
					  "a-z][\\w\\-@]*/#?)*)(@?[a-z][\\w\\-@]*[\\']*)",
					  "(?=[\\)\\{\\}\\[\\]=;\"`A-Z]|  "
					  "[\\r\\n]|(infix|infixr|infixl|inline|noinline|fip|fbip|tail|type|co|lazy|"
					  "rec|effect|ambient|alias|extern|fn|fun|function|val|var|raw|final|ctl|con|"
					  "if|then|else|elif|match|inject|mask|named|handle|handler|return|module|"
					  "import|as|pub|abstract|value|reference|open|extend)(?![\\w\\-?']))" },
					{ "normal", "keyword", "literal", "type" },
					{ "operator" },
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "#type_content_top" },
						  "normal",
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },

			  } },
			{ "type_identifier",
			  {
				  { { "[\\$]?[@a-z][\\w\\-@]*[\\']*" }, "type", "", SyntaxPatternMatchType::RegEx },

			  } },
			{ "character",
			  {
				  { { "'[^\\'\\\\$]'" },
					{ "operator" },
					{ "operator" },
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "type_parameter",
			  {
				  { { "([\\^]\\s+)?((?:[\\?][\\?]?\\s*)?[@a-z][\\w\\-@]*[\\']*)\\s*(?=:(?!:))" },
					{ "normal", "keyword", "parameter" },
					{},
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "type_qidentifier",
			  {
				  { { "([@a-z][\\w\\-@]*[\\']*/#?)+" }, "type", "", SyntaxPatternMatchType::RegEx },

			  } },
			{ "type_variable",
			  {
				  { { "([_]?[a-z][0-9]*|_[\\w\\-]*[\\']*|self)(?!\\w)" },
					"type",
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "characteresc",
			  {
				  { { "(')(\\\\([abfnrtv0\\\\\"'\\?]|x[\\da-fA-F]{2}|u[\\da-fA-F]{4}|U[\\da-fA-F]{"
					  "6}))(')" },
					{ "normal", "string", "string", "string", "string" },
					{},
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "wildcard",
			  {
				  { { "@?_[\\w\\-@]*[\\']*" }, "normal", "", SyntaxPatternMatchType::RegEx },

			  } },
			{ "branch",
			  {
				  { { "(finally|initially)\\s*(?=->|[\\{\\(])" },
					"keyword",
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "type_content_top",
			  {
				  { { "(forall|exists|some|with|in|iff|when|is|if)(?![\\w\\-])" },
					"keyword",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "(\\->|::?|\\.)(?![$%&\\*\\+@!\\\\\\^~=\\.:\\-\\?\\|<>])" },
					"operator",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "include", "#type_qidentifier" },
					"normal",
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#type_variable" },
					"normal",
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#type_identifier" },
					"normal",
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#type_kind" }, "normal", "", SyntaxPatternMatchType::LuaPattern },
				  { { "\\(", "\\)" },
					{ "operator" },
					{ "operator" },
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "#type_content" },
						  "normal",
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },
				  { { "<(?![%&\\*\\+@!/\\\\\\^~=\\.:\\-\\?\\|])", ">|\\n|  " },
					{ "operator" },
					{ "operator" },
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "#type_content" },
						  "normal",
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },
				  { { "\\[", "\\]" },
					{ "operator" },
					{ "operator" },
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "#type_content" },
						  "normal",
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },
				  { { "include", "#line_comment" },
					"normal",
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#block_comment" },
					"normal",
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "[;,]|:" }, "operator", "", SyntaxPatternMatchType::RegEx },

			  } },
		} );
}

}}}} // namespace EE::UI::Doc::Language
