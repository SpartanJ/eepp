#include "settingsactions.hpp"
#include "eterm.hpp"
#include "settingspanel.hpp"

#include <eepp/ui/tools/uifontpickerdialog.hpp>
#include <eepp/ui/tools/uisettingspanel.hpp>

namespace eterm {

namespace {

/** The file of a loaded font as an absolute path, empty when unknown. */
std::string fontFilePath( Font* font ) {
	FontDesc desc;
	if ( !font || font->getType() != FontType::TTF ||
		 !static_cast<FontTrueType*>( font )->getFontDesc( desc ) )
		return {};
	return FileSystem::getRealPath( desc.path );
}

} // namespace

SettingsActions::SettingsActions( App* app ) : mApp( app ) {}

void SettingsActions::showSettings() {
	if ( mSettingsWindow ) {
		mSettingsWindow->show();
		mSettingsWindow->toFront();
		auto* panel = mSettingsWindow->find<Tools::UISettingsPanel>( "settings_panel_content" );
		panel->runOnMainThread( [panel] { panel->focusSearch(); } );
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
	// Not configured: the font in use (the bundled one, or the first explicit fallback).
	if ( currentPath.empty() ) {
		if ( uiFont ) {
			currentPath = fontFilePath( mApp->scene->getUIThemeManager()->getDefaultFont() );
		} else if ( fallbackFont ) {
			const auto& fallbacks =
				mApp->scene->getResourceScope()->getFontService().getFallbackFonts();
			if ( !fallbacks.empty() )
				currentPath = fontFilePath( fallbacks.front().get() );
		} else {
			currentPath = fontFilePath( mApp->terminalFont );
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
	// The theme decides the size (and font) of new text; existing text views that do not get them
	// from the stylesheet are updated in place (as ecode does).
	const Float size = PixelDensity::dpToPx( sizeDp );
	UIThemeManager* manager = mApp->scene->getUIThemeManager();
	manager->setDefaultFontSize( size );
	if ( manager->getDefaultTheme() )
		manager->getDefaultTheme()->setDefaultFontSize( size );
	if ( font ) {
		manager->setDefaultFont( font );
		if ( manager->getDefaultTheme() )
			manager->getDefaultTheme()->setDefaultFont( font );
	}
	mApp->scene->forEachNode( [font, size]( Node* node ) {
		if ( !node->isType( UI_TYPE_TEXTVIEW ) )
			return;
		UITextView* text = node->asType<UITextView>();
		const UIStyle* style = text->getUIStyle();
		if ( !style || !style->hasProperty( PropertyId::FontSize ) )
			text->setFontSize( size );
		if ( font && ( !style || !style->hasProperty( PropertyId::FontFamily ) ) )
			text->setFont( font );
	} );
}

void SettingsActions::applyFontRendering() {
	// The base UI fonts live in the default resource scope (UIApplication creates them there),
	// the scene's own fonts in the scene's scope: both, as UIApplication does at startup.
	for ( FontService* service : { &defaultResourceScope().getFontService(),
								   &mApp->scene->getResourceScope()->getFontService() } ) {
		service->setHinting( mApp->config->font.hinting );
		service->setAntialiasing( mApp->config->font.antialiasing );
	}
	mApp->forEachTerminal( []( UITerminal* terminal ) { terminal->syncFontRenderingConfig(); } );
	mApp->scene->getRoot()->invalidateDraw();
}

} // namespace eterm
