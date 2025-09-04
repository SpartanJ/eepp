#include <eepp/ui/doc/languages/d.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

#define CSV_COLORING_PATTERN \
	"keyword", "parameter", "string", "number", "literal", "function", "type", "normal",

SyntaxDefinition& addCSV() {
	return SyntaxDefinitionManager::instance()->add(
		{ "CSV",
		  { "%.csv$" },
		  { { { "csv_parser" },
			  { "operator", CSV_COLORING_PATTERN CSV_COLORING_PATTERN CSV_COLORING_PATTERN
								CSV_COLORING_PATTERN CSV_COLORING_PATTERN CSV_COLORING_PATTERN
									CSV_COLORING_PATTERN CSV_COLORING_PATTERN },
			  "",
			  SyntaxPatternMatchType::Parser } },
		  {} } );
}

SyntaxDefinition& addTSV() {
	return SyntaxDefinitionManager::instance()->add(
		{ "TSV",
		  { "%.tsv$" },
		  { { { "tsv_parser" },
			  { "operator", CSV_COLORING_PATTERN CSV_COLORING_PATTERN CSV_COLORING_PATTERN
								CSV_COLORING_PATTERN CSV_COLORING_PATTERN CSV_COLORING_PATTERN
									CSV_COLORING_PATTERN CSV_COLORING_PATTERN },
			  "",
			  SyntaxPatternMatchType::Parser } },
		  {} } );
}

}}}} // namespace EE::UI::Doc::Language
