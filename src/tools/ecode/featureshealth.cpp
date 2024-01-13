#include "featureshealth.hpp"
#include "plugins/formatter/formatterplugin.hpp"
#include "plugins/linter/linterplugin.hpp"
#include "plugins/lsp/lspclientplugin.hpp"
#include "plugins/pluginmanager.hpp"
#include <eepp/system/sys.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>
#include <eepp/ui/uiiconthememanager.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitableview.hpp>
#include <tabulate/tabulate.hpp>

using namespace EE::System;
using namespace EE::UI::Doc;
using namespace EE::UI;
using namespace tabulate;

namespace ecode {

inline static bool sortKey( const FeaturesHealth::LangHealth& struct1,
							const FeaturesHealth::LangHealth& struct2 ) {
	return ( struct1.lang < struct2.lang );
}

std::vector<FeaturesHealth::LangHealth> FeaturesHealth::getHealth( PluginManager* pluginManager,
																   const std::string& langFilter ) {
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

		if ( !langFilter.empty() && langFilter != def.getLSPName() )
			continue;

		FeaturesHealth::LangHealth lang;
		lang.lang = def.getLSPName();
		lang.syntaxHighlighting = true;

		if ( linter ) {
			Linter found = linter->getLinterForLang( def.getLSPName() );
			if ( !found.command.empty() ) {
				lang.linter.name = String::split( found.command, ' ' )[0];
				lang.linter.path = Sys::which( lang.linter.name );
				lang.linter.found = !lang.linter.path.empty();
				lang.linter.url = found.url;
			}
		}

		if ( formatter ) {
			FormatterPlugin::Formatter found =
				formatter->getFormatterForLang( def.getLSPName() );
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
				lsp->getClientManager().getLSPForLang( def.getLSPName() );
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

class HealthModel : public Model {
  public:
	static std::shared_ptr<HealthModel> create( std::vector<FeaturesHealth::LangHealth>&& data,
												UISceneNode* sceneNode ) {
		return std::make_shared<HealthModel>( std::move( data ), sceneNode );
	}

	HealthModel( std::vector<FeaturesHealth::LangHealth>&& data, UISceneNode* sceneNode ) :
		mData( data ), mSceneNode( sceneNode ) {}

	virtual size_t rowCount( const ModelIndex& ) const { return mData.size(); }

	virtual size_t columnCount( const ModelIndex& ) const { return 5; }

	virtual std::string columnName( const size_t& idx ) const {
		static const std::vector<std::string> columns = { "language", "highlight", "LSP", "linter",
														  "formatter" };
		if ( idx < columns.size() )
			return mSceneNode->i18n( columns[idx], String::capitalize( columns[idx] ) );
		return "";
	}

	virtual Variant data( const ModelIndex& index, ModelRole role = ModelRole::Display ) const {
		eeASSERT( index.row() < (Int64)mData.size() );
		static std::string none;
		static UIIcon* icon = nullptr;
		if ( none.empty() )
			none = mSceneNode->i18n( "none", "None" );
		if ( icon == nullptr )
			icon = mSceneNode->findIcon( "ok" );
		if ( index.row() >= (Int64)mData.size() )
			return {};
		const FeaturesHealth::LangHealth& lang = mData[index.row()];
		switch ( role ) {
			case ModelRole::Icon: {
				if ( index.column() == 1 )
					return Variant( icon );
				break;
			}
			case ModelRole::Display: {
				switch ( index.column() ) {
					case 0:
						return Variant( lang.lang );
					case 1:
						break;
					case 2:
						return Variant( lang.lsp.name.empty() ? none : lang.lsp.name );
					case 3:
						return Variant( lang.linter.name.empty() ? none : lang.linter.name );
					case 4:
						return Variant( lang.formatter.name.empty() ? none : lang.formatter.name );
					default: {
					}
				}
				break;
			}
			case ModelRole::Class: {
				switch ( index.column() ) {
					case 1:
						return Variant( "theme-success" );
					case 2:
						if ( !lang.lsp.name.empty() )
							return Variant( lang.lsp.found ? "theme-success" : "theme-error" );
						else
							return Variant( "theme-none" );
						break;
					case 3:
						if ( !lang.linter.name.empty() )
							return Variant( lang.linter.found ? "theme-success" : "theme-error" );
						else
							return Variant( "theme-none" );
						break;
					case 4:
						if ( !lang.formatter.name.empty() )
							return Variant( lang.formatter.found ? "theme-success"
																 : "theme-error" );
						else
							return Variant( "theme-none" );
						break;
					default: {
					}
				}
			}
			default: {
			}
		}
		return {};
	}

	virtual bool classModelRoleEnabled() { return true; }

	const FeaturesHealth::LangHealth& getHealthRow( size_t idx ) {
		eeASSERT( idx < mData.size() );
		return mData[idx];
	}

  protected:
	std::vector<FeaturesHealth::LangHealth> mData;
	UISceneNode* mSceneNode{ nullptr };
};

#define I18N( key, val ) sceneNode->i18n( key, val ).toUtf8().c_str()

void FeaturesHealth::displayHealth( PluginManager* pluginManager, UISceneNode* sceneNode ) {
	UIWindow* win = sceneNode
						->loadLayoutFromString( R"xml(
	<window
		id="health-window"
		lw="600dp" lh="600dp"
		padding="8dp"
		window-title="@string(languages_health, Languages Health)"
		window-flags="default|maximize|shadow"
		window-min-size="300dp 300dp">
		<vbox lw="mp" lh="mp">
			<TableView id="health-table" lw="mp" lh="0" lw8="1" />
			<vbox id="health-lang-info" lw="mp" lh="wc" min-height="118dp" visible="false"></vbox>
		</vbox>
	</window>
	)xml" )
						->asType<UIWindow>();
	UITableView* table = win->find<UITableView>( "health-table" );
	auto health = FeaturesHealth::getHealth( pluginManager );
	auto model = HealthModel::create( std::move( health ), sceneNode );
	table->setAutoColumnsWidth( true );
	table->setFitAllColumnsToWidget( true );
	table->setModel( model );
	auto healthLangInfo = win->find( "health-lang-info" );
	table->setOnSelectionChange( [table, healthLangInfo, sceneNode]() {
		if ( table->getSelection().isEmpty() || nullptr == table->getModel() ) {
			healthLangInfo->setVisible( false );
			return;
		}
		ModelIndex index = table->getSelection().first();
		static const std::string none = sceneNode->i18n( "none", "None" );
		static const std::string notFound =
			sceneNode->i18n( "not_found_in_path", "Not found in $PATH" );
		static const std::string patherr =
			String::format( "%s: %s", I18N( "path_is", "PATH is" ), std::getenv( "PATH" ) );

		HealthModel* model = static_cast<HealthModel*>( table->getModel() );
		const auto& lang = model->getHealthRow( index.row() );
		healthLangInfo->childsCloseAll();
		std::string type = lang.lsp.url.empty()
							   ? "TextView"
							   : String::format( "Anchor href='%s'", lang.lsp.url.c_str() );
		std::string l = String::format(
			R"xml(
			<hbox><TextView text='%s: ' /><TextView text='%s' font-style="bold" /></hbox>
			<hbox><TextView text='%s: ' /><TextView text='%s' class="success" /></hbox>
			<hbox><TextView text='%s: ' /><%s text='%s' class='%s' /></hbox>
		)xml",
			I18N( "language", "Language" ), lang.lang.c_str(), I18N( "highlight", "Highlight" ),
			I18N( "found", "Found" ),
			I18N( "configured_language_server", "Configured language server" ), type.c_str(),
			lang.lsp.name.empty() ? none.c_str() : lang.lsp.name.c_str(),
			lang.lsp.found ? "success" : ( lang.lsp.name.empty() ? "none" : "error" ) );

		if ( !lang.lsp.name.empty() ) {
			l += String::format( "<hbox><TextView text='%s: ' /><TextView text='%s' class='%s' "
								 "tooltip='%s' /></hbox>",
								 I18N( "binary_for_language_server", "Binary for language server" ),
								 lang.lsp.path.empty() ? notFound.c_str() : lang.lsp.path.c_str(),
								 !lang.lsp.path.empty() ? "success" : "error",
								 lang.lsp.path.empty() ? patherr.c_str() : "" );
		}

		type = lang.linter.url.empty()
				   ? "TextView"
				   : String::format( "Anchor href='%s'", lang.linter.url.c_str() );
		l += String::format( "<hbox><TextView text='%s: ' /><%s text='%s' class='%s' /></hbox>",
							 I18N( "configured_linter", "Configured linter" ), type.c_str(),
							 lang.linter.name.empty() ? none.c_str() : lang.linter.name.c_str(),
							 lang.linter.found ? "success"
											   : ( lang.linter.name.empty() ? "none" : "error" ) );

		if ( !lang.linter.name.empty() ) {
			l += String::format( "<hbox><TextView text='%s: ' /><TextView text='%s' class='%s' "
								 "tooltip='%s' /></hbox>",
								 I18N( "binary_for_linter", "Binary for linter" ),
								 lang.linter.path.empty() ? notFound.c_str()
														  : lang.linter.path.c_str(),
								 !lang.linter.path.empty() ? "success" : "error",
								 lang.linter.path.empty() ? patherr.c_str() : "" );
		}

		type = lang.formatter.url.empty()
				   ? "TextView"
				   : String::format( "Anchor href='%s'", lang.formatter.url.c_str() );
		l += String::format(
			"<hbox><TextView text='%s: ' /><%s text='%s' class='%s' /></hbox>",
			I18N( "configured_formatter", "Configured formatter" ), type.c_str(),
			lang.formatter.name.empty() ? none.c_str() : lang.formatter.name.c_str(),
			lang.formatter.found ? "success" : ( lang.formatter.name.empty() ? "none" : "error" ) );

		if ( !lang.formatter.name.empty() ) {
			l += String::format( "<hbox><TextView text='%s: ' /><TextView text='%s' class='%s' "
								 "tooltip='%s' /></hbox>",
								 I18N( "binary_for_formatter", "Binary for formatter" ),
								 lang.formatter.path.empty() ? notFound.c_str()
															 : lang.formatter.path.c_str(),
								 !lang.formatter.path.empty() ? "success" : "error",
								 lang.formatter.path.empty() ? patherr.c_str() : "" );
		}

		sceneNode->loadLayoutFromString( l, healthLangInfo );
		healthLangInfo->setVisible( true );
	} );

	win->setKeyBindingCommand( "close-window", [win]() { win->closeWindow(); } );
	win->addKeyBinding( { KEY_ESCAPE }, "close-window" );
	win->showWhenReady();
	win->center();
}

} // namespace ecode
