#include "settingspanel.hpp"
#include "eproc.hpp"

#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/ui/tools/uisettingspanel.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiwindow.hpp>

#include <algorithm>
#include <iterator>

using namespace EE::UI::Tools;

namespace eproc {

UIWindow* SettingsPanel::create( App& app ) {
	UIWindow::StyleConfig windowStyle{ UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON |
									   UI_WIN_MODAL };
	auto* window = UIWindow::NewOpt( UIWindow::SIMPLE_LAYOUT, windowStyle );
	window->setId( "settings_panel" );
	auto* scene = app.mApp->getUI();
	window->setTitle( scene->i18n( "settings", "Settings" ) );
	const auto sceneSize = scene->getPixelsSize();
	window->setPixelsSize( { eeclamp( sceneSize.getWidth() * 0.82f, 720.f, 1200.f ),
							 eeclamp( sceneSize.getHeight() * 0.82f, 520.f, 850.f ) } );
	window->setMinWindowSize( 640, 440 );
	auto* panel = UISettingsPanel::New( window->getContainer() );
	panel->setId( "settings_panel_content" );
	panel->setSearchResultsText( scene->i18n( "search_results", "Search Results" ) );

	panel->addCategory( "monitoring.refresh", scene->i18n( "monitoring", "Monitoring" ),
						scene->i18n( "refresh", "Refresh" ) );
	panel->addInteger(
		{ "refreshInterval", "monitoring.refresh",
		  scene->i18n( "eproc_refresh_interval", "Refresh Interval (ms)" ),
		  scene->i18n(
			  "eproc_refresh_interval_desc",
			  "How often process data and performance charts are sampled (250–60000 ms)." ) },
		AppConfig::MinRefreshIntervalMs, AppConfig::MaxRefreshIntervalMs,
		[&app] { return static_cast<int>( app.mUpdateIntervalMs ); },
		[&app]( int value ) {
			app.mUpdateIntervalMs = std::clamp( value, AppConfig::MinRefreshIntervalMs,
												AppConfig::MaxRefreshIntervalMs );
			app.mConfig->refreshIntervalMs = app.mUpdateIntervalMs;
			// Start the new cadence from this edit; the worker and publish poll remain independent.
			app.mDispatchClock.getElapsedTimeAndReset();
			app.savePreferences();
		} );

	panel->addCategory( "appearance.theme", scene->i18n( "appearance", "Appearance" ),
						scene->i18n( "theme_and_language", "Theme & Language" ) );
	panel->addChoice(
		{ "uiColorScheme", "appearance.theme",
		  scene->i18n( "ui_prefes_color_scheme", "UI Prefers Color Scheme" ),
		  scene->i18n(
			  "ui_prefers_color_scheme_desc",
			  "Choose whether the interface follows the system, light, or dark appearance." ) },
		{ scene->i18n( "system", "System" ), scene->i18n( "light", "Light" ),
		  scene->i18n( "dark", "Dark" ) },
		[&app] {
			return app.mColorScheme == ColorSchemeExtPreference::System	 ? size_t{ 0 }
				   : app.mColorScheme == ColorSchemeExtPreference::Light ? size_t{ 1 }
																		 : size_t{ 2 };
		},
		[&app, scene]( size_t selected ) {
			static constexpr ColorSchemeExtPreference values[] = { ColorSchemeExtPreference::System,
																   ColorSchemeExtPreference::Light,
																   ColorSchemeExtPreference::Dark };
			app.mColorScheme = values[std::min( selected, size_t{ 2 } )];
			app.mConfig->colorScheme = app.mColorScheme;
			scene->setColorSchemePreference( app.mColorScheme );
			scene->updateWindowTitleBarColor();
			app.savePreferences();
		} );

	panel->addCategory( "appearance.fonts", scene->i18n( "appearance", "Appearance" ),
						scene->i18n( "fonts_and_scale", "Fonts & Scale" ) );
	panel->addFloat(
		{ "uiScaleFactor", "appearance.fonts", scene->i18n( "ui_scale_factor", "UI Scale Factor" ),
		  scene->i18n( "ui_scale_factor_desc",
					   "Scale the complete user interface. Restart required." ) },
		1, 6, 0.1,
		[&app] {
			return app.mPixelDensity ? static_cast<double>( *app.mPixelDensity )
									 : static_cast<double>( PixelDensity::getPixelDensity() );
		},
		[&app]( double value ) {
			app.mPixelDensity = static_cast<Float>( value );
			app.mConfig->pixelDensity = *app.mPixelDensity;
			app.savePreferences();
		} );
	panel->addChoice(
		{ "fontHinting", "appearance.fonts", scene->i18n( "ui_font_hint", "Font Hinting" ),
		  scene->i18n( "ui_font_hint_desc", "Control glyph alignment to the pixel grid." ) },
		{ scene->i18n( "none", "None" ), scene->i18n( "slight", "Slight" ),
		  scene->i18n( "full", "Full" ) },
		[&app] { return static_cast<size_t>( app.mFontHinting ); },
		[&app, scene]( size_t selected ) {
			static constexpr FontHinting values[] = { FontHinting::None, FontHinting::Slight,
													  FontHinting::Full };
			app.mFontHinting = values[std::min( selected, size_t{ 2 } )];
			app.mConfig->fontHinting = app.mFontHinting;
			scene->getResourceScope()->getFontService().setHinting( app.mFontHinting );
			app.savePreferences();
		} );
	panel->addChoice(
		{ "fontAntialiasing", "appearance.fonts",
		  scene->i18n( "ui_font_antialiasing", "Font Anti-Aliasing" ),
		  scene->i18n( "ui_font_antialiasing_desc", "Choose how glyph edges are smoothed." ) },
		{ scene->i18n( "none", "None" ), scene->i18n( "grayscale", "Grayscale" ),
		  scene->i18n( "subpixel", "Subpixel" ) },
		[&app] { return static_cast<size_t>( app.mFontAntialiasing ); },
		[&app, scene]( size_t selected ) {
			static constexpr FontAntialiasing values[] = {
				FontAntialiasing::None, FontAntialiasing::Grayscale, FontAntialiasing::Subpixel };
			app.mFontAntialiasing = values[std::min( selected, size_t{ 2 } )];
			app.mConfig->fontAntialiasing = app.mFontAntialiasing;
			scene->getResourceScope()->getFontService().setAntialiasing( app.mFontAntialiasing );
			app.savePreferences();
		} );

	panel->addCategory( "window.renderer", scene->i18n( "window", "Window" ),
						scene->i18n( "renderer", "Renderer" ) );
	panel->addBool( { "vsync", "window.renderer", scene->i18n( "vsync", "VSync" ),
					  scene->i18n( "vsync_desc",
								   "Synchronize rendering with the display. Restart required." ) },
					&app.mConfig->vsync, [&app]( bool ) { app.savePreferences(); } );
	const String monitorRefreshRate = scene->i18n( "monitor_refresh_rate", "Monitor Refresh Rate" );
	const String unlimitedFrameRate = scene->i18n( "unlimited", "Unlimited" );
	panel->addEditableChoice(
		{ "frameRateLimit", "window.renderer",
		  scene->i18n( "frame_rate_limit", "Frame Rate Limit" ),
		  scene->i18n( "frame_rate_limit_desc", "Limit rendered frames per second, follow the "
												"monitor refresh rate, or disable the limit." ) },
		{ monitorRefreshRate, unlimitedFrameRate, "30", "60", "75", "120", "144", "165", "240" },
		[&app, monitorRefreshRate, unlimitedFrameRate] {
			return app.mConfig->frameRateLimit ==
						   static_cast<Uint32>( ContextSettings::FrameRateLimitScreenRefreshRate )
					   ? monitorRefreshRate
				   : app.mConfig->frameRateLimit == 0
					   ? unlimitedFrameRate
					   : String( String::toString( app.mConfig->frameRateLimit ) );
		},
		[&app, monitorRefreshRate, unlimitedFrameRate]( const String& selection ) {
			Uint32 value;
			if ( selection == monitorRefreshRate )
				value = ContextSettings::FrameRateLimitScreenRefreshRate;
			else if ( selection == unlimitedFrameRate )
				value = 0;
			else if ( !String::fromString( value, selection ) || value > 1000 )
				return false;
			app.mConfig->frameRateLimit = value;
			app.mApp->getWindow()->setFrameRateLimit( value );
			app.savePreferences();
			return true;
		} );
	auto rendererVersions = Renderer::getAvailableGraphicsLibraryVersions();
	std::vector<String> rendererNames;
	for ( auto version : rendererVersions )
		rendererNames.emplace_back( Renderer::graphicsLibraryVersionToString( version ) );
	if ( !rendererVersions.empty() ) {
		panel->addChoice(
			{ "rendererVersion", "window.renderer",
			  scene->i18n( "ui_renderer_version", "Renderer Version" ),
			  scene->i18n( "ui_renderer_version_desc",
						   "Select the graphics API. Restart required." ) },
			rendererNames,
			[&app, rendererVersions] {
				auto found = std::find( rendererVersions.begin(), rendererVersions.end(),
										app.mConfig->rendererVersion );
				return found == rendererVersions.end()
						   ? size_t{ 0 }
						   : static_cast<size_t>( found - rendererVersions.begin() );
			},
			[&app, rendererVersions]( size_t selected ) {
				app.mConfig->rendererVersion =
					rendererVersions[std::min( selected, rendererVersions.size() - 1 )];
				app.savePreferences();
			} );
	}
	panel->addChoice(
		{ "multisamples", "window.renderer",
		  scene->i18n( "ui_multisamples_level", "Multisample Anti-Aliasing Level" ),
		  scene->i18n( "ui_multisamples_level_desc",
					   "Set renderer multisampling. Restart required." ) },
		{ "0", "2", "4", "8", "16" },
		[&app] {
			static constexpr Uint32 values[] = { 0, 2, 4, 8, 16 };
			auto found =
				std::find( std::begin( values ), std::end( values ), app.mConfig->multisamples );
			return found == std::end( values )
					   ? size_t{ 0 }
					   : static_cast<size_t>( found - std::begin( values ) );
		},
		[&app]( size_t selected ) {
			static constexpr Uint32 values[] = { 0, 2, 4, 8, 16 };
			app.mConfig->multisamples = values[std::min( selected, size_t{ 4 } )];
			app.savePreferences();
		} );

	panel->build();
	window->on( Event::OnWindowReady, [panel]( const Event* ) {
		panel->runOnMainThread( [panel] { panel->focusSearch(); } );
	} );
	window->setKeyBindingCommand( "closeWindow", [window] { window->closeWindow(); } );
	window->getKeyBindings().addKeybind( { KEY_ESCAPE }, "closeWindow" );
	window->center();
	window->showWhenReady();
	panel->focusSearch();
	return window;
}

} // namespace eproc
