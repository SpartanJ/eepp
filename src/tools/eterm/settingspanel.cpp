#include "settingspanel.hpp"
#include "eterm.hpp"

#include <eepp/ui/tools/uisettingspanel.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiwindow.hpp>

#include <string>

using namespace EE::UI::Tools;

namespace eterm {

UIWindow* SettingsPanel::create( App& app ) {
	UIWindow::StyleConfig windowStyle{ UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON |
									   UI_WIN_MODAL };
	auto settingsWindow = UIWindow::NewOpt( UIWindow::SIMPLE_LAYOUT, windowStyle );
	settingsWindow->setId( "settings_panel" );
	settingsWindow->setTitle( app.i18n( "settings", "Settings" ) );
	const auto sceneSize = app.scene->getPixelsSize();
	settingsWindow->setPixelsSize( { eeclamp( sceneSize.getWidth() * 0.82f, 720.f, 1200.f ),
									 eeclamp( sceneSize.getHeight() * 0.82f, 520.f, 850.f ) } );
	settingsWindow->setMinWindowSize( 640, 440 );
	auto* panel = UISettingsPanel::New( settingsWindow->getContainer() );
	panel->setSearchResultsText( app.i18n( "search_results", "Search Results" ) );

	panel->addCategory( "appearance.theme", app.i18n( "appearance", "Appearance" ),
						app.i18n( "theme_and_language", "Theme & Language" ) );
	panel->addChoice(
		{ "uiColorScheme", "appearance.theme",
		  app.i18n( "ui_prefes_color_scheme", "UI Prefers Color Scheme" ),
		  app.i18n(
			  "ui_prefers_color_scheme_desc",
			  "Choose whether the interface follows the system, light, or dark appearance." ) },
		{ app.i18n( "system", "System" ), app.i18n( "light", "Light" ),
		  app.i18n( "dark", "Dark" ) },
		[&app] {
			return app.config->theme.uiColorScheme == ColorSchemeExtPreference::System ? size_t{ 0 }
				   : app.config->theme.uiColorScheme == ColorSchemeExtPreference::Light
					   ? size_t{ 1 }
					   : size_t{ 2 };
		},
		[&app]( size_t selected ) {
			static constexpr ColorSchemeExtPreference values[] = { ColorSchemeExtPreference::System,
																   ColorSchemeExtPreference::Light,
																   ColorSchemeExtPreference::Dark };
			app.config->theme.uiColorScheme = values[std::min( selected, size_t{ 2 } )];
			app.scene->setColorSchemePreference( app.config->theme.uiColorScheme );
			app.savePreferences();
		} );
	panel->addCategory( "appearance.fonts", app.i18n( "appearance", "Appearance" ),
						app.i18n( "fonts_and_scale", "Fonts & Scale" ) );
	panel->addAction(
		{ "uiFont", "appearance.fonts",
		  app.i18n( "ui_font_and_size_ellipsis", "UI Font & Size..." ),
		  app.i18n( "ui_font_desc", "Choose the proportional font used by the interface." ) },
		app.i18n( "choose_font", "Choose Font..." ),
		[&app] { app.settingsActions->openFontPicker( true ); } );
	panel->addAction(
		{ "terminalFont", "appearance.fonts",
		  app.i18n( "terminal_font_and_size_ellipsis", "Terminal Font & Size..." ),
		  app.i18n( "terminal_font_desc", "Choose the monospace font used by terminals." ) },
		app.i18n( "choose_font", "Choose Font..." ),
		[&app] { app.settingsActions->openFontPicker( false ); } );
	panel->addAction(
		{ "fallbackFont", "appearance.fonts",
		  app.i18n( "fallback_font_ellipsis", "Fallback Font..." ),
		  app.i18n( "fallback_font_desc", "Choose the font used for missing glyphs." ) },
		app.i18n( "choose_font", "Choose Font..." ),
		[&app] { app.settingsActions->openFontPicker( false, true ); } );
	panel->addFloat(
		{ "uiFontSize", "appearance.fonts", app.i18n( "ui_font_size", "UI Font Size" ),
		  app.i18n( "ui_font_size_desc", "Set the font size used by the application UI." ) },
		6, 72, 0.5, [&app] { return app.config->font.uiSize; },
		[&app]( double value ) { app.settingsActions->setUIFontSize( value ); } );
	panel->addFloat(
		{ "terminalFontSize", "appearance.fonts",
		  app.i18n( "terminal_font_size", "Terminal Font Size" ),
		  app.i18n( "terminal_font_size_desc", "Set the default terminal font size." ) },
		6, 72, 0.5, [&app] { return app.config->font.size; },
		[&app]( double value ) { app.settingsActions->setTerminalFontSize( value ); } );
	panel->addFloat(
		{ "uiScaleFactor", "appearance.fonts", app.i18n( "ui_scale_factor", "UI Scale Factor" ),
		  app.i18n( "ui_scale_factor_desc",
					"Scale the complete user interface. Restart required." ) },
		1, 6, 0.1,
		[&app] {
			return app.config->window.pixelDensity > 0 ? app.config->window.pixelDensity
													   : PixelDensity::getPixelDensity();
		},
		[&app]( double value ) {
			app.config->window.pixelDensity = value;
			app.savePreferences();
		} );
	panel->addChoice(
		{ "fontHinting", "appearance.fonts", app.i18n( "ui_font_hint", "Font Hinting" ),
		  app.i18n( "ui_font_hint_desc", "Control glyph alignment to the pixel grid." ) },
		{ app.i18n( "none", "None" ), app.i18n( "slight", "Slight" ), app.i18n( "full", "Full" ) },
		[&app] { return static_cast<size_t>( app.config->font.hinting ); },
		[&app]( size_t selected ) {
			static constexpr FontHinting values[] = { FontHinting::None, FontHinting::Slight,
													  FontHinting::Full };
			app.config->font.hinting = values[std::min( selected, size_t{ 2 } )];
			app.scene->getResourceScope()->getFontService().setHinting( app.config->font.hinting );
			app.forEachTerminal(
				[]( UITerminal* terminal ) { terminal->syncFontRenderingConfig(); } );
			app.savePreferences();
		} );
	panel->addChoice(
		{ "fontAntialiasing", "appearance.fonts",
		  app.i18n( "ui_font_antialiasing", "Font Anti-Aliasing" ),
		  app.i18n( "ui_font_antialiasing_desc", "Choose how glyph edges are smoothed." ) },
		{ app.i18n( "none", "None" ), app.i18n( "grayscale", "Grayscale" ),
		  app.i18n( "subpixel", "Subpixel" ) },
		[&app] { return static_cast<size_t>( app.config->font.antialiasing ); },
		[&app]( size_t selected ) {
			static constexpr FontAntialiasing values[] = {
				FontAntialiasing::None, FontAntialiasing::Grayscale, FontAntialiasing::Subpixel };
			app.config->font.antialiasing = values[std::min( selected, size_t{ 2 } )];
			app.scene->getResourceScope()->getFontService().setAntialiasing(
				app.config->font.antialiasing );
			app.forEachTerminal(
				[]( UITerminal* terminal ) { terminal->syncFontRenderingConfig(); } );
			app.savePreferences();
		} );

	panel->addCategory( "window.renderer", app.i18n( "window", "Window" ),
						app.i18n( "renderer", "Renderer" ) );
	panel->addBool(
		{ "vsync", "window.renderer", app.i18n( "vsync", "VSync" ),
		  app.i18n( "vsync_desc", "Synchronize rendering with the display. Restart required." ) },
		&app.config->window.vsync, [&app]( bool ) { app.savePreferences(); } );
	const String monitorRefreshRate = app.i18n( "monitor_refresh_rate", "Monitor Refresh Rate" );
	const String unlimitedFrameRate = app.i18n( "unlimited", "Unlimited" );
	panel->addEditableChoice(
		{ "frameRateLimit", "window.renderer", app.i18n( "frame_rate_limit", "Frame Rate Limit" ),
		  app.i18n( "frame_rate_limit_desc", "Limit rendered frames per second, follow the monitor "
											 "refresh rate, or disable the limit." ) },
		{ monitorRefreshRate, unlimitedFrameRate, "30", "60", "75", "120", "144", "165", "240" },
		[&app, monitorRefreshRate, unlimitedFrameRate] {
			return app.config->window.maxFPS ==
						   static_cast<Uint32>( ContextSettings::FrameRateLimitScreenRefreshRate )
					   ? monitorRefreshRate
				   : app.config->window.maxFPS == 0
					   ? unlimitedFrameRate
					   : String( String::toString( app.config->window.maxFPS ) );
		},
		[&app, monitorRefreshRate, unlimitedFrameRate]( const String& selection ) {
			Uint32 value;
			if ( selection == monitorRefreshRate )
				value = ContextSettings::FrameRateLimitScreenRefreshRate;
			else if ( selection == unlimitedFrameRate )
				value = 0;
			else if ( !String::fromString( value, selection ) || value > 1000 )
				return false;
			app.config->window.maxFPS = value;
			app.appWindow->setFrameRateLimit( value );
			app.savePreferences();
			return true;
		} );
	auto rendererVersions = Renderer::getAvailableGraphicsLibraryVersions();
	std::vector<String> rendererNames;
	for ( auto version : rendererVersions )
		rendererNames.emplace_back( Renderer::graphicsLibraryVersionToString( version ) );
	if ( !rendererVersions.empty() )
		panel->addChoice(
			{ "rendererVersion", "window.renderer",
			  app.i18n( "ui_renderer_version", "Renderer Version" ),
			  app.i18n( "ui_renderer_version_desc",
						"Select the graphics API. Restart required." ) },
			rendererNames,
			[&app, rendererVersions] {
				auto found = std::find( rendererVersions.begin(), rendererVersions.end(),
										app.config->window.rendererVersion );
				return found == rendererVersions.end()
						   ? size_t{ 0 }
						   : static_cast<size_t>( found - rendererVersions.begin() );
			},
			[&app, rendererVersions]( size_t selected ) {
				app.config->window.rendererVersion =
					rendererVersions[std::min( selected, rendererVersions.size() - 1 )];
				app.savePreferences();
			} );
	panel->addChoice(
		{ "multisamples", "window.renderer",
		  app.i18n( "ui_multisamples_level", "Multisample Anti-Aliasing Level" ),
		  app.i18n( "ui_multisamples_level_desc",
					"Set renderer multisampling. Restart required." ) },
		{ "0", "2", "4", "8", "16" },
		[&app] {
			static constexpr Uint32 values[] = { 0, 2, 4, 8, 16 };
			auto found = std::find( std::begin( values ), std::end( values ),
									app.config->window.multisamples );
			return found == std::end( values )
					   ? size_t{ 0 }
					   : static_cast<size_t>( found - std::begin( values ) );
		},
		[&app]( size_t selected ) {
			static constexpr Uint32 values[] = { 0, 2, 4, 8, 16 };
			app.config->window.multisamples = values[std::min( selected, size_t{ 4 } )];
			app.savePreferences();
		} );
	panel->addBool(
		{ "benchmarkMode", "window.renderer", app.i18n( "benchmark_mode", "Benchmark Mode" ),
		  app.i18n( "benchmark_mode_desc", "Render continuously to measure performance." ) },
		&app.config->window.benchmarkMode, [&app]( bool value ) {
			app.benchmarkMode = value;
			app.savePreferences();
		} );

	panel->addCategory( "terminal.behavior", app.i18n( "terminal", "Terminal" ),
						app.i18n( "behavior", "Behavior" ) );
	panel->addChoice(
		{ "newTerminalBehavior", "terminal.behavior",
		  app.i18n( "new_terminal_behavior", "New Terminal Behavior" ),
		  app.i18n( "new_terminal_behavior_desc",
					"Choose where new terminal sessions are opened." ) },
		{ app.i18n( "open_in_same_tabbar", "Open in Current Tab Bar" ),
		  app.i18n( "open_in_vertical_split", "Open in New Vertical Split" ),
		  app.i18n( "open_in_horizontal_split", "Open in New Horizontal Split" ) },
		[&app] { return static_cast<size_t>( app.config->terminal.newTerminalBehavior ); },
		[&app]( size_t selected ) {
			app.config->terminal.newTerminalBehavior =
				static_cast<NewTerminalBehavior>( std::min( selected, size_t{ 2 } ) );
			app.savePreferences();
		} );
	panel->addBool(
		{ "terminalExclusiveMode", "terminal.behavior",
		  app.i18n( "enable_exclusive_mode_by_default", "Enable Exclusive Mode by Default" ),
		  app.i18n( "enable_exclusive_mode_by_default_tooltip",
					"Disable global keybindings in newly created terminals." ) },
		&app.config->terminal.exclusiveMode, [&app]( bool value ) {
			app.forEachTerminal(
				[value]( UITerminal* terminal ) { terminal->setExclusiveMode( value ); } );
			app.savePreferences();
		} );
	panel->addBool(
		{ "closeTerminalTabOnExit", "terminal.behavior",
		  app.i18n( "close_terminal_tab_on_exit", "Close Terminal Tab on Exit" ),
		  app.i18n( "close_terminal_tab_on_exit_tooltip", "Close a tab when its process exits." ) },
		&app.config->terminal.closeOnExit, [&app]( bool value ) {
			app.terminalConfig.closeOnExit = value;
			app.forEachTerminal(
				[value]( UITerminal* terminal ) { terminal->getTerm()->setKeepAlive( !value ); } );
			app.savePreferences();
		} );
	panel->addBool( { "warnBeforeClosingTerminal", "terminal.behavior",
					  app.i18n( "warn_before_closing_tab", "Warn Before Closing Tab" ),
					  app.i18n( "warn_before_closing_tab_tooltip",
								"Ask before closing a terminal while a program is running." ) },
					&app.config->window.warnBeforeClose, [&app]( bool value ) {
						app.warnBeforeClose = value;
						app.savePreferences();
					} );
	const std::vector<std::string> knownShells{ "bash", "sh",  "zsh",  "fish", "nu",		"csh",
												"tcsh", "ksh", "dash", "cmd",  "powershell" };
	std::vector<String> installedShells;
	for ( const auto& shell : knownShells ) {
		auto path = Sys::which( shell );
		if ( !path.empty() )
			installedShells.emplace_back( std::move( path ) );
	}
	auto currentShell = app.config->terminal.shell;
	if ( currentShell.empty() ) {
		if ( const char* shell = std::getenv( "SHELL" ); shell )
			currentShell = FileSystem::fileExists( shell ) ? shell : Sys::which( shell );
		if ( currentShell.empty() )
			currentShell = Sys::which(
				Sys::getPlatformType() == Sys::PlatformType::Windows ? "powershell" : "bash" );
	}
	if ( !currentShell.empty() && std::find( installedShells.begin(), installedShells.end(),
											 String( currentShell ) ) == installedShells.end() )
		installedShells.emplace_back( currentShell );
	panel->addEditableChoice(
		{ "terminalShell", "terminal.behavior", app.i18n( "terminal_shell", "Terminal Shell" ),
		  app.i18n( "terminal_shell_desc", "Set the shell executable used by new terminals." ) },
		installedShells, [currentShell] { return String( currentShell ); },
		[&app]( const String& selection ) {
			auto shell = selection.toUtf8();
			if ( Sys::which( shell ).empty() && !FileSystem::fileExists( shell ) )
				return false;
			app.config->terminal.shell = shell;
			app.terminalConfig.program = shell;
			app.savePreferences();
			return true;
		} );
	panel->addText(
		{ "terminalShellArguments", "terminal.behavior",
		  app.i18n( "terminal_shell_arguments", "Terminal Shell Arguments" ),
		  app.i18n( "terminal_shell_arguments_desc",
					"Set shell arguments used by new terminals." ) },
		[&app] { return app.config->terminal.shellArguments; },
		[&app]( const std::string& value ) {
			app.config->terminal.shellArguments = value;
			app.terminalConfig.arguments = String::split( value );
			app.savePreferences();
			return true;
		},
		true );
	panel->addInteger(
		{ "terminalScrollback", "terminal.behavior",
		  app.i18n( "terminal_scrollback", "Terminal Scrollback" ),
		  app.i18n( "configure_terminal_scrollback_desc",
					"Set retained terminal history lines." ) },
		0, std::numeric_limits<int>::max(),
		[&app] {
			return static_cast<int>( std::min<size_t>( app.config->terminal.historySize,
													   std::numeric_limits<int>::max() ) );
		},
		[&app]( int value ) {
			app.config->terminal.historySize = value;
			app.terminalConfig.historySize = value;
			app.savePreferences();
		} );
	panel->addBool(
		{ "terminalFrameBuffer", "terminal.behavior", app.i18n( "framebuffer", "Use Framebuffer" ),
		  app.i18n(
			  "framebuffer_desc",
			  "Render terminals through an FBO instead of the default VBO path. This can improve "
			  "rendering performance by using a different invalidation strategy." ) },
		&app.config->terminal.useFrameBuffer, [&app]( bool value ) {
			app.terminalConfig.useFrameBuffer = value;
			app.savePreferences();
		} );

	panel->addCategory( "terminal.appearance", app.i18n( "terminal", "Terminal" ),
						app.i18n( "appearance", "Appearance" ) );
	std::vector<String> schemeNames{ app.i18n( "default", "Default" ) };
	std::vector<std::string> schemeIds{ "" };
	for ( const auto& [name, scheme] : app.terminalColorSchemes ) {
		schemeNames.emplace_back( name );
		schemeIds.emplace_back( name );
	}
	if ( !schemeIds.empty() )
		panel->addChoice(
			{ "terminalColorScheme", "terminal.appearance",
			  app.i18n( "terminal_color_scheme", "Terminal Color Scheme" ),
			  app.i18n( "terminal_color_scheme_desc", "Choose the colors used by terminals." ) },
			schemeNames,
			[&app, schemeIds] {
				auto found =
					std::find( schemeIds.begin(), schemeIds.end(), app.config->theme.colorScheme );
				return found == schemeIds.end() ? size_t{ 0 }
												: static_cast<size_t>( found - schemeIds.begin() );
			},
			[&app, schemeIds]( size_t selected ) {
				app.config->theme.colorScheme =
					schemeIds[std::min( selected, schemeIds.size() - 1 )];
				if ( app.config->theme.colorScheme.empty() ) {
					app.selectedColorScheme = nullptr;
					auto defaultScheme = TerminalColorScheme::getDefault();
					app.forEachTerminal( [&defaultScheme]( UITerminal* terminal ) {
						terminal->setColorScheme( defaultScheme );
					} );
					app.savePreferences();
					return;
				}
				auto found = app.terminalColorSchemes.find( app.config->theme.colorScheme );
				if ( found != app.terminalColorSchemes.end() ) {
					app.selectedColorScheme = &found->second;
					app.forEachTerminal( [&app]( UITerminal* terminal ) {
						terminal->setColorScheme( *app.selectedColorScheme );
					} );
				}
				app.savePreferences();
			} );
	panel->addChoice(
		{ "terminalScrollbarType", "terminal.appearance",
		  app.i18n( "scrollbar_type", "Scrollbar Type" ),
		  app.i18n( "scrollbar_type_desc",
					"Place the scrollbar over or outside terminal content." ) },
		{ app.i18n( "overlay", "Overlay" ), app.i18n( "outside", "Outside" ) },
		[&app] { return app.config->terminal.scrollBarType == ScrollViewType::Overlay ? 0 : 1; },
		[&app]( size_t selected ) {
			app.config->terminal.scrollBarType =
				selected == 0 ? ScrollViewType::Overlay : ScrollViewType::Outside;
			app.forEachTerminal( [&app]( UITerminal* terminal ) {
				terminal->setScrollViewType( app.config->terminal.scrollBarType );
			} );
			app.savePreferences();
		} );
	panel->addChoice(
		{ "terminalCursorStyle", "terminal.appearance", app.i18n( "cursor_style", "Cursor Style" ),
		  app.i18n( "cursor_style_desc", "Choose the terminal cursor shape and animation." ) },
		{ app.i18n( "blinking_block", "Blinking Block" ),
		  app.i18n( "steady_block", "Steady Block" ),
		  app.i18n( "blink_underline", "Blink Underline" ),
		  app.i18n( "steady_underline", "Steady Underline" ), app.i18n( "blink_bar", "Blink Bar" ),
		  app.i18n( "steady_bar", "Steady Bar" ) },
		[&app] {
			static constexpr TerminalCursorMode modes[] = {
				TerminalCursorMode::BlinkingBlock,	TerminalCursorMode::SteadyBlock,
				TerminalCursorMode::BlinkUnderline, TerminalCursorMode::SteadyUnderline,
				TerminalCursorMode::BlinkBar,		TerminalCursorMode::SteadyBar };
			auto found = std::find( std::begin( modes ), std::end( modes ),
									app.config->terminal.cursorStyle );
			return found == std::end( modes ) ? size_t{ 0 }
											  : static_cast<size_t>( found - std::begin( modes ) );
		},
		[&app]( size_t selected ) {
			static constexpr TerminalCursorMode modes[] = {
				TerminalCursorMode::BlinkingBlock,	TerminalCursorMode::SteadyBlock,
				TerminalCursorMode::BlinkUnderline, TerminalCursorMode::SteadyUnderline,
				TerminalCursorMode::BlinkBar,		TerminalCursorMode::SteadyBar };
			app.config->terminal.cursorStyle = modes[std::min( selected, size_t{ 5 } )];
			app.terminalConfig.cursorStyle = app.config->terminal.cursorStyle;
			app.forEachTerminal( [&app]( UITerminal* terminal ) {
				terminal->getTerm()->setCursorMode( app.config->terminal.cursorStyle );
			} );
			app.savePreferences();
		} );
	panel->addChoice(
		{ "terminalScrollbarMode", "terminal.appearance",
		  app.i18n( "scrollbar_mode", "Scrollbar Mode" ),
		  app.i18n( "scrollbar_mode_desc", "Control when the terminal scrollbar is visible." ) },
		{ app.i18n( "auto", "Auto" ), app.i18n( "always_visible", "Always Visible" ),
		  app.i18n( "always_hidden", "Always Hidden" ) },
		[&app] {
			return app.config->terminal.scrollBarMode == ScrollBarMode::Auto	   ? 0
				   : app.config->terminal.scrollBarMode == ScrollBarMode::AlwaysOn ? 1
																				   : 2;
		},
		[&app]( size_t selected ) {
			app.config->terminal.scrollBarMode = selected == 0	 ? ScrollBarMode::Auto
												 : selected == 1 ? ScrollBarMode::AlwaysOn
																 : ScrollBarMode::AlwaysOff;
			app.forEachTerminal( [&app]( UITerminal* terminal ) {
				terminal->setVerticalScrollMode( app.config->terminal.scrollBarMode );
			} );
			app.savePreferences();
		} );
	panel->addBool( { "alwaysShowTabBar", "terminal.appearance",
					  app.i18n( "always_show_tab_bar", "Always Show Tab Bar" ),
					  app.i18n( "always_show_tab_bar_desc",
								"Show the tab bar even when it contains one tab." ) },
					&app.config->window.alwaysShowTabBar, [&app]( bool value ) {
						app.tabSplitter->setHideTabBarOnSingleTab( !value );
						app.savePreferences();
					} );

	panel->build();
	settingsWindow->setKeyBindingCommand( "closeWindow",
										  [settingsWindow] { settingsWindow->closeWindow(); } );
	settingsWindow->getKeyBindings().addKeybind( { KEY_ESCAPE }, "closeWindow" );
	settingsWindow->on( Event::OnWindowClose, [&app]( const Event* ) {
		app.savePreferences();
		if ( app.tabSplitter && app.tabSplitter->getCurWidget() )
			app.tabSplitter->getCurWidget()->setFocus();
	} );
	settingsWindow->center();
	settingsWindow->showWhenReady();
	panel->focusSearch();
	return settingsWindow;
}

} // namespace eterm
