#include <eepp/ui/doc/languages/containerfile.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addContainerFile() {

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
			  { "EXPOSE", "type" },
			  { "FROM", "keyword" },
			  { "STOPSIGNAL", "type" },
			  { "HEALTHCHECK", "type" },
			  { "SHELL", "type" },
			  { "ENV", "type" },
			  { "RUN", "type" },
			  { "ENTRYPOINT", "function" },
			  { "VOLUME", "type" },
			  { "WORKDIR", "type" },
			  { "LABEL", "type" },
			  { "USER", "type" },
			  { "ONBUILD", "type" },
			  { "COPY", "type" },
			  { "ARG", "type" },
			  { "ADD", "type" },
			  { "CMD", "function" },

		  },
		  "#",
		  {},
		  "dockerfile" } );

	sd.setFoldRangeType( FoldRangeType::Indentation );
	return sd;
}

}}}} // namespace EE::UI::Doc::Language
