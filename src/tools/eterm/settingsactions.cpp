#include "settingsactions.hpp"
#include "eterm.hpp"
#include "settingspanel.hpp"

#include <eepp/ui/tools/uifontpickerdialog.hpp>
#include <eepp/ui/tools/uisettingspanel.hpp>

namespace eterm {

SettingsActions::SettingsActions( App* app ) : mApp( app ) {}

void SettingsActions::showSettings( const std::string& category ) {
	if ( mSettingsWindow ) {
		mSettingsWindow->show();
		mSettingsWindow->toFront();
		auto* panel = mSettingsWindow->find<Tools::UISettingsPanel>( "settings_panel_content" );
		if ( !category.empty() )
			panel->selectCategory( category );
		panel->runOnMainThread( [panel] { panel->focusSearch(); } );
		return;
	}
	mSettingsWindow = SettingsPanel::create( *mApp );
	mSettingsWindow->on( Event::OnWindowClose,
						 [this]( const Event* ) { mSettingsWindow = nullptr; } );
	if ( !category.empty() ) {
		mSettingsWindow->find<Tools::UISettingsPanel>( "settings_panel_content" )
			->selectCategory( category );
	}
}

void SettingsActions::closeSettings() {
	if ( mSettingsWindow )
		mSettingsWindow->closeWindow();
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
	// Not configured: the font in use (the bundled one, or the first explicit fallback).
	if ( currentPath.empty() ) {
		if ( uiFont ) {
			currentPath =
				FontTrueType::filePathOf( mApp->scene->getUIThemeManager()->getDefaultFont() );
		} else if ( fallbackFont ) {
			const auto& fallbacks =
				mApp->scene->getResourceScope()->getFontService().getFallbackFonts();
			if ( !fallbacks.empty() )
				currentPath = FontTrueType::filePathOf( fallbacks.front().get() );
		} else {
			currentPath = FontTrueType::filePathOf( mApp->terminalFont );
		}
	}
	if ( !currentPath.empty() )
		dialog->setSelectedFont( currentPath );
	if ( !fallbackFont ) {
		auto selection = dialog->getSelection();
		selection.size = static_cast<Uint32>(
			!uiFont ? mApp->config->font.size
			: mApp->config->font.uiSize > 0
				? mApp->config->font.uiSize
				: PixelDensity::pxToDp( mApp->scene->getUIThemeManager()->getDefaultFontSize() ) );
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
				applyUIFont( font.get(), mApp->config->font.uiSize );
			}
		} else if ( fallbackFont ) {
			auto font = FontTrueType::New( "eterm-fallback-font", resourceScope );
			if ( font->loadFromFile( selection.font.path ) ) {
				mApp->config->font.fallbackPath = selection.font.path;
				// It replaces the previous one (fallbacks are looked up in order).
				auto& service = resourceScope.getFontService();
				auto previous = service.getFallbackFonts();
				for ( const auto& fallback : previous )
					service.removeFallbackFont( fallback.get() );
				service.addFallbackFont( std::move( font ) );
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
	dialog->center();
	dialog->show();
}

void SettingsActions::setUIFontSize( Float size ) {
	mApp->config->font.uiSize = size;
	applyUIFont( nullptr, size );
	mApp->savePreferences();
}

void SettingsActions::setTerminalFontSize( Float size ) {
	mApp->config->font.size = size;
	mApp->terminalFontSize = PixelDensity::dpToPx( size );
	mApp->forEachTerminal(
		[this]( UITerminal* terminal ) { terminal->setFontSize( mApp->terminalFontSize ); } );
	mApp->savePreferences();
}

void SettingsActions::applyUIFont( Font* font, Float sizeDp ) {
	mApp->scene->applyDefaultFont( font, PixelDensity::dpToPx( sizeDp ) );
}

void SettingsActions::applyFontRendering() {
	mApp->scene->setFontRendering( mApp->config->font.hinting, mApp->config->font.antialiasing );
	mApp->forEachTerminal( []( UITerminal* terminal ) { terminal->syncFontRenderingConfig(); } );
}

} // namespace eterm
