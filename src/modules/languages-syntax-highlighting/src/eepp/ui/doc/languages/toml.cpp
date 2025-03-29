#include <eepp/ui/doc/languages/toml.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addToml() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "TOML",
		  { "%.toml$" },
		  {
			  { { "#.-\n" }, "comment" },
			  { { "\"\"\"", "\"\"\"", "\\" }, "string" },
			  { { "\"", "\"", "\\" }, "string" },
			  { { "'''", "'''" }, "string" },
			  { { "'", "'" }, "string" },
			  { { "[A-Za-z0-9_%.%-]+%s*%f[=]" }, "function" },
			  { { "%[[A-Za-z0-9_%.%- ]+%]" }, "keyword" },
			  { { "%[%[[A-Za-z0-9_%.%- ]+%]%]" }, "keyword" },
			  { { "[%-+]?[0-9_]+%.[0-9_]+[eE][%-+]?[0-9_]+" }, "number" },
			  { { "[%-+]?[0-9_]+%.[0-9_]+" }, "number" },
			  { { "[%-+]?[0-9_]+[eE][%-+]?[0-9_]+" }, "number" },
			  { { "[%-+]?[0-9_]+" }, "number" },
			  { { "[%-+]?0x[0-9a-fA-F_]+" }, "number" },
			  { { "[%-+]?0o[0-7_]+" }, "number" },
			  { { "[%-+]?0b[01_]+" }, "number" },
			  { { "[%-+]?nan" }, "number" },
			  { { "[%-+]?inf" }, "number" },
			  { { "[a-z]+" }, "symbol" },

		  },
		  {
			  { "true", "literal" },
			  { "false", "literal" },

		  },
		  "#",
		  {}

		} );

	sd.setFoldRangeType( FoldRangeType::Indentation );
}

}}}} // namespace EE::UI::Doc::Language
