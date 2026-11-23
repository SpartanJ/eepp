#include "appconfig.hpp"
#include <algorithm>
#include <eepp/system/filesystem.hpp>
#include <eepp/window/platformhelper.hpp>
#include <eepp/window/window.hpp>
#include <nlohmann/json.hpp>

using namespace EE::Graphics;
using namespace EE::System;
using namespace EE::UI;
using EE::Window::PlatformHelper;
using namespace eterm::Terminal;
using EE::eemax;

namespace eterm {

AppConfig::AppConfig( std::string configPath ) : mConfigPath( std::move( configPath ) ) {
	FileSystem::dirAddSlashAtEnd( mConfigPath );
	mIni.path( mConfigPath + "config.cfg" );
	mState.path( mConfigPath + "state.cfg" );
}

void AppConfig::load() {
	const bool newProfile =
		!FileSystem::fileExists( mIni.path() ) && !FileSystem::fileExists( mState.path() );
	mIni.loadFromFile( mIni.path() );
	mState.loadFromFile( mState.path() );
	terminal.shell = mIni.getValue( "terminal", "shell", terminal.shell );
	terminal.shellArguments =
		mIni.getValue( "terminal", "shell_arguments", terminal.shellArguments );
	// The legacy working_directory key also captured one-off command-line paths.
	terminal.defaultWorkingDirectory =
		mIni.getValue( "terminal", "default_working_directory", terminal.defaultWorkingDirectory );
	terminal.useCustomWorkingDirectory = mIni.getValueB(
		"terminal", "use_custom_working_directory", !terminal.defaultWorkingDirectory.empty() );
	terminal.executeInShell =
		mIni.getValue( "terminal", "execute_in_shell", terminal.executeInShell );
	terminal.load( mIni );
	terminal.useFrameBuffer = mIni.getValueB( "terminal", "framebuffer", terminal.useFrameBuffer );
	terminal.closeOnExit = mIni.getValueB( "terminal", "close_on_exit", terminal.closeOnExit );
	terminal.initialTabs = mIni.getValueU( "terminal", "initial_tabs", terminal.initialTabs );
	terminal.exclusiveMode = mIni.getValueB( "terminal", "exclusive_mode", terminal.exclusiveMode );
	terminal.inheritWorkingDirectory =
		mIni.getValueB( "terminal", "inherit_working_directory", terminal.inheritWorkingDirectory );
	terminal.programStatus.load( mIni );
	terminal.restoreLastSession =
		mIni.getValueB( "terminal", "restore_last_session", terminal.restoreLastSession );
	terminal.recoverHarnessSessions =
		mIni.getValueB( "terminal", "recover_harness_sessions", terminal.recoverHarnessSessions );
	ui.smoothScroll = mIni.getValueB(
		"ui", "smooth_scroll", mIni.getValueB( "terminal", "smooth_scroll", ui.smoothScroll ) );
	const auto newTerminalBehavior =
		mIni.getValue( "terminal", "new_terminal_behavior", "current" );
	terminal.newTerminalBehavior =
		newTerminalBehavior == "vertical"	  ? NewTerminalBehavior::VerticalSplit
		: newTerminalBehavior == "horizontal" ? NewTerminalBehavior::HorizontalSplit
											  : NewTerminalBehavior::CurrentTabBar;
	font.path = mIni.getValue( "font", "path", font.path );
	font.fallbackPath = mIni.getValue( "font", "fallback_path", font.fallbackPath );
	font.size = mIni.getValueF( "font", "size", font.size );
	font.hinting = Font::fontHintingFromString(
		mIni.getValue( "font", "hinting", Font::fontHintingToString( font.hinting ) ) );
	font.antialiasing = Font::fontAntialiasingFromString( mIni.getValue(
		"font", "antialiasing", Font::fontAntialiasingToString( font.antialiasing ) ) );
	font.uiPath = mIni.getValue( "font", "ui_path", font.uiPath );
	font.uiSize = mIni.keyValueExists( "font", "ui_size" )
					  ? mIni.getValueF( "font", "ui_size", font.uiSize )
					  : PlatformHelper::getSystemUIFontSize( font.uiSize );

	window.pixelDensity = mIni.getValueF( "window", "pixel_density", window.pixelDensity );
	window.pixelDensityMode = newProfile ? EE::Window::Window::getNewProfilePixelDensityMode()
										 : EE::Window::Window::pixelDensityModeFromString(
											   mIni.getValue( "window", "pixel_density_mode" ) );
	window.displayDensityOverrides = EE::Window::Window::loadDisplayDensityOverrides( mIni );
	window.maxFPS = mIni.getValueU( "window", "frame_rate_limit", window.maxFPS );
	window.vsync = mIni.getValueB( "window", "vsync", window.vsync );
	window.exitOnClose = mIni.getValueB( "window", "exit_on_close", window.exitOnClose );
	window.benchmarkMode = mIni.getValueB( "window", "benchmark_mode", window.benchmarkMode );
	window.warnBeforeClose =
		mIni.getValueB( "window", "warn_before_closing", window.warnBeforeClose );
	window.alwaysShowTabBar =
		mIni.getValueB( "window", "always_show_tab_bar", window.alwaysShowTabBar );
	window.showTabBarWhenSplit =
		mIni.getValueB( "window", "show_tab_bar_when_split", window.showTabBarWhenSplit );
	window.tabPosition = UITabWidget::tabPositionFromString( mIni.getValue(
		"window", "tab_position", UITabWidget::tabPositionToString( window.tabPosition ) ) );
	window.rendererVersion = Renderer::glVersionFromString(
		mIni.getValue( "window", "renderer_version",
					   Renderer::graphicsLibraryVersionToString( window.rendererVersion ) ) );
	window.multisamples = mIni.getValueU( "window", "multisamples", window.multisamples );
	theme.colorScheme = mIni.getValue( "theme", "color_scheme", theme.colorScheme );
	theme.uiColorScheme = ColorSchemePreferences::fromStringExt( mIni.getValue(
		"theme", "ui_color_scheme", ColorSchemePreferences::toString( theme.uiColorScheme ) ) );

	windowState.size.setWidth( mState.getValueI( "window", "width", windowState.size.getWidth() ) );
	windowState.size.setHeight(
		mState.getValueI( "window", "height", windowState.size.getHeight() ) );
	windowState.position.x = mState.getValueI( "window", "x", windowState.position.x );
	windowState.position.y = mState.getValueI( "window", "y", windowState.position.y );
	windowState.displayIndex =
		mState.getValueI( "window", "display_index", windowState.displayIndex );
	windowState.maximized = mState.getValueB( "window", "maximized", windowState.maximized );
	windowState.tabBarWidth =
		mState.getValueF( "window", "tab_bar_width", windowState.tabBarWidth );
}

bool AppConfig::save() {
	if ( !FileSystem::isDirectory( mConfigPath ) && !FileSystem::makeDir( mConfigPath, true ) )
		return false;
	return savePreferences() && saveWindowState();
}

bool AppConfig::savePreferences() {
	if ( !FileSystem::isDirectory( mConfigPath ) && !FileSystem::makeDir( mConfigPath, true ) )
		return false;
	mIni.deleteValue( "theme", "language" );
	mIni.deleteValue( "window", "max_fps" );
	mIni.setValue( "terminal", "shell", terminal.shell );
	mIni.setValue( "terminal", "shell_arguments", terminal.shellArguments );
	mIni.deleteValue( "terminal", "working_directory" );
	mIni.setValue( "terminal", "default_working_directory", terminal.defaultWorkingDirectory );
	mIni.setValueB( "terminal", "use_custom_working_directory",
					terminal.useCustomWorkingDirectory );
	mIni.setValue( "terminal", "execute_in_shell", terminal.executeInShell );
	terminal.save( mIni );
	mIni.setValueB( "terminal", "framebuffer", terminal.useFrameBuffer );
	mIni.setValueB( "terminal", "close_on_exit", terminal.closeOnExit );
	mIni.setValueU( "terminal", "initial_tabs", terminal.initialTabs );
	mIni.setValueB( "terminal", "exclusive_mode", terminal.exclusiveMode );
	mIni.setValueB( "terminal", "inherit_working_directory", terminal.inheritWorkingDirectory );
	terminal.programStatus.save( mIni );
	mIni.setValueB( "terminal", "restore_last_session", terminal.restoreLastSession );
	mIni.setValueB( "terminal", "recover_harness_sessions", terminal.recoverHarnessSessions );
	mIni.setValueB( "ui", "smooth_scroll", ui.smoothScroll );
	mIni.deleteValue( "terminal", "smooth_scroll" );
	mIni.setValue( "terminal", "new_terminal_behavior",
				   terminal.newTerminalBehavior == NewTerminalBehavior::VerticalSplit ? "vertical"
				   : terminal.newTerminalBehavior == NewTerminalBehavior::HorizontalSplit
					   ? "horizontal"
					   : "current" );
	mIni.setValue( "font", "path", font.path );
	mIni.setValue( "font", "fallback_path", font.fallbackPath );
	mIni.setValueF( "font", "size", font.size );
	mIni.setValue( "font", "hinting", Font::fontHintingToString( font.hinting ) );
	mIni.setValue( "font", "antialiasing", Font::fontAntialiasingToString( font.antialiasing ) );
	mIni.setValue( "font", "ui_path", font.uiPath );
	mIni.setValueF( "font", "ui_size", font.uiSize );
	mIni.setValueF( "window", "pixel_density", window.pixelDensity );
	mIni.setValue( "window", "pixel_density_mode",
				   EE::Window::Window::pixelDensityModeToString( window.pixelDensityMode ) );
	EE::Window::Window::saveDisplayDensityOverrides( mIni, window.displayDensityOverrides );
	mIni.setValueU( "window", "frame_rate_limit", window.maxFPS );
	mIni.setValueB( "window", "vsync", window.vsync );
	mIni.setValueB( "window", "exit_on_close", window.exitOnClose );
	mIni.setValueB( "window", "benchmark_mode", window.benchmarkMode );
	mIni.setValueB( "window", "warn_before_closing", window.warnBeforeClose );
	mIni.setValueB( "window", "always_show_tab_bar", window.alwaysShowTabBar );
	mIni.setValue( "window", "tab_position",
				   UITabWidget::tabPositionToString( window.tabPosition ) );
	mIni.setValueB( "window", "show_tab_bar_when_split", window.showTabBarWhenSplit );
	mIni.setValue( "window", "renderer_version",
				   Renderer::graphicsLibraryVersionToString( window.rendererVersion ) );
	mIni.setValueU( "window", "multisamples", window.multisamples );
	mIni.setValue( "theme", "color_scheme", theme.colorScheme );
	mIni.setValue( "theme", "ui_color_scheme",
				   ColorSchemePreferences::toString( theme.uiColorScheme ) );
	return mIni.writeFile();
}

bool AppConfig::saveWindowState() {
	if ( !FileSystem::isDirectory( mConfigPath ) && !FileSystem::makeDir( mConfigPath, true ) )
		return false;
	mState.setValueI( "window", "width", windowState.size.getWidth() );
	mState.setValueI( "window", "height", windowState.size.getHeight() );
	mState.setValueI( "window", "x", windowState.position.x );
	mState.setValueI( "window", "y", windowState.position.y );
	mState.setValueI( "window", "display_index", windowState.displayIndex );
	mState.setValueB( "window", "maximized", windowState.maximized );
	mState.setValueF( "window", "tab_bar_width", windowState.tabBarWidth );
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

nlohmann::json AppConfig::takeSession() {
	const std::string path = sessionPath();
	std::string data;
	if ( !FileSystem::fileGet( path, data ) )
		return {};
	FileSystem::fileRemove( path );
	auto session = nlohmann::json::parse( data, nullptr, false );
	return session.is_discarded() ? nlohmann::json{} : session;
}

bool AppConfig::saveSession( const nlohmann::json& session ) {
	if ( !FileSystem::isDirectory( mConfigPath ) && !FileSystem::makeDir( mConfigPath, true ) )
		return false;
	// Titles can come from programs, so an invalid sequence must not fail the whole session.
	return FileSystem::fileWriteAtomic(
		sessionPath(), session.dump( -1, ' ', false, nlohmann::json::error_handler_t::replace ),
		true );
}

} // namespace eterm
