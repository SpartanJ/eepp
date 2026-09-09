#include "settingsactions.hpp"
#include "eterm.hpp"
#include "settingspanel.hpp"

#include <eepp/ui/tools/uifontpickerdialog.hpp>

namespace eterm {

SettingsActions::SettingsActions( App* app ) : mApp( app ) {}

void SettingsActions::showSettings() {
	if ( mSettingsWindow ) {
		mSettingsWindow->show();
		mSettingsWindow->toFront();
		return;
	}
	mSettingsWindow = SettingsPanel::create( *mApp );
	mSettingsWindow->on( Event::OnWindowClose,
						 [this]( const Event* ) { mSettingsWindow = nullptr; } );
}

void SettingsActions::openFontPicker( bool uiFont, bool fallbackFont ) {
	const Uint32 flags = UIFontPickerDialog::ShowStyle |
						 ( fallbackFont ? 0 : UIFontPickerDialog::ShowSize ) |
						 ( !uiFont && !fallbackFont ? UIFontPickerDialog::MonospaceOnly : 0 );
	auto* dialog = UIFontPickerDialog::New( flags );
	dialog->setTitle( mApp->i18n( "select_font", "Select Font" ) );
	dialog->setCloseShortcut( KEY_ESCAPE );
	std::string currentPath = uiFont		 ? mApp->config->font.uiPath
							  : fallbackFont ? mApp->config->font.fallbackPath
											 : mApp->config->font.path;
	if ( !currentPath.empty() )
		dialog->setSelectedFont( currentPath );
	if ( !fallbackFont ) {
		auto selection = dialog->getSelection();
		selection.size =
			static_cast<Uint32>( uiFont ? mApp->config->font.uiSize : mApp->config->font.size );
		dialog->setSelection( selection );
	}
	dialog->setOnFontPicked( [this, uiFont, fallbackFont]( const UIFontSelection& selection ) {
		if ( selection.font.path.empty() )
			return;
		auto& resourceScope = *mApp->scene->getResourceScope();
		if ( uiFont ) {
			auto font = FontTrueType::New( "eterm-ui-font", resourceScope );
			if ( font->loadFromFile( selection.font.path ) ) {
				mApp->config->font.uiPath = selection.font.path;
				mApp->config->font.uiSize = selection.size;
				mApp->scene->getUIThemeManager()->setDefaultFont( font.get() );
				mApp->scene->getUIThemeManager()->setDefaultFontSize( mApp->config->font.uiSize );
				mApp->scene->getRoot()->reloadStyle( true, true, true, true, true );
			}
		} else if ( fallbackFont ) {
			auto font = FontTrueType::New( "eterm-fallback-font", resourceScope );
			if ( font->loadFromFile( selection.font.path ) ) {
				mApp->config->font.fallbackPath = selection.font.path;
				resourceScope.getFontService().addFallbackFont( std::move( font ) );
			}
		} else {
			auto font = FontTrueType::New( "eterm-monospace", resourceScope );
			if ( font->loadFromFile( selection.font.path ) ) {
				mApp->config->font.path = selection.font.path;
				mApp->config->font.size = selection.size;
				mApp->terminalFont = font.get();
				mApp->terminalFontSize = PixelDensity::dpToPx( mApp->config->font.size );
				FontFamily::loadFromRegular( mApp->terminalFont );
				mApp->forEachTerminal( [this]( UITerminal* terminal ) {
					terminal->setFont( mApp->terminalFont );
					terminal->setFontSize( mApp->terminalFontSize );
				} );
			}
		}
		mApp->savePreferences();
	} );
	dialog->show();
}

void SettingsActions::setUIFontSize( Float size ) {
	mApp->config->font.uiSize = size;
	mApp->scene->getUIThemeManager()->setDefaultFontSize( size );
	mApp->scene->getRoot()->reloadStyle( true, true, true, true, true );
	mApp->savePreferences();
}

void SettingsActions::setTerminalFontSize( Float size ) {
	mApp->config->font.size = size;
	mApp->terminalFontSize = PixelDensity::dpToPx( size );
	mApp->forEachTerminal(
		[this]( UITerminal* terminal ) { terminal->setFontSize( mApp->terminalFontSize ); } );
	mApp->savePreferences();
}

} // namespace eterm
