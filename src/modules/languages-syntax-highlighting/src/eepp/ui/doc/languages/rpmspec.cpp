#include <eepp/ui/doc/languages/rpmspec.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addRpmspec() {

	return SyntaxDefinitionManager::instance()
		->add(

			{ "RPM Spec",
			  { "%.spec$" },
			  {
				  { { "(%package)\\s+(.*)" },
					{ "normal", "keyword", "string" },
					{},
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "%changelog", "(?<=%)" },
					{ "keyword" },
					{},
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "(%description)\\s*(?<packageName>.*)", "(?=%)" },
					{ "string", "keyword", "normal" },
					{},
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "(Content|BuildArch|BuildArchitectures|ExclusiveArch|ExcludeArch):", "$" },
					{ "keyword" },
					{},
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "#archShortcuts" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },
						{ { "include", "#archValues" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },
				  { { "(?i)(Conflicts|Obsoletes|Provides|Requires|Requires\\(.*\\)|Enhances|"
					  "Suggests|BuildConflicts|BuildRequires|Recommends|PreReq|Supplements|Url|"
					  "Copyright|License|Summary|Summary\\(.*\\)|Distribution|Vendor|Packager|"
					  "Group|Source\\d*|Patch\\d*|BuildRoot|BuildArch|Prefix|Icon|ExclusiveOs|"
					  "ExcludeOs|Autoreq):",
					  "$" },
					{ "normal", "keyword" },
					{},
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "#logicalOperators" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },
						{ { "include", "#globalVariable" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },
						{ { "include", "#stringValue" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },
				  { { "(Epoch|Serial|Nosource|Nopatch):" },
					"keyword",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "(AutoReq|AutoProv|AutoReqProv):" },
					"keyword",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "(?i)(Name|Version|Release):", "$" },
					{ "string", "keyword" },
					{},
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "#numericConstant" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },
						{ { "include", "#globalVariable" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },
				  { { "^# norootforbuild" }, "keyword", "", SyntaxPatternMatchType::RegEx },
				  { { "(%global)\\s+([^\\s]+)", "$" },
					{ "normal", "keyword", "normal" },
					{},
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "#logicalOperators" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },
						{ { "include", "#globalVariable" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },
						{ { "include", "#stringValue" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },
						{ { "include", "#numericConstant" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },
				  { { "%(define|undefine)", "$" },
					{ "keyword" },
					{},
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "\\s+", "$" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::RegEx,
						  {
							  { { "([^\\s(]+)(\\(\\s*\\))", "$" },
								{ "normal", "function", "operator" },
								{},
								"",
								SyntaxPatternMatchType::RegEx,
								{
									{ { ".*" }, "string", "", SyntaxPatternMatchType::RegEx },

								} },
							  { { "\\S+", "$" },
								{ "normal" },
								{},
								"",
								SyntaxPatternMatchType::RegEx,
								{
									{ { ".*" }, "string", "", SyntaxPatternMatchType::RegEx },

								} },

						  } },

					} },
				  { { "%(ifarch|ifnarch)", "$" },
					{ "keyword" },
					{},
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "#globalVariable" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },
						{ { "include", "#archShortcuts" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },
						{ { "include", "#archValues" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },
				  { { "%(if)\\b", "$" },
					{ "keyword" },
					{},
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "#logicalOperators" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },
						{ { "include", "#globalVariable" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },
						{ { "include", "#numericConstant" },
						  { "normal" },
						  {},
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },
				  { { "^%(check$|files)" }, "keyword", "", SyntaxPatternMatchType::RegEx },
				  { { "^(%(prep$|build$|install$|clean$|(pre|post)(un|trans)?|trigger(in|un|postun)"
					  "|verifyscript))" },
					"keyword",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "%(else|endif|define|undefine|ifos|ifnos)" },
					"keyword",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "(%\\{)", "(\\})" },
					{ "operator" },
					{ "operator" },
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "%([A-Za-z0-9_]+)" }, "keyword", "", SyntaxPatternMatchType::RegEx },
				  { { "^\\s*#.*$" }, "comment", "", SyntaxPatternMatchType::RegEx },
				  { { "^\\* .*\\)$" }, "normal", "", SyntaxPatternMatchType::RegEx },

			  },
			  {

			  },
			  "",
			  {}

			} )
		.addRepositories( {

			{ "archShortcuts",
			  {
				  { { "%\\{(alpha|arm|arm32|arm64|ix86|loongarch64|mips|mips32|mips64|mipseb|"
					  "mipsel|power64|riscv128|riscv32|sparc|x86_64)\\}" },
					"literal",
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "archValues",
			  {
				  { { "\\b(aarch64|alpha|alphaev5|alphaev56|alphaev6|alphaev67|alphapca56|amd64|"
					  "armv3l|armv4b|armv4l|armv5tejl|armv5tel|armv5tl|armv6hl|armv6l|armv7hl|"
					  "armv7hnl|armv7l|armv8hl|armv8l|athlon|em64t|geode|i370|i386|i486|i586|i686|"
					  "ia32e|ia64|loongarch64|m68k|m68kmint|mips|mips64|mips64el|mips64r6|"
					  "mips64r6el|mipsel|mipsr6|mipsr6el|pentium3|pentium4|ppc|ppc32dy4|ppc64|"
					  "ppc64iseries|ppc64le|ppc64p7|ppc64pseries|ppc8260|ppc8560|ppciseries|"
					  "ppcpseries|riscv64|rs6000|s390|s390x|sgi|sh|sh3|sh4|sh4a|sparc|sparc64|"
					  "sparc64v|sparcv8|sparcv9|sparcv9v|x86_64|x86_64_v2|x86_64_v3|x86_64_v4|"
					  "xtensa)\\b" },
					"literal",
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "numericConstant",
			  {
				  { { "\\b([0-9]+)\\b" }, "number", "", SyntaxPatternMatchType::RegEx },

			  } },
			{ "logicalOperators",
			  {
				  { { "<=|>=|==|!=|&&|\\|\\|<|>" }, "operator", "", SyntaxPatternMatchType::RegEx },

			  } },
			{ "globalVariable",
			  {
				  { { "([\\w]*)(%\\{)", "\\}" },
					{ "normal", "string", "operator" },
					{ "operator" },
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "\\?" }, "operator", "", SyntaxPatternMatchType::RegEx },
						{ { "([\\w]+)" }, "normal", "", SyntaxPatternMatchType::RegEx },
						{ { "(:)([\\w]+)" },
						  { "normal", "operator", "string" },
						  {},
						  "",
						  SyntaxPatternMatchType::RegEx },

					} },

			  } },
			{ "stringValue",
			  {
				  { { "([\\w\\-\\.:/,\\+]+)" }, "string", "", SyntaxPatternMatchType::RegEx },

			  } },
		} );
}

}}}} // namespace EE::UI::Doc::Language
