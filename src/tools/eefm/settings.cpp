#include "app.hpp"

#include <eepp/graphics/fontfamily.hpp>
#include <eepp/ui/tools/uifontpickerdialog.hpp>
#include <eepp/ui/tools/uisettingspanel.hpp>
#include <eterm/ui/uiterminal.hpp>

#include <limits>

using namespace EE::UI::Tools;
using namespace eterm::Terminal;
using namespace eterm::UI;

namespace eefm {

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

void App::savePreferences() {
	mConfig->save();
}

void App::applyFontRendering() {
	// The base UI fonts live in the default resource scope (UIApplication creates them there),
	// the scene's own fonts in the scene's scope: both, as UIApplication does at startup.
	for ( FontService* service : { &defaultResourceScope().getFontService(),
								   &mUI->getResourceScope()->getFontService() } ) {
		service->setHinting( mConfig->font.hinting );
		service->setAntialiasing( mConfig->font.antialiasing );
	}
	if ( mTerminal )
		mTerminal->syncFontRenderingConfig();
	mUI->getRoot()->invalidateDraw();
}

void App::applyUIFont( Font* font, Float sizeDp ) {
	// As ecode: the theme decides the size (and font) of new text, existing text views that
	// do not get them from the stylesheet are updated in place.
	const Float size = PixelDensity::dpToPx( sizeDp );
	UIThemeManager* manager = mUI->getUIThemeManager();
	manager->setDefaultFontSize( size );
	if ( manager->getDefaultTheme() )
		manager->getDefaultTheme()->setDefaultFontSize( size );
	if ( font ) {
		manager->setDefaultFont( font );
		if ( manager->getDefaultTheme() )
			manager->getDefaultTheme()->setDefaultFont( font );
	}
	mUI->forEachNode( [font, size]( Node* node ) {
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

const std::map<std::string, TerminalColorScheme>& App::terminalColorSchemes() {
	if ( !mTerminalColorSchemesLoaded ) {
		mTerminalColorSchemesLoaded = true;
		// The schemes eterm ships, then the user's (eterm's config folder: shared with eterm).
		auto schemes = TerminalColorScheme::loadFromFile(
			Sys::getProcessPath() + "assets/colorschemes/terminalcolorschemes.conf" );
		std::string userSchemes =
			Sys::getConfigPath( "eterm" ) + FileSystem::getOSSlash() + "colorschemes";
		if ( FileSystem::isDirectory( userSchemes ) ) {
			for ( const auto& file : FileSystem::filesGetInPath( userSchemes ) ) {
				auto more = TerminalColorScheme::loadFromFile( userSchemes +
															   FileSystem::getOSSlash() + file );
				schemes.insert( schemes.end(), std::make_move_iterator( more.begin() ),
								std::make_move_iterator( more.end() ) );
			}
		}
		for ( auto& scheme : schemes ) {
			std::string name = scheme.getName();
			mTerminalColorSchemes.emplace( std::move( name ), std::move( scheme ) );
		}
	}
	return mTerminalColorSchemes;
}

void App::applyTerminalConfig() {
	if ( !mTerminal || !mTerminal->getTerm() )
		return;
	const auto& schemes = terminalColorSchemes();
	auto found = schemes.find( mConfig->terminal.colorScheme );
	mTerminal->setColorScheme( found != schemes.end() ? found->second
													  : TerminalColorScheme::getDefault() );
	mTerminal->getTerm()->setCursorMode( mConfig->terminal.cursorStyle );
	mTerminal->setVerticalScrollMode( mConfig->terminal.scrollBarMode );
}

void App::applyFilesConfig() {
	mFileIcons.clear();
	updateIconStyle();
	for ( auto& pane : mPanes ) {
		if ( !pane )
			continue;
		for ( auto& tab : pane->tabList ) {
			tab->view->setSingleClickNavigation( mConfig->files.singleClick );
			tab->model->setCountDirectoryItems( mConfig->files.countFolderItems &&
												tab->model->uri().getScheme() == "file" );
			tab->view->invalidateDraw();
		}
	}
	refreshPlaces();
}

void App::openFontPicker( FontTarget target ) {
	const Uint32 flags = UIFontPickerDialog::ShowStyle |
						 ( target == FontTarget::Fallback ? 0 : UIFontPickerDialog::ShowSize ) |
						 ( target == FontTarget::Terminal ? UIFontPickerDialog::MonospaceOnly : 0 );
	auto* dialog = UIFontPickerDialog::New( flags );
	dialog->setTitle( mUI->i18n( "eefm_select_font", "Select Font" ) );
	dialog->setCloseShortcut( KEY_ESCAPE );
	std::string current = target == FontTarget::UI		   ? mConfig->font.uiPath
						  : target == FontTarget::Fallback ? mConfig->font.fallbackPath
														   : mConfig->terminal.fontPath;
	// Not configured: the font in use (the bundled one, or the first explicit fallback).
	if ( current.empty() ) {
		if ( target == FontTarget::UI ) {
			current = fontFilePath( mUI->getUIThemeManager()->getDefaultFont() );
		} else if ( target == FontTarget::Fallback ) {
			const auto& fallbacks = mUI->getResourceScope()->getFontService().getFallbackFonts();
			if ( !fallbacks.empty() )
				current = fontFilePath( fallbacks.front().get() );
		} else {
			current = mTerminalFont ? fontFilePath( mTerminalFont )
									: FileSystem::getRealPath( Sys::getProcessPath() +
															   "assets/fonts/DejaVuSansMono.ttf" );
		}
	}
	if ( !current.empty() )
		dialog->setSelectedFont( current );
	if ( target != FontTarget::Fallback ) {
		auto selection = dialog->getSelection();
		selection.size = static_cast<Uint32>(
			target == FontTarget::UI
				? ( mConfig->font.uiSize > 0
						? mConfig->font.uiSize
						: PixelDensity::pxToDp( mUI->getUIThemeManager()->getDefaultFontSize() ) )
				: mConfig->terminal.fontSize );
		dialog->setSelection( selection );
	}
	dialog->setOnFontPicked( [this, target]( const UIFontSelection& selection ) {
		if ( selection.font.path.empty() )
			return;
		auto& scope = *mUI->getResourceScope();
		if ( target == FontTarget::UI ) {
			auto font = FontTrueType::New( "eefm-ui-font", scope );
			if ( !font->loadFromFile( selection.font.path ) )
				return;
			mConfig->font.uiPath = selection.font.path;
			mConfig->font.uiSize = selection.size;
			applyUIFont( font.get(), mConfig->font.uiSize );
		} else if ( target == FontTarget::Fallback ) {
			auto font = FontTrueType::New( "eefm-fallback-font", scope );
			if ( !font->loadFromFile( selection.font.path ) )
				return;
			mConfig->font.fallbackPath = selection.font.path;
			// It replaces the previous one (fallbacks are looked up in order).
			auto& service = scope.getFontService();
			auto previous = service.getFallbackFonts();
			for ( const auto& fallback : previous )
				service.removeFallbackFont( fallback.get() );
			service.addFallbackFont( std::move( font ) );
		} else {
			auto font = FontTrueType::New( "eefm-monospace", scope );
			if ( !font->loadFromFile( selection.font.path ) )
				return;
			mConfig->terminal.fontPath = selection.font.path;
			mConfig->terminal.fontSize = selection.size;
			mTerminalFont = font.get();
			FontFamily::loadFromRegular( mTerminalFont );
			if ( mTerminal ) {
				mTerminal->setFont( mTerminalFont );
				mTerminal->setFontSize( PixelDensity::dpToPx( mConfig->terminal.fontSize ) );
			}
		}
		savePreferences();
	} );
	dialog->center();
	dialog->show();
}

void App::showSettings() {
	if ( mSettingsWindow ) {
		mSettingsWindow->toFront();
		return;
	}
	UIWindow::StyleConfig windowStyle{ UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON |
									   UI_WIN_MODAL };
	UIWindow* window = UIWindow::NewOpt( UIWindow::SIMPLE_LAYOUT, windowStyle );
	mSettingsWindow = window;
	window->setId( "settings_panel" );
	window->setTitle( mUI->i18n( "eefm_settings", "Configure eefm" ) );
	const Sizef sceneSize = mUI->getPixelsSize();
	window->setPixelsSize( { eeclamp( sceneSize.getWidth() * 0.82f, 720.f, 1200.f ),
							 eeclamp( sceneSize.getHeight() * 0.82f, 520.f, 850.f ) } );
	window->setMinWindowSize( 640, 440 );
	auto* panel = UISettingsPanel::New( window->getContainer() );
	panel->setId( "settings_panel_content" );
	panel->setSearchResultsText( mUI->i18n( "eefm_search_results", "Search Results" ) );
	const String restart = mUI->i18n( "eefm_restart_required", "Restart required." );

	// Appearance
	panel->addCategory( "appearance.theme", mUI->i18n( "eefm_appearance", "Appearance" ),
						mUI->i18n( "eefm_theme", "Theme" ) );
	panel->addChoice(
		{ "uiColorScheme", "appearance.theme",
		  mUI->i18n( "eefm_ui_color_scheme", "UI Prefers Color Scheme" ),
		  mUI->i18n( "eefm_ui_color_scheme_desc",
					 "Follow the system, or always use the light or dark appearance." ) },
		{ mUI->i18n( "eefm_system", "System" ), mUI->i18n( "eefm_light", "Light" ),
		  mUI->i18n( "eefm_dark", "Dark" ) },
		[this] {
			return mConfig->theme.uiColorScheme == ColorSchemeExtPreference::System	 ? size_t{ 0 }
				   : mConfig->theme.uiColorScheme == ColorSchemeExtPreference::Light ? size_t{ 1 }
																					 : size_t{ 2 };
		},
		[this]( size_t selected ) {
			static constexpr ColorSchemeExtPreference values[] = { ColorSchemeExtPreference::System,
																   ColorSchemeExtPreference::Light,
																   ColorSchemeExtPreference::Dark };
			mConfig->theme.uiColorScheme = values[std::min( selected, size_t{ 2 } )];
			mUI->setColorSchemePreference( mConfig->theme.uiColorScheme );
			mUI->updateWindowTitleBarColor();
			savePreferences();
		} );

	panel->addCategory( "appearance.fonts", mUI->i18n( "eefm_appearance", "Appearance" ),
						mUI->i18n( "eefm_fonts_and_scale", "Fonts & Scale" ) );
	panel->addAction( { "uiFont", "appearance.fonts",
						mUI->i18n( "eefm_ui_font", "UI Font & Size..." ),
						mUI->i18n( "eefm_ui_font_desc", "The font of the interface." ) },
					  mUI->i18n( "eefm_choose_font", "Choose Font..." ),
					  [this] { openFontPicker( FontTarget::UI ); } );
	panel->addAction( { "fallbackFont", "appearance.fonts",
						mUI->i18n( "eefm_fallback_font", "Fallback Font..." ),
						mUI->i18n( "eefm_fallback_font_desc",
								   "The font used for characters the UI font does not have." ) },
					  mUI->i18n( "eefm_choose_font", "Choose Font..." ),
					  [this] { openFontPicker( FontTarget::Fallback ); } );
	panel->addFloat(
		{ "uiFontSize", "appearance.fonts", mUI->i18n( "eefm_ui_font_size", "UI Font Size" ),
		  mUI->i18n( "eefm_ui_font_size_desc", "The font size of the interface." ) },
		6, 72, 0.5,
		[this] {
			if ( mConfig->font.uiSize > 0 )
				return mConfig->font.uiSize;
			auto* theme = mUI->getUIThemeManager()->getDefaultTheme();
			return theme ? PixelDensity::pxToDp( theme->getDefaultFontSize() ) : Float( 11 );
		},
		[this]( double value ) {
			mConfig->font.uiSize = value;
			applyUIFont( nullptr, mConfig->font.uiSize );
			savePreferences();
		} );
	panel->addFloat(
		{ "uiScaleFactor", "appearance.fonts", mUI->i18n( "eefm_ui_scale", "UI Scale Factor" ),
		  mUI->i18n( "eefm_ui_scale_desc", "Scale the whole interface. " ) + restart },
		1, 6, 0.1,
		[this] {
			return mConfig->window.pixelDensity > 0 ? mConfig->window.pixelDensity
													: PixelDensity::getPixelDensity();
		},
		[this]( double value ) {
			mConfig->window.pixelDensity = value;
			savePreferences();
		} );
	panel->addChoice(
		{ "fontHinting", "appearance.fonts", mUI->i18n( "eefm_font_hinting", "Font Hinting" ),
		  mUI->i18n( "eefm_font_hinting_desc", "Align glyphs to the pixel grid." ) },
		{ mUI->i18n( "eefm_none", "None" ), mUI->i18n( "eefm_slight", "Slight" ),
		  mUI->i18n( "eefm_full", "Full" ) },
		[this] { return static_cast<size_t>( mConfig->font.hinting ); },
		[this]( size_t selected ) {
			static constexpr FontHinting values[] = { FontHinting::None, FontHinting::Slight,
													  FontHinting::Full };
			mConfig->font.hinting = values[std::min( selected, size_t{ 2 } )];
			applyFontRendering();
			savePreferences();
		} );
	panel->addChoice(
		{ "fontAntialiasing", "appearance.fonts",
		  mUI->i18n( "eefm_font_antialiasing", "Font Anti-Aliasing" ),
		  mUI->i18n( "eefm_font_antialiasing_desc", "How glyph edges are smoothed." ) },
		{ mUI->i18n( "eefm_none", "None" ), mUI->i18n( "eefm_grayscale", "Grayscale" ),
		  mUI->i18n( "eefm_subpixel", "Subpixel" ) },
		[this] { return static_cast<size_t>( mConfig->font.antialiasing ); },
		[this]( size_t selected ) {
			static constexpr FontAntialiasing values[] = {
				FontAntialiasing::None, FontAntialiasing::Grayscale, FontAntialiasing::Subpixel };
			mConfig->font.antialiasing = values[std::min( selected, size_t{ 2 } )];
			applyFontRendering();
			savePreferences();
		} );

	// Files
	panel->addCategory( "files.behavior", mUI->i18n( "eefm_files", "Files" ),
						mUI->i18n( "eefm_behavior", "Behavior" ) );
	panel->addBool( { "singleClick", "files.behavior",
					  mUI->i18n( "eefm_single_click", "Open with a Single Click" ),
					  mUI->i18n( "eefm_single_click_desc",
								 "Open files and folders with one click instead of two." ) },
					&mConfig->files.singleClick, [this]( bool ) {
						applyFilesConfig();
						savePreferences();
					} );
	panel->addBool( { "countFolderItems", "files.behavior",
					  mUI->i18n( "eefm_count_folder_items", "Count Items in Folders" ),
					  mUI->i18n( "eefm_count_folder_items_desc",
								 "Show how many items local folders contain in the Size "
								 "column." ) },
					&mConfig->files.countFolderItems, [this]( bool ) {
						applyFilesConfig();
						savePreferences();
					} );
	panel->addBool( { "smoothScroll", "files.behavior",
					  mUI->i18n( "eefm_smooth_scroll", "Smooth Scrolling" ),
					  mUI->i18n( "eefm_smooth_scroll_desc",
								 "Animate scrolling from mouse wheels and trackpads." ) },
					&mConfig->files.smoothScroll, [this]( bool value ) {
						mUI->setSmoothScrollEnabled( value, true );
						savePreferences();
					} );
	panel->addCategory( "files.icons", mUI->i18n( "eefm_files", "Files" ),
						mUI->i18n( "eefm_icons", "Icons" ) );
	panel->addBool(
		{ "systemIcons", "files.icons",
		  mUI->i18n( "eefm_system_icons", "Use the System Icon Theme" ),
		  mUI->i18n( "eefm_system_icons_desc",
					 "Show the desktop's icons for files, folders and places (the icon theme on "
					 "Linux, the shell's icons on Windows and macOS) instead of eefm's own." ) },
		&mConfig->files.systemIcons, [this]( bool ) {
			applyFilesConfig();
			savePreferences();
		} );

	// Terminal
	panel->addCategory( "terminal.behavior", mUI->i18n( "eefm_terminal", "Terminal" ),
						mUI->i18n( "eefm_behavior", "Behavior" ) );
	panel->addBool( { "followActivePane", "terminal.behavior",
					  mUI->i18n( "eefm_follow_active_pane", "Follow the Active Pane" ),
					  mUI->i18n( "eefm_follow_active_pane_desc",
								 "Change the shell's folder when the active pane changes, "
								 "while the shell waits at its prompt." ) },
					&mConfig->terminal.followActivePane, [this]( bool value ) {
						if ( value )
							syncTerminalDirectory();
						savePreferences();
					} );
	const std::vector<std::string> knownShells{ "bash", "sh",  "zsh",  "fish", "nu",		"csh",
												"tcsh", "ksh", "dash", "cmd",  "powershell" };
	const String defaultShell = mUI->i18n( "eefm_default_shell", "Default" );
	std::vector<String> shells{ defaultShell };
	for ( const auto& shell : knownShells ) {
		std::string path = Sys::which( shell );
		if ( !path.empty() )
			shells.emplace_back( path );
	}
	if ( !mConfig->terminal.shell.empty() &&
		 std::find( shells.begin(), shells.end(), String( mConfig->terminal.shell ) ) ==
			 shells.end() )
		shells.emplace_back( mConfig->terminal.shell );
	panel->addEditableChoice(
		{ "terminalShell", "terminal.behavior", mUI->i18n( "eefm_terminal_shell", "Shell" ),
		  mUI->i18n( "eefm_terminal_shell_desc",
					 "The shell started by the terminal panel (Default: your shell). Applies to "
					 "new terminals." ) },
		shells,
		[this, defaultShell] {
			return mConfig->terminal.shell.empty() ? defaultShell
												   : String( mConfig->terminal.shell );
		},
		[this, defaultShell]( const String& selection ) {
			std::string shell = selection == defaultShell ? std::string() : selection.toUtf8();
			if ( !shell.empty() && Sys::which( shell ).empty() && !FileSystem::fileExists( shell ) )
				return false;
			mConfig->terminal.shell = shell;
			savePreferences();
			return true;
		} );
	panel->addText(
		{ "terminalShellArguments", "terminal.behavior",
		  mUI->i18n( "eefm_terminal_shell_arguments", "Shell Arguments" ),
		  mUI->i18n( "eefm_terminal_shell_arguments_desc",
					 "Arguments passed to the shell. Applies to new terminals." ) },
		[this] { return mConfig->terminal.shellArguments; },
		[this]( const std::string& value ) {
			mConfig->terminal.shellArguments = value;
			savePreferences();
			return true;
		},
		true );
	panel->addInteger(
		{ "terminalScrollback", "terminal.behavior",
		  mUI->i18n( "eefm_terminal_scrollback", "Scrollback" ),
		  mUI->i18n( "eefm_terminal_scrollback_desc",
					 "Lines of history kept. Applies to new terminals." ) },
		0, std::numeric_limits<int>::max(),
		[this] {
			return static_cast<int>( std::min<size_t>( mConfig->terminal.historySize,
													   std::numeric_limits<int>::max() ) );
		},
		[this]( int value ) {
			mConfig->terminal.historySize = value;
			savePreferences();
		} );

	panel->addCategory( "terminal.appearance", mUI->i18n( "eefm_terminal", "Terminal" ),
						mUI->i18n( "eefm_appearance", "Appearance" ) );
	panel->addAction(
		{ "terminalFont", "terminal.appearance",
		  mUI->i18n( "eefm_terminal_font", "Terminal Font & Size..." ),
		  mUI->i18n( "eefm_terminal_font_desc", "The monospace font of the terminal." ) },
		mUI->i18n( "eefm_choose_font", "Choose Font..." ),
		[this] { openFontPicker( FontTarget::Terminal ); } );
	panel->addFloat(
		{ "terminalFontSize", "terminal.appearance",
		  mUI->i18n( "eefm_terminal_font_size", "Terminal Font Size" ),
		  mUI->i18n( "eefm_terminal_font_size_desc", "The font size of the terminal." ) },
		6, 72, 0.5, [this] { return mConfig->terminal.fontSize; },
		[this]( double value ) {
			mConfig->terminal.fontSize = value;
			if ( mTerminal )
				mTerminal->setFontSize( PixelDensity::dpToPx( value ) );
			savePreferences();
		} );
	std::vector<String> schemeNames{ mUI->i18n( "eefm_default", "Default" ) };
	std::vector<std::string> schemeIds{ "" };
	for ( const auto& [name, scheme] : terminalColorSchemes() ) {
		schemeNames.emplace_back( name );
		schemeIds.emplace_back( name );
	}
	panel->addChoice(
		{ "terminalColorScheme", "terminal.appearance",
		  mUI->i18n( "eefm_terminal_color_scheme", "Color Scheme" ),
		  mUI->i18n( "eefm_terminal_color_scheme_desc", "The colors of the terminal." ) },
		schemeNames,
		[this, schemeIds] {
			auto found =
				std::find( schemeIds.begin(), schemeIds.end(), mConfig->terminal.colorScheme );
			return found == schemeIds.end() ? size_t{ 0 }
											: static_cast<size_t>( found - schemeIds.begin() );
		},
		[this, schemeIds]( size_t selected ) {
			mConfig->terminal.colorScheme = schemeIds[std::min( selected, schemeIds.size() - 1 )];
			applyTerminalConfig();
			savePreferences();
		} );
	panel->addChoice(
		{ "terminalCursorStyle", "terminal.appearance",
		  mUI->i18n( "eefm_cursor_style", "Cursor Style" ),
		  mUI->i18n( "eefm_cursor_style_desc", "The shape and blinking of the cursor." ) },
		{ mUI->i18n( "eefm_blinking_block", "Blinking Block" ),
		  mUI->i18n( "eefm_steady_block", "Steady Block" ),
		  mUI->i18n( "eefm_blink_underline", "Blinking Underline" ),
		  mUI->i18n( "eefm_steady_underline", "Steady Underline" ),
		  mUI->i18n( "eefm_blink_bar", "Blinking Bar" ),
		  mUI->i18n( "eefm_steady_bar", "Steady Bar" ) },
		[this] {
			static constexpr TerminalCursorMode modes[] = {
				TerminalCursorMode::BlinkingBlock,	TerminalCursorMode::SteadyBlock,
				TerminalCursorMode::BlinkUnderline, TerminalCursorMode::SteadyUnderline,
				TerminalCursorMode::BlinkBar,		TerminalCursorMode::SteadyBar };
			auto found =
				std::find( std::begin( modes ), std::end( modes ), mConfig->terminal.cursorStyle );
			return found == std::end( modes ) ? size_t{ 0 }
											  : static_cast<size_t>( found - std::begin( modes ) );
		},
		[this]( size_t selected ) {
			static constexpr TerminalCursorMode modes[] = {
				TerminalCursorMode::BlinkingBlock,	TerminalCursorMode::SteadyBlock,
				TerminalCursorMode::BlinkUnderline, TerminalCursorMode::SteadyUnderline,
				TerminalCursorMode::BlinkBar,		TerminalCursorMode::SteadyBar };
			mConfig->terminal.cursorStyle = modes[std::min( selected, size_t{ 5 } )];
			applyTerminalConfig();
			savePreferences();
		} );
	panel->addChoice(
		{ "terminalScrollbarMode", "terminal.appearance",
		  mUI->i18n( "eefm_scrollbar_mode", "Scrollbar" ),
		  mUI->i18n( "eefm_scrollbar_mode_desc", "When the terminal scrollbar is shown." ) },
		{ mUI->i18n( "eefm_auto", "Auto" ), mUI->i18n( "eefm_always_visible", "Always Visible" ),
		  mUI->i18n( "eefm_always_hidden", "Always Hidden" ) },
		[this] {
			return mConfig->terminal.scrollBarMode == ScrollBarMode::Auto		? size_t{ 0 }
				   : mConfig->terminal.scrollBarMode == ScrollBarMode::AlwaysOn ? size_t{ 1 }
																				: size_t{ 2 };
		},
		[this]( size_t selected ) {
			mConfig->terminal.scrollBarMode = selected == 0	  ? ScrollBarMode::Auto
											  : selected == 1 ? ScrollBarMode::AlwaysOn
															  : ScrollBarMode::AlwaysOff;
			applyTerminalConfig();
			savePreferences();
		} );

	// Window
	panel->addCategory( "window.renderer", mUI->i18n( "eefm_window", "Window" ),
						mUI->i18n( "eefm_renderer", "Renderer" ) );
	panel->addBool(
		{ "vsync", "window.renderer", mUI->i18n( "eefm_vsync", "VSync" ),
		  mUI->i18n( "eefm_vsync_desc", "Synchronize rendering with the display. " ) + restart },
		&mConfig->window.vsync, [this]( bool ) { savePreferences(); } );
	const String monitorRate = mUI->i18n( "eefm_monitor_refresh_rate", "Monitor Refresh Rate" );
	const String unlimited = mUI->i18n( "eefm_unlimited", "Unlimited" );
	panel->addEditableChoice(
		{ "frameRateLimit", "window.renderer",
		  mUI->i18n( "eefm_frame_rate_limit", "Frame Rate Limit" ),
		  mUI->i18n( "eefm_frame_rate_limit_desc",
					 "Limit the frames per second, follow the monitor, or do not limit them." ) },
		{ monitorRate, unlimited, "30", "60", "75", "120", "144", "165", "240" },
		[this, monitorRate, unlimited] {
			return mConfig->window.maxFPS ==
						   static_cast<Uint32>( ContextSettings::FrameRateLimitScreenRefreshRate )
					   ? monitorRate
				   : mConfig->window.maxFPS == 0
					   ? unlimited
					   : String( String::toString( mConfig->window.maxFPS ) );
		},
		[this, monitorRate, unlimited]( const String& selection ) {
			Uint32 value;
			if ( selection == monitorRate )
				value = ContextSettings::FrameRateLimitScreenRefreshRate;
			else if ( selection == unlimited )
				value = 0;
			else if ( !String::fromString( value, selection ) || value > 1000 )
				return false;
			mConfig->window.maxFPS = value;
			mApp->getWindow()->setFrameRateLimit( value );
			savePreferences();
			return true;
		} );
	auto rendererVersions = Renderer::getAvailableGraphicsLibraryVersions();
	std::vector<String> rendererNames;
	for ( auto version : rendererVersions )
		rendererNames.emplace_back( Renderer::graphicsLibraryVersionToString( version ) );
	if ( !rendererVersions.empty() )
		panel->addChoice(
			{ "rendererVersion", "window.renderer",
			  mUI->i18n( "eefm_renderer_version", "Renderer Version" ),
			  mUI->i18n( "eefm_renderer_version_desc", "The graphics API. " ) + restart },
			rendererNames,
			[this, rendererVersions] {
				auto found = std::find( rendererVersions.begin(), rendererVersions.end(),
										mConfig->window.rendererVersion );
				return found == rendererVersions.end()
						   ? size_t{ 0 }
						   : static_cast<size_t>( found - rendererVersions.begin() );
			},
			[this, rendererVersions]( size_t selected ) {
				mConfig->window.rendererVersion =
					rendererVersions[std::min( selected, rendererVersions.size() - 1 )];
				savePreferences();
			} );
	panel->addChoice(
		{ "multisamples", "window.renderer",
		  mUI->i18n( "eefm_multisamples", "Multisample Anti-Aliasing" ),
		  mUI->i18n( "eefm_multisamples_desc", "Renderer multisampling. " ) + restart },
		{ "0", "2", "4", "8", "16" },
		[this] {
			static constexpr Uint32 values[] = { 0, 2, 4, 8, 16 };
			auto found =
				std::find( std::begin( values ), std::end( values ), mConfig->window.multisamples );
			return found == std::end( values )
					   ? size_t{ 0 }
					   : static_cast<size_t>( found - std::begin( values ) );
		},
		[this]( size_t selected ) {
			static constexpr Uint32 values[] = { 0, 2, 4, 8, 16 };
			mConfig->window.multisamples = values[std::min( selected, size_t{ 4 } )];
			savePreferences();
		} );

	panel->build();
	window->on( Event::OnWindowReady, [panel]( const Event* ) {
		panel->runOnMainThread( [panel] { panel->focusSearch(); } );
	} );
	window->setKeyBindingCommand( "closeWindow", [window] { window->closeWindow(); } );
	window->getKeyBindings().addKeybind( { KEY_ESCAPE }, "closeWindow" );
	window->on( Event::OnWindowClose, [this]( const Event* ) {
		mSettingsWindow = nullptr;
		savePreferences();
	} );
	window->center();
	window->showWhenReady();
	panel->focusSearch();
}

} // namespace eefm
