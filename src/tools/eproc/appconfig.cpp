#include "appconfig.hpp"

#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/ui/colorschemepreferences.hpp>

#include <algorithm>
#include <cmath>

using namespace EE;
using namespace EE::System;

namespace eproc {

AppConfig::AppConfig( std::string configPath ) : mConfigPath( std::move( configPath ) ) {
	FileSystem::dirAddSlashAtEnd( mConfigPath );
	mState.path( mConfigPath + "state.cfg" );
}

void AppConfig::load() {
	mState.loadFromFile( mState.path() );

	windowState.size.setWidth( mState.getValueI( "window", "width", windowState.size.getWidth() ) );
	windowState.size.setHeight(
		mState.getValueI( "window", "height", windowState.size.getHeight() ) );
	windowState.position.x = mState.getValueI( "window", "x", windowState.position.x );
	windowState.position.y = mState.getValueI( "window", "y", windowState.position.y );
	windowState.displayIndex =
		mState.getValueI( "window", "display_index", windowState.displayIndex );
	windowState.maximized = mState.getValueB( "window", "maximized", windowState.maximized );
	processTableState = mState.getValue( "process_table", "state", processTableState );
	divideCpuUsage =
		mState.getValueB( "process_table", "divide_cpu_usage",
						  mState.getValueB( "window", "divide_cpu_usage", divideCpuUsage ) );
	performancePerCore = mState.getValueB( "performance", "cpu_per_core", performancePerCore );
	treeView = mState.getValueB( "process_table", "tree_view", treeView );
	filterMode = mState.getValueI( "process_table", "filter_mode", treeView ? 1 : filterMode );
	refreshIntervalMs =
		std::clamp( mState.getValueI( "monitoring", "refresh_interval_ms", refreshIntervalMs ),
					MinRefreshIntervalMs, MaxRefreshIntervalMs );
	const double savedDensity = mState.getValueF( "appearance", "pixel_density", pixelDensity );
	if ( std::isfinite( savedDensity ) && savedDensity >= 0 && savedDensity <= 6 )
		pixelDensity = static_cast<Float>( savedDensity );
	const int savedHinting = mState.getValueI( "appearance", "font_hinting", 2 );
	if ( savedHinting >= 0 && savedHinting <= 2 )
		fontHinting = static_cast<FontHinting>( savedHinting );
	const int savedAntialiasing = mState.getValueI( "appearance", "font_antialiasing", 1 );
	if ( savedAntialiasing >= 0 && savedAntialiasing <= 2 )
		fontAntialiasing = static_cast<FontAntialiasing>( savedAntialiasing );
	const auto savedScheme = mState.getValue( "appearance", "color_scheme", "system" );
	if ( savedScheme == "system" || savedScheme == "light" || savedScheme == "dark" )
		colorScheme = EE::UI::ColorSchemePreferences::fromStringExt( savedScheme );
	vsync = mState.getValueB( "renderer", "vsync", vsync );
	frameRateLimit = static_cast<Uint32>( std::clamp(
		mState.getValueI( "renderer", "frame_rate_limit", static_cast<int>( frameRateLimit ) ), 0,
		ContextSettings::FrameRateLimitScreenRefreshRate ) );
	const auto savedRenderer = mState.getValue( "renderer", "version", "" );
	for ( auto version : Renderer::getAvailableGraphicsLibraryVersions() ) {
		if ( savedRenderer == Renderer::graphicsLibraryVersionToString( version ) ) {
			rendererVersion = version;
			break;
		}
	}
	const int savedMultisamples = mState.getValueI( "renderer", "multisamples", multisamples );
	if ( savedMultisamples == 0 || savedMultisamples == 2 || savedMultisamples == 4 ||
		 savedMultisamples == 8 || savedMultisamples == 16 )
		multisamples = static_cast<Uint32>( savedMultisamples );
}

bool AppConfig::save() {
	if ( !FileSystem::isDirectory( mConfigPath ) && !FileSystem::makeDir( mConfigPath, true ) )
		return false;

	mState.setValueI( "window", "width", windowState.size.getWidth() );
	mState.setValueI( "window", "height", windowState.size.getHeight() );
	mState.setValueI( "window", "x", windowState.position.x );
	mState.setValueI( "window", "y", windowState.position.y );
	mState.setValueI( "window", "display_index", windowState.displayIndex );
	mState.setValueB( "window", "maximized", windowState.maximized );
	mState.setValue( "process_table", "state", processTableState );
	mState.setValueB( "process_table", "divide_cpu_usage", divideCpuUsage );
	mState.setValueB( "performance", "cpu_per_core", performancePerCore );
	mState.setValueB( "process_table", "tree_view", treeView );
	mState.setValueI( "process_table", "filter_mode", filterMode );
	mState.setValueI( "monitoring", "refresh_interval_ms", refreshIntervalMs );
	mState.setValueF( "appearance", "pixel_density", pixelDensity );
	mState.setValueI( "appearance", "font_hinting", static_cast<int>( fontHinting ) );
	mState.setValueI( "appearance", "font_antialiasing", static_cast<int>( fontAntialiasing ) );
	mState.setValue( "appearance", "color_scheme",
					 EE::UI::ColorSchemePreferences::toString( colorScheme ) );
	mState.setValueB( "renderer", "vsync", vsync );
	mState.setValueI( "renderer", "frame_rate_limit", frameRateLimit );
	mState.setValue( "renderer", "version",
					 Renderer::graphicsLibraryVersionToString( rendererVersion ) );
	mState.setValueI( "renderer", "multisamples", multisamples );
	return mState.writeFile();
}

void AppConfig::captureWindowState( EE::Window::Window* window ) {
	if ( !window )
		return;

	windowState.size = Sys::getPlatformType() == Sys::PlatformType::macOS
						   ? window->getSizeInScreenCoordinates()
						   : window->getLastWindowedSizeInScreenCoordinates();
	windowState.position = window->getPosition();
	windowState.position.x = eemax( 0, windowState.position.x );
	windowState.position.y = eemax( 0, windowState.position.y );
	windowState.displayIndex = window->getCurrentDisplayIndex();
	windowState.maximized = window->isMaximized();
}

} // namespace eproc
