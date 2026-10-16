#include "settingsactions.hpp"
#include "app.hpp"
#include "panemanager.hpp"
#include "placescontroller.hpp"
#include "terminalpanel.hpp"

#include <eepp/ui/tools/uifontpickerdialog.hpp>
#include <eepp/ui/tools/uisettingspanel.hpp>
#include <eterm/ui/uiterminal.hpp>

#include <limits>

using namespace EE::UI::Tools;
using namespace eterm::Terminal;
using namespace eterm::UI;

namespace eefm {

SettingsActions::SettingsActions( App& app ) : mApp( app ), mUI( app.ui() ) {}

void SettingsActions::savePreferences() {
	mApp.config().save();
}

void SettingsActions::applyFontRendering() {
	mUI->setFontRendering( mApp.config().font.hinting, mApp.config().font.antialiasing );
	mApp.terminal().syncFontRendering();
}

void SettingsActions::applyUIFont( Font* font, Float sizeDp ) {
	mUI->applyDefaultFont( font, PixelDensity::dpToPx( sizeDp ) );
}

void SettingsActions::applyFilesConfig() {
	mApp.clearFileIcons();
	mApp.updateIconStyle();
	for ( auto& pane : mApp.panes().all() ) {
		if ( !pane )
			continue;
		for ( auto& tab : pane->tabList ) {
			tab->view->setSingleClickNavigation( mApp.config().files.singleClick );
			tab->model->setCountDirectoryItems( mApp.config().files.countFolderItems &&
												tab->model->uri().getScheme() == "file" );
			tab->view->invalidateDraw();
		}
	}
	mApp.places().refresh();
}

void SettingsActions::openFontPicker( FontTarget target ) {
	const Uint32 flags = UIFontPickerDialog::ShowStyle |
						 ( target == FontTarget::Fallback ? 0 : UIFontPickerDialog::ShowSize ) |
						 ( target == FontTarget::Terminal ? UIFontPickerDialog::MonospaceOnly : 0 );
	auto* dialog = UIFontPickerDialog::New( flags );
	dialog->setTitle( mUI->i18n( "eefm_select_font", "Select Font" ) );
	dialog->setCloseShortcut( KEY_ESCAPE );
	std::string current = target == FontTarget::UI		   ? mApp.config().font.uiPath
						  : target == FontTarget::Fallback ? mApp.config().font.fallbackPath
														   : mApp.config().terminal.fontPath;
	// Not configured: the font in use (the bundled one, or the first explicit fallback).
	if ( current.empty() ) {
		if ( target == FontTarget::UI ) {
			current = FontTrueType::filePathOf( mUI->getUIThemeManager()->getDefaultFont() );
		} else if ( target == FontTarget::Fallback ) {
			const auto& fallbacks = mUI->getResourceScope()->getFontService().getFallbackFonts();
			if ( !fallbacks.empty() )
				current = FontTrueType::filePathOf( fallbacks.front().get() );
		} else {
			FontTrueType* terminalFont = mApp.terminal().terminalFont();
			current = terminalFont ? FontTrueType::filePathOf( terminalFont )
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
				? ( mApp.config().font.uiSize > 0
						? mApp.config().font.uiSize
						: PixelDensity::pxToDp( mUI->getUIThemeManager()->getDefaultFontSize() ) )
				: mApp.config().terminal.fontSize );
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
			mApp.config().font.uiPath = selection.font.path;
			mApp.config().font.uiSize = selection.size;
			applyUIFont( font.get(), mApp.config().font.uiSize );
		} else if ( target == FontTarget::Fallback ) {
			auto font = FontTrueType::New( "eefm-fallback-font", scope );
			if ( !font->loadFromFile( selection.font.path ) )
				return;
			mApp.config().font.fallbackPath = selection.font.path;
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
			mApp.config().terminal.fontPath = selection.font.path;
			mApp.config().terminal.fontSize = selection.size;
			mApp.terminal().setTerminalFont( font.get() );
		}
		savePreferences();
	} );
	dialog->center();
	dialog->show();
}

void SettingsActions::showSettings() {
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
			return mApp.config().theme.uiColorScheme == ColorSchemeExtPreference::System
					   ? size_t{ 0 }
				   : mApp.config().theme.uiColorScheme == ColorSchemeExtPreference::Light
					   ? size_t{ 1 }
					   : size_t{ 2 };
		},
		[this]( size_t selected ) {
			static constexpr ColorSchemeExtPreference values[] = { ColorSchemeExtPreference::System,
																   ColorSchemeExtPreference::Light,
																   ColorSchemeExtPreference::Dark };
			mApp.config().theme.uiColorScheme = values[std::min( selected, size_t{ 2 } )];
			mUI->setColorSchemePreference( mApp.config().theme.uiColorScheme );
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
			if ( mApp.config().font.uiSize > 0 )
				return mApp.config().font.uiSize;
			auto* theme = mUI->getUIThemeManager()->getDefaultTheme();
			return theme ? PixelDensity::pxToDp( theme->getDefaultFontSize() ) : Float( 11 );
		},
		[this]( double value ) {
			mApp.config().font.uiSize = value;
			applyUIFont( nullptr, mApp.config().font.uiSize );
			savePreferences();
		} );
	panel->addFloat(
		{ "uiScaleFactor", "appearance.fonts", mUI->i18n( "eefm_ui_scale", "UI Scale Factor" ),
		  mUI->i18n( "eefm_ui_scale_desc", "Scale the whole interface. " ) + restart },
		1, 6, 0.1,
		[this] {
			return mApp.config().window.pixelDensity > 0 ? mApp.config().window.pixelDensity
														 : PixelDensity::getPixelDensity();
		},
		[this]( double value ) {
			mApp.config().window.pixelDensity = value;
			savePreferences();
		} );
	panel->addChoice(
		{ "fontHinting", "appearance.fonts", mUI->i18n( "eefm_font_hinting", "Font Hinting" ),
		  mUI->i18n( "eefm_font_hinting_desc", "Align glyphs to the pixel grid." ) },
		{ mUI->i18n( "eefm_none", "None" ), mUI->i18n( "eefm_slight", "Slight" ),
		  mUI->i18n( "eefm_full", "Full" ) },
		[this] { return static_cast<size_t>( mApp.config().font.hinting ); },
		[this]( size_t selected ) {
			static constexpr FontHinting values[] = { FontHinting::None, FontHinting::Slight,
													  FontHinting::Full };
			mApp.config().font.hinting = values[std::min( selected, size_t{ 2 } )];
			applyFontRendering();
			savePreferences();
		} );
	panel->addChoice(
		{ "fontAntialiasing", "appearance.fonts",
		  mUI->i18n( "eefm_font_antialiasing", "Font Anti-Aliasing" ),
		  mUI->i18n( "eefm_font_antialiasing_desc", "How glyph edges are smoothed." ) },
		{ mUI->i18n( "eefm_none", "None" ), mUI->i18n( "eefm_grayscale", "Grayscale" ),
		  mUI->i18n( "eefm_subpixel", "Subpixel" ) },
		[this] { return static_cast<size_t>( mApp.config().font.antialiasing ); },
		[this]( size_t selected ) {
			static constexpr FontAntialiasing values[] = {
				FontAntialiasing::None, FontAntialiasing::Grayscale, FontAntialiasing::Subpixel };
			mApp.config().font.antialiasing = values[std::min( selected, size_t{ 2 } )];
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
					&mApp.config().files.singleClick, [this]( bool ) {
						applyFilesConfig();
						savePreferences();
					} );
	panel->addBool( { "countFolderItems", "files.behavior",
					  mUI->i18n( "eefm_count_folder_items", "Count Items in Folders" ),
					  mUI->i18n( "eefm_count_folder_items_desc",
								 "Show how many items local folders contain in the Size "
								 "column." ) },
					&mApp.config().files.countFolderItems, [this]( bool ) {
						applyFilesConfig();
						savePreferences();
					} );
	panel->addBool( { "smoothScroll", "files.behavior",
					  mUI->i18n( "eefm_smooth_scroll", "Smooth Scrolling" ),
					  mUI->i18n( "eefm_smooth_scroll_desc",
								 "Animate scrolling from mouse wheels and trackpads." ) },
					&mApp.config().files.smoothScroll, [this]( bool value ) {
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
		&mApp.config().files.systemIcons, [this]( bool ) {
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
					&mApp.config().terminal.followActivePane, [this]( bool value ) {
						if ( value )
							mApp.terminal().syncDirectory();
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
	if ( !mApp.config().terminal.shell.empty() &&
		 std::find( shells.begin(), shells.end(), String( mApp.config().terminal.shell ) ) ==
			 shells.end() )
		shells.emplace_back( mApp.config().terminal.shell );
	panel->addEditableChoice(
		{ "terminalShell", "terminal.behavior", mUI->i18n( "eefm_terminal_shell", "Shell" ),
		  mUI->i18n( "eefm_terminal_shell_desc",
					 "The shell started by the terminal panel (Default: your shell). Applies to "
					 "new terminals." ) },
		shells,
		[this, defaultShell] {
			return mApp.config().terminal.shell.empty() ? defaultShell
														: String( mApp.config().terminal.shell );
		},
		[this, defaultShell]( const String& selection ) {
			std::string shell = selection == defaultShell ? std::string() : selection.toUtf8();
			if ( !shell.empty() && Sys::which( shell ).empty() && !FileSystem::fileExists( shell ) )
				return false;
			mApp.config().terminal.shell = shell;
			savePreferences();
			return true;
		} );
	panel->addText(
		{ "terminalShellArguments", "terminal.behavior",
		  mUI->i18n( "eefm_terminal_shell_arguments", "Shell Arguments" ),
		  mUI->i18n( "eefm_terminal_shell_arguments_desc",
					 "Arguments passed to the shell. Applies to new terminals." ) },
		[this] { return mApp.config().terminal.shellArguments; },
		[this]( const std::string& value ) {
			mApp.config().terminal.shellArguments = value;
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
			return static_cast<int>( std::min<size_t>( mApp.config().terminal.historySize,
													   std::numeric_limits<int>::max() ) );
		},
		[this]( int value ) {
			mApp.config().terminal.historySize = value;
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
		6, 72, 0.5, [this] { return mApp.config().terminal.fontSize; },
		[this]( double value ) {
			mApp.config().terminal.fontSize = value;
			mApp.terminal().setFontSize( value );
			savePreferences();
		} );
	std::vector<String> schemeNames{ mUI->i18n( "eefm_default", "Default" ) };
	std::vector<std::string> schemeIds{ "" };
	for ( const auto& [name, scheme] : mApp.terminal().colorSchemes() ) {
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
				std::find( schemeIds.begin(), schemeIds.end(), mApp.config().terminal.colorScheme );
			return found == schemeIds.end() ? size_t{ 0 }
											: static_cast<size_t>( found - schemeIds.begin() );
		},
		[this, schemeIds]( size_t selected ) {
			mApp.config().terminal.colorScheme =
				schemeIds[std::min( selected, schemeIds.size() - 1 )];
			mApp.terminal().applyConfig();
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
			auto found = std::find( std::begin( modes ), std::end( modes ),
									mApp.config().terminal.cursorStyle );
			return found == std::end( modes ) ? size_t{ 0 }
											  : static_cast<size_t>( found - std::begin( modes ) );
		},
		[this]( size_t selected ) {
			static constexpr TerminalCursorMode modes[] = {
				TerminalCursorMode::BlinkingBlock,	TerminalCursorMode::SteadyBlock,
				TerminalCursorMode::BlinkUnderline, TerminalCursorMode::SteadyUnderline,
				TerminalCursorMode::BlinkBar,		TerminalCursorMode::SteadyBar };
			mApp.config().terminal.cursorStyle = modes[std::min( selected, size_t{ 5 } )];
			mApp.terminal().applyConfig();
			savePreferences();
		} );
	panel->addChoice(
		{ "terminalScrollbarMode", "terminal.appearance",
		  mUI->i18n( "eefm_scrollbar_mode", "Scrollbar" ),
		  mUI->i18n( "eefm_scrollbar_mode_desc", "When the terminal scrollbar is shown." ) },
		{ mUI->i18n( "eefm_auto", "Auto" ), mUI->i18n( "eefm_always_visible", "Always Visible" ),
		  mUI->i18n( "eefm_always_hidden", "Always Hidden" ) },
		[this] {
			return mApp.config().terminal.scrollBarMode == ScrollBarMode::Auto		 ? size_t{ 0 }
				   : mApp.config().terminal.scrollBarMode == ScrollBarMode::AlwaysOn ? size_t{ 1 }
																					 : size_t{ 2 };
		},
		[this]( size_t selected ) {
			mApp.config().terminal.scrollBarMode = selected == 0   ? ScrollBarMode::Auto
												   : selected == 1 ? ScrollBarMode::AlwaysOn
																   : ScrollBarMode::AlwaysOff;
			mApp.terminal().applyConfig();
			savePreferences();
		} );

	// Window
	panel->addCategory( "window.renderer", mUI->i18n( "eefm_window", "Window" ),
						mUI->i18n( "eefm_renderer", "Renderer" ) );
	panel->addBool(
		{ "vsync", "window.renderer", mUI->i18n( "eefm_vsync", "VSync" ),
		  mUI->i18n( "eefm_vsync_desc", "Synchronize rendering with the display. " ) + restart },
		&mApp.config().window.vsync, [this]( bool ) { savePreferences(); } );
	const String monitorRate = mUI->i18n( "eefm_monitor_refresh_rate", "Monitor Refresh Rate" );
	const String unlimited = mUI->i18n( "eefm_unlimited", "Unlimited" );
	panel->addEditableChoice(
		{ "frameRateLimit", "window.renderer",
		  mUI->i18n( "eefm_frame_rate_limit", "Frame Rate Limit" ),
		  mUI->i18n( "eefm_frame_rate_limit_desc",
					 "Limit the frames per second, follow the monitor, or do not limit them." ) },
		{ monitorRate, unlimited, "30", "60", "75", "120", "144", "165", "240" },
		[this, monitorRate, unlimited] {
			return mApp.config().window.maxFPS ==
						   static_cast<Uint32>( ContextSettings::FrameRateLimitScreenRefreshRate )
					   ? monitorRate
				   : mApp.config().window.maxFPS == 0
					   ? unlimited
					   : String( String::toString( mApp.config().window.maxFPS ) );
		},
		[this, monitorRate, unlimited]( const String& selection ) {
			Uint32 value;
			if ( selection == monitorRate )
				value = ContextSettings::FrameRateLimitScreenRefreshRate;
			else if ( selection == unlimited )
				value = 0;
			else if ( !String::fromString( value, selection ) || value > 1000 )
				return false;
			mApp.config().window.maxFPS = value;
			mApp.window()->setFrameRateLimit( value );
			savePreferences();
			return true;
		} );
	auto rendererVersions = Renderer::getAvailableGraphicsLibraryVersions();
	std::vector<String> rendererNames;
	for ( auto version : rendererVersions )
		rendererNames.emplace_back( Renderer::graphicsLibraryVersionToString( version ) );
	if ( !rendererVersions.empty() ) {
		panel->addChoice(
			{ "rendererVersion", "window.renderer",
			  mUI->i18n( "eefm_renderer_version", "Renderer Version" ),
			  mUI->i18n( "eefm_renderer_version_desc", "The graphics API. " ) + restart },
			rendererNames,
			[this, rendererVersions] {
				auto found = std::find( rendererVersions.begin(), rendererVersions.end(),
										mApp.config().window.rendererVersion );
				return found == rendererVersions.end()
						   ? size_t{ 0 }
						   : static_cast<size_t>( found - rendererVersions.begin() );
			},
			[this, rendererVersions]( size_t selected ) {
				mApp.config().window.rendererVersion =
					rendererVersions[std::min( selected, rendererVersions.size() - 1 )];
				savePreferences();
			} );
	}
	panel->addChoice(
		{ "multisamples", "window.renderer",
		  mUI->i18n( "eefm_multisamples", "Multisample Anti-Aliasing" ),
		  mUI->i18n( "eefm_multisamples_desc", "Renderer multisampling. " ) + restart },
		{ "0", "2", "4", "8", "16" },
		[this] {
			static constexpr Uint32 values[] = { 0, 2, 4, 8, 16 };
			auto found = std::find( std::begin( values ), std::end( values ),
									mApp.config().window.multisamples );
			return found == std::end( values )
					   ? size_t{ 0 }
					   : static_cast<size_t>( found - std::begin( values ) );
		},
		[this]( size_t selected ) {
			static constexpr Uint32 values[] = { 0, 2, 4, 8, 16 };
			mApp.config().window.multisamples = values[std::min( selected, size_t{ 4 } )];
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
