#include <eepp/ui/doc/languages/containerfile.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addContainerFile() {

	auto& sd = SyntaxDefinitionManager::instance()->add(

		{ "Containerfile",
		  { "^[Cc]ontainerfile$", "^[dD]ockerfile$", "%.[dD]ockerfile$" },
		  {
			  { { "#.*\n" }, "comment" },
			  { { "%[", "%]" }, "string" },
			  { { "%sas%s" }, "literal" },
			  { { "--platform=" }, "literal" },
			  { { "--chown=" }, "literal" },
			  { { "[%a_][%w_]*" }, "symbol" },

		  },
		  {
			  { "EXPOSE", "keyword2" },
			  { "FROM", "keyword" },
			  { "STOPSIGNAL", "keyword2" },
			  { "HEALTHCHECK", "keyword2" },
			  { "SHELL", "keyword2" },
			  { "ENV", "keyword2" },
			  { "RUN", "keyword2" },
			  { "ENTRYPOINT", "function" },
			  { "VOLUME", "keyword2" },
			  { "WORKDIR", "keyword2" },
			  { "LABEL", "keyword2" },
			  { "USER", "keyword2" },
			  { "ONBUILD", "keyword2" },
			  { "COPY", "keyword2" },
			  { "ARG", "keyword2" },
			  { "ADD", "keyword2" },
			  { "CMD", "function" },

		  },
		  "#",
		  {},
		  "dockerfile" } );

	sd.setFoldRangeType( FoldRangeType::Indentation );
}

}}}} // namespace EE::UI::Doc::Language
