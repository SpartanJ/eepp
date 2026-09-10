#include "appconfig.hpp"
#include <eepp/system/filesystem.hpp>
#include <eepp/window/window.hpp>

using namespace EE::Graphics;
using namespace EE::System;
using namespace EE::UI;
using namespace eterm::Terminal;
using EE::eemax;

namespace eterm {

AppConfig::AppConfig( std::string configPath ) : mConfigPath( std::move( configPath ) ) {
	FileSystem::dirAddSlashAtEnd( mConfigPath );
	mIni.path( mConfigPath + "config.cfg" );
	mState.path( mConfigPath + "state.cfg" );
}

void AppConfig::load() {
	mIni.loadFromFile( mIni.path() );
	mState.loadFromFile( mState.path() );
	terminal.shell = mIni.getValue( "terminal", "shell", terminal.shell );
	terminal.shellArguments =
		mIni.getValue( "terminal", "shell_arguments", terminal.shellArguments );
	terminal.workingDirectory =
		mIni.getValue( "terminal", "working_directory", terminal.workingDirectory );
	terminal.executeInShell =
		mIni.getValue( "terminal", "execute_in_shell", terminal.executeInShell );
	terminal.historySize = mIni.getValueU( "terminal", "scrollback", terminal.historySize );
	terminal.cursorStyle = TerminalCursorHelper::modeFromString( mIni.getValue(
		"terminal", "cursor_style", TerminalCursorHelper::modeToString( terminal.cursorStyle ) ) );
	terminal.useFrameBuffer = mIni.getValueB( "terminal", "framebuffer", terminal.useFrameBuffer );
	terminal.closeOnExit = mIni.getValueB( "terminal", "close_on_exit", terminal.closeOnExit );
	terminal.initialTabs = mIni.getValueU( "terminal", "initial_tabs", terminal.initialTabs );
	terminal.exclusiveMode = mIni.getValueB( "terminal", "exclusive_mode", terminal.exclusiveMode );
	const auto newTerminalBehavior =
		mIni.getValue( "terminal", "new_terminal_behavior", "current" );
	terminal.newTerminalBehavior =
		newTerminalBehavior == "vertical"	  ? NewTerminalBehavior::VerticalSplit
		: newTerminalBehavior == "horizontal" ? NewTerminalBehavior::HorizontalSplit
											  : NewTerminalBehavior::CurrentTabBar;
	terminal.scrollBarType = mIni.getValue( "terminal", "scrollbar_type", "overlay" ) == "outside"
								 ? ScrollViewType::Outside
								 : ScrollViewType::Overlay;
	const auto scrollBarMode = mIni.getValue( "terminal", "scrollbar_mode", "auto" );
	terminal.scrollBarMode = scrollBarMode == "always_on"	 ? ScrollBarMode::AlwaysOn
							 : scrollBarMode == "always_off" ? ScrollBarMode::AlwaysOff
															 : ScrollBarMode::Auto;
	font.path = mIni.getValue( "font", "path", font.path );
	font.fallbackPath = mIni.getValue( "font", "fallback_path", font.fallbackPath );
	font.size = mIni.getValueF( "font", "size", font.size );
	font.hinting = Font::fontHintingFromString(
		mIni.getValue( "font", "hinting", Font::fontHintingToString( font.hinting ) ) );
	font.antialiasing = Font::fontAntialiasingFromString( mIni.getValue(
		"font", "antialiasing", Font::fontAntialiasingToString( font.antialiasing ) ) );
	font.uiPath = mIni.getValue( "font", "ui_path", font.uiPath );
	font.uiSize = mIni.getValueF( "font", "ui_size", font.uiSize );

	window.pixelDensity = mIni.getValueF( "window", "pixel_density", window.pixelDensity );
	window.maxFPS = mIni.getValueU( "window", "frame_rate_limit", window.maxFPS );
	window.vsync = mIni.getValueB( "window", "vsync", window.vsync );
	window.benchmarkMode = mIni.getValueB( "window", "benchmark_mode", window.benchmarkMode );
	window.warnBeforeClose =
		mIni.getValueB( "window", "warn_before_closing", window.warnBeforeClose );
	window.alwaysShowTabBar =
		mIni.getValueB( "window", "always_show_tab_bar", window.alwaysShowTabBar );
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
	mIni.setValue( "terminal", "working_directory", terminal.workingDirectory );
	mIni.setValue( "terminal", "execute_in_shell", terminal.executeInShell );
	mIni.setValueU( "terminal", "scrollback", terminal.historySize );
	mIni.setValue( "terminal", "cursor_style",
				   TerminalCursorHelper::modeToString( terminal.cursorStyle ) );
	mIni.setValueB( "terminal", "framebuffer", terminal.useFrameBuffer );
	mIni.setValueB( "terminal", "close_on_exit", terminal.closeOnExit );
	mIni.setValueU( "terminal", "initial_tabs", terminal.initialTabs );
	mIni.setValueB( "terminal", "exclusive_mode", terminal.exclusiveMode );
	mIni.setValue( "terminal", "new_terminal_behavior",
				   terminal.newTerminalBehavior == NewTerminalBehavior::VerticalSplit ? "vertical"
				   : terminal.newTerminalBehavior == NewTerminalBehavior::HorizontalSplit
					   ? "horizontal"
					   : "current" );
	mIni.setValue( "terminal", "scrollbar_type",
				   terminal.scrollBarType == ScrollViewType::Overlay ? "overlay" : "outside" );
	mIni.setValue( "terminal", "scrollbar_mode",
				   terminal.scrollBarMode == ScrollBarMode::AlwaysOn	? "always_on"
				   : terminal.scrollBarMode == ScrollBarMode::AlwaysOff ? "always_off"
																		: "auto" );
	mIni.setValue( "font", "path", font.path );
	mIni.setValue( "font", "fallback_path", font.fallbackPath );
	mIni.setValueF( "font", "size", font.size );
	mIni.setValue( "font", "hinting", Font::fontHintingToString( font.hinting ) );
	mIni.setValue( "font", "antialiasing", Font::fontAntialiasingToString( font.antialiasing ) );
	mIni.setValue( "font", "ui_path", font.uiPath );
	mIni.setValueF( "font", "ui_size", font.uiSize );
	mIni.setValueF( "window", "pixel_density", window.pixelDensity );
	mIni.setValueU( "window", "frame_rate_limit", window.maxFPS );
	mIni.setValueB( "window", "vsync", window.vsync );
	mIni.setValueB( "window", "benchmark_mode", window.benchmarkMode );
	mIni.setValueB( "window", "warn_before_closing", window.warnBeforeClose );
	mIni.setValueB( "window", "always_show_tab_bar", window.alwaysShowTabBar );
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

} // namespace eterm
