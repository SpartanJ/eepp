#include "featureshealth.hpp"
#include "plugins/formatter/formatterplugin.hpp"
#include "plugins/linter/linterplugin.hpp"
#include "plugins/lsp/lspclientplugin.hpp"
#include "plugins/pluginmanager.hpp"
#include <eepp/system/sys.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>
#include <tabulate/tabulate.hpp>

using namespace EE::System;
using namespace EE::UI::Doc;
using namespace tabulate;

namespace ecode {

std::vector<FeaturesHealth::LangHealth> FeaturesHealth::getHealth( PluginManager* pluginManager ) {
	std::vector<FeaturesHealth::LangHealth> langs;

	const auto& definitions = SyntaxDefinitionManager::instance()->getDefinitions();
	LinterPlugin* linter = static_cast<LinterPlugin*>( pluginManager->get( "linter" ) );
	FormatterPlugin* formatter =
		static_cast<FormatterPlugin*>( pluginManager->get( "autoformatter" ) );
	LSPClientPlugin* lsp = static_cast<LSPClientPlugin*>( pluginManager->get( "lspclient" ) );

	for ( const auto& def : definitions ) {
		FeaturesHealth::LangHealth lang;
		lang.lang = def.getLSPName();
		lang.syntaxHighlighting = true;

		if ( linter ) {
			Linter found = linter->getLinterForLang( def.getLSPName(), def.getFiles() );
			if ( !found.command.empty() ) {
				lang.linter.name = String::split( found.command, ' ' )[0];
				lang.linter.path = Sys::which( lang.linter.name );
				lang.linter.found = !lang.linter.path.empty();
			}
		}

		if ( formatter ) {
			FormatterPlugin::Formatter found =
				formatter->getFormatterForLang( def.getLSPName(), def.getFiles() );
			if ( !found.command.empty() ) {
				lang.formatter.name = found.type == FormatterPlugin::FormatterType::Native
										  ? "native"
										  : String::split( found.command, ' ' )[0];
				lang.formatter.path = Sys::which( lang.formatter.name );
				lang.formatter.found = !lang.linter.path.empty() ||
									   found.type == FormatterPlugin::FormatterType::Native;
			}
		}

		if ( lsp ) {
			LSPDefinition found =
				lsp->getClientManager().getLSPForLang( def.getLSPName(), def.getFiles() );
			if ( !found.command.empty() ) {
				lang.lsp.name = String::split( found.command, ' ' )[0];
				lang.lsp.path = Sys::which( lang.lsp.name );
				lang.lsp.found = !lang.lsp.path.empty();
			}
		}

		langs.emplace_back( std::move( lang ) );
	}

	return langs;
}

void FeaturesHealth::printHealth( PluginManager* pluginManager ) {
	auto status( getHealth( pluginManager ) );
	Table table;
	table.format().border_top( "" ).border_bottom( "" ).border_left( "" ).border_right( "" ).corner(
		"" );

	size_t numRows = 5;

	table.add_row( { "Language", "Highlight", "LSP", "Linter", "Formatter" } );

	for ( size_t i = 0; i < numRows; ++i ) {
		table[0][i]
			.format()
			.font_color( tabulate::Color::white )
			.font_align( FontAlign::center )
			.font_style( { FontStyle::bold } );
	}

	for ( const auto& ht : status ) {
		table.add_row( { ht.lang, "Found", ht.lsp.name.empty() ? "None" : ht.lsp.name,
						 ht.linter.name.empty() ? "None" : ht.linter.name,
						 ht.formatter.name.empty() ? "None" : ht.formatter.name } );

		auto& row = table[table.size() - 1];

		row[1].format().font_color( tabulate::Color::green );

		if ( !ht.lsp.name.empty() ) {
			row[2].format().font_color( ht.lsp.found ? tabulate::Color::green
													 : tabulate::Color::red );
		} else {
			row[2].format().font_color( tabulate::Color::yellow );
		}

		if ( !ht.linter.name.empty() ) {
			row[3].format().font_color( ht.linter.found ? tabulate::Color::green
														: tabulate::Color::red );
		} else {
			row[3].format().font_color( tabulate::Color::yellow );
		}

		if ( !ht.formatter.name.empty() ) {
			row[4].format().font_color( ht.formatter.found ? tabulate::Color::green
														   : tabulate::Color::red );
		} else {
			row[4].format().font_color( tabulate::Color::yellow );
		}
	}

	std::cout << table << "\n";
}

} // namespace ecode
