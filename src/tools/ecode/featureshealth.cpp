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

inline static bool sortKey( const FeaturesHealth::LangHealth& struct1,
							const FeaturesHealth::LangHealth& struct2 ) {
	return ( struct1.lang < struct2.lang );
}

std::vector<FeaturesHealth::LangHealth> FeaturesHealth::getHealth( PluginManager* pluginManager,
																   const std::string& lang ) {
	std::vector<FeaturesHealth::LangHealth> langs;
	bool ownsLinter = false;
	bool ownsFormatter = false;
	bool ownsLSP = false;

	const auto& definitions = SyntaxDefinitionManager::instance()->getDefinitions();

	LinterPlugin* linter = static_cast<LinterPlugin*>( pluginManager->get( "linter" ) );

	FormatterPlugin* formatter =
		static_cast<FormatterPlugin*>( pluginManager->get( "autoformatter" ) );

	LSPClientPlugin* lsp = static_cast<LSPClientPlugin*>( pluginManager->get( "lspclient" ) );

	if ( !linter ) {
		ownsLinter = true;
		linter = static_cast<LinterPlugin*>( LinterPlugin::NewSync( pluginManager ) );
	}

	if ( !formatter ) {
		ownsFormatter = true;
		formatter = static_cast<FormatterPlugin*>( FormatterPlugin::NewSync( pluginManager ) );
	}
	if ( !lsp ) {
		ownsLSP = true;
		lsp = static_cast<LSPClientPlugin*>( LSPClientPlugin::NewSync( pluginManager ) );
	}

	for ( const auto& def : definitions ) {
		if ( !def.isVisible() )
			continue;

		if ( !lang.empty() && lang != def.getLSPName() )
			continue;

		FeaturesHealth::LangHealth lang;
		lang.lang = def.getLSPName();
		lang.syntaxHighlighting = true;

		if ( linter ) {
			Linter found = linter->getLinterForLang( def.getLSPName(), def.getFiles() );
			if ( !found.command.empty() ) {
				lang.linter.name = String::split( found.command, ' ' )[0];
				lang.linter.path = Sys::which( lang.linter.name );
				lang.linter.found = !lang.linter.path.empty();
				lang.linter.url = found.url;
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
				lang.formatter.found = !lang.formatter.path.empty() ||
									   found.type == FormatterPlugin::FormatterType::Native;
				lang.formatter.url = found.url;
			}
		}

		if ( lsp ) {
			LSPDefinition found =
				lsp->getClientManager().getLSPForLang( def.getLSPName(), def.getFiles() );
			if ( !found.command.empty() ) {
				lang.lsp.name = found.name;
				lang.lsp.url = found.url;
				lang.lsp.path = Sys::which( String::split( found.command, ' ' )[0] );
				lang.lsp.found = !lang.lsp.path.empty();
			}
		}

		langs.emplace_back( std::move( lang ) );
	}

	if ( ownsLinter )
		eeSAFE_DELETE( linter );

	if ( ownsFormatter )
		eeSAFE_DELETE( formatter );

	if ( ownsLSP )
		eeSAFE_DELETE( lsp );

	std::sort( langs.begin(), langs.end(), sortKey );

	return langs;
}

std::string FeaturesHealth::generateHealthStatus( PluginManager* pluginManager,
												  OutputFormat format ) {
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

#if EE_PLATFORM == EE_PLATFORM_WIN
	std::string check = "Yes";
#else
	std::string check = "✓";
#endif

	for ( const auto& ht : status ) {
		std::string lspName = ht.lsp.name.empty() ? "None" : ht.lsp.name;
		if ( OutputFormat::Markdown == format && !ht.lsp.name.empty() && !ht.lsp.url.empty() ) {
			lspName = "[" + ht.lsp.name + "](" + ht.lsp.url + ")";
		}
		std::string linterName = ht.linter.name.empty() ? "None" : ht.linter.name;
		if ( OutputFormat::Markdown == format && !ht.linter.name.empty() &&
			 !ht.linter.url.empty() ) {
			linterName = "[" + ht.linter.name + "](" + ht.linter.url + ")";
		}
		std::string formatterName = ht.formatter.name.empty() ? "None" : ht.formatter.name;
		if ( OutputFormat::Markdown == format && !ht.formatter.name.empty() &&
			 !ht.formatter.url.empty() ) {
			formatterName = "[" + ht.formatter.name + "](" + ht.formatter.url + ")";
		}

		table.add_row( { ht.lang, check, lspName, linterName, formatterName } );

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

	if ( OutputFormat::Markdown == format ) {
		MarkdownExporter exporter;
		return exporter.dump( table );
	} else if ( OutputFormat::AsciiDoc == format ) {
		AsciiDocExporter exporter;
		return exporter.dump( table );
	} else if ( OutputFormat::Terminal == format ) {
		std::cout << table << "\n";
		return "";
	}

	return table.str();
}

void FeaturesHealth::doHealth( PluginManager* pluginManager, const std::string& lang,
							   const OutputFormat& format ) {
	if ( lang.empty() ) {
		if ( format != FeaturesHealth::OutputFormat::Terminal ) {
			std::cout << FeaturesHealth::generateHealthStatus( pluginManager, format ) << "\n";
		} else {
			FeaturesHealth::generateHealthStatus( pluginManager, format );
		}
	} else {
		auto healthRes = FeaturesHealth::getHealth( pluginManager, lang );
		if ( healthRes.empty() )
			return;

		auto& hr = healthRes[0];
		const std::string notFound = "Not found in $PATH";
		const std::string none = "None";

		std::string lspName = hr.lsp.name.empty() ? "\033[33mNone" : "\033[32m" + hr.lsp.name;
		std::string lspBinary = hr.lsp.found ? "\033[32m" + hr.lsp.path : "\033[31m" + notFound;

		std::cout << "Highlight: \033[32mFound\n\033[00m";

		std::cout << "Configured language server: " << lspName << "\n\033[00m";
		if ( !hr.lsp.name.empty() )
			std::cout << "Binary for language server: " << lspBinary << "\n\033[00m";

		std::string linterName =
			hr.linter.name.empty() ? "\033[33mNone" : "\033[32m" + hr.linter.name;
		std::string linterBinary =
			hr.linter.found ? "\033[32m" + hr.linter.path : "\033[31m" + notFound;

		std::cout << "Configured linter: " << linterName << "\n\033[00m";
		if ( !hr.linter.name.empty() )
			std::cout << "Binary for linter: " << linterBinary << "\n\033[00m";

		std::string formatterName =
			hr.formatter.name.empty() ? "\033[33mNone" : "\033[32m" + hr.formatter.name;
		std::string formatterBinary =
			hr.formatter.found ? "\033[32m" + hr.formatter.path : "\033[31m" + notFound;

		std::cout << "Configured formatter: " << formatterName << "\n\033[00m";
		if ( !hr.formatter.name.empty() )
			std::cout << "Binary for formatter: " << formatterBinary << "\n\033[00m";
	}
}

} // namespace ecode
