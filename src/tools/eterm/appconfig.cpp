#include "appconfig.hpp"
#include <algorithm>
#include <eepp/system/filelock.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/window/platformhelper.hpp>
#include <eepp/window/runtime.hpp>
#include <eepp/window/window.hpp>
#include <nlohmann/json.hpp>

using namespace EE::Graphics;
using namespace EE::System;
using namespace EE::UI;
using EE::Window::PlatformHelper;
using EE::Window::Runtime;
using EE::Window::RuntimeMode;
using namespace eterm::Terminal;
using EE::eemax;

namespace eterm {

AppConfig::AppConfig( std::string configPath, bool quakeMode ) :
	mConfigPath( std::move( configPath ) ), mQuakeMode( quakeMode ) {
	FileSystem::dirAddSlashAtEnd( mConfigPath );
	mStateDirectory = mConfigPath;
	if ( mQuakeMode ) {
		mStateDirectory += "quake";
		FileSystem::dirAddSlashAtEnd( mStateDirectory );
		window.tabPosition = UITabWidget::TabPosition::Bottom;
		window.alwaysShowTabBar = true;
	}
	mState.path( mStateDirectory + "state.cfg" );
}

std::string_view AppConfig::tabPreferencesSection() const {
	return mQuakeMode ? "quake" : "window";
}

std::string_view AppConfig::restorePreferencesSection() const {
	return mQuakeMode ? "quake" : "terminal";
}

void AppConfig::load() {
	const auto configFile = mConfigPath + "config.cfg";
	const bool newProfile = !FileSystem::fileExists( configFile ) &&
							!FileSystem::fileExists( mConfigPath + "state.cfg" );
	IniFile ini( configFile );
	mState.loadFromFile( mState.path() );
	if ( mQuakeMode ) {
		quake.shortcut = ini.getValue( "quake", "shortcut", quake.shortcut );
		quake.secondaryShortcut =
			ini.getValue( "quake", "secondary_shortcut", quake.secondaryShortcut );
		quake.widthPercent =
			std::clamp( ini.getValueI( "quake", "width_percent", quake.widthPercent ),
						QuakeConfig::MinSizePercent, QuakeConfig::MaxSizePercent );
		quake.heightPercent =
			std::clamp( ini.getValueI( "quake", "height_percent", quake.heightPercent ),
						QuakeConfig::MinSizePercent, QuakeConfig::MaxSizePercent );
		quake.animationMs = std::clamp( ini.getValueI( "quake", "animation_ms", quake.animationMs ),
										0, QuakeConfig::MaxAnimationMs );
		UIHorizontalAlignment alignment;
		if ( UIHorizontalAlignmentHelper::fromString(
				 ini.getValue( "quake", "horizontal_alignment" ), alignment ) &&
			 !isLogicalAlignment( alignment ) )
			quake.horizontalAlignment = alignment;
		quake.launchHidden = ini.getValueB( "quake", "launch_hidden", quake.launchHidden );
		quake.hideOnFocusLoss =
			ini.getValueB( "quake", "hide_on_focus_loss", quake.hideOnFocusLoss );
	}
	terminal.shell = ini.getValue( "terminal", "shell", terminal.shell );
	terminal.shellArguments =
		ini.getValue( "terminal", "shell_arguments", terminal.shellArguments );
	// The legacy working_directory key also captured one-off command-line paths.
	terminal.defaultWorkingDirectory =
		ini.getValue( "terminal", "default_working_directory", terminal.defaultWorkingDirectory );
	terminal.useCustomWorkingDirectory = ini.getValueB( "terminal", "use_custom_working_directory",
														!terminal.defaultWorkingDirectory.empty() );
	terminal.executeInShell =
		ini.getValue( "terminal", "execute_in_shell", terminal.executeInShell );
	terminal.load( ini );
	terminal.useFrameBuffer = ini.getValueB( "terminal", "framebuffer", terminal.useFrameBuffer );
	terminal.closeOnExit = ini.getValueB( "terminal", "close_on_exit", terminal.closeOnExit );
	terminal.initialTabs = ini.getValueU( "terminal", "initial_tabs", terminal.initialTabs );
	terminal.exclusiveMode = ini.getValueB( "terminal", "exclusive_mode", terminal.exclusiveMode );
	terminal.inheritWorkingDirectory =
		ini.getValueB( "terminal", "inherit_working_directory", terminal.inheritWorkingDirectory );
	terminal.programStatus.load( ini );
	terminal.restoreLastSession = ini.getValueB(
		restorePreferencesSection(), "restore_last_session", terminal.restoreLastSession );
	terminal.recoverHarnessSessions = ini.getValueB(
		restorePreferencesSection(), "recover_harness_sessions", terminal.recoverHarnessSessions );
	ui.smoothScroll = ini.getValueB(
		"ui", "smooth_scroll", ini.getValueB( "terminal", "smooth_scroll", ui.smoothScroll ) );
	const auto newTerminalBehavior = ini.getValue( "terminal", "new_terminal_behavior", "current" );
	terminal.newTerminalBehavior =
		newTerminalBehavior == "vertical"	  ? NewTerminalBehavior::VerticalSplit
		: newTerminalBehavior == "horizontal" ? NewTerminalBehavior::HorizontalSplit
											  : NewTerminalBehavior::CurrentTabBar;
	font.path = ini.getValue( "font", "path", font.path );
	font.fallbackPath = ini.getValue( "font", "fallback_path", font.fallbackPath );
	font.size = ini.getValueF( "font", "size", font.size );
	font.hinting = Font::fontHintingFromString(
		ini.getValue( "font", "hinting", Font::fontHintingToString( font.hinting ) ) );
	font.antialiasing = Font::fontAntialiasingFromString( ini.getValue(
		"font", "antialiasing", Font::fontAntialiasingToString( font.antialiasing ) ) );
	font.uiPath = ini.getValue( "font", "ui_path", font.uiPath );
	font.uiSize = ini.keyValueExists( "font", "ui_size" )
					  ? ini.getValueF( "font", "ui_size", font.uiSize )
					  : PlatformHelper::getSystemUIFontSize( font.uiSize );

	window.pixelDensity = ini.getValueF( "window", "pixel_density", window.pixelDensity );
	window.pixelDensityMode = newProfile ? EE::Window::Window::getNewProfilePixelDensityMode()
										 : EE::Window::Window::pixelDensityModeFromString(
											   ini.getValue( "window", "pixel_density_mode" ) );
	window.displayDensityOverrides = EE::Window::Window::loadDisplayDensityOverrides( ini );
	window.maxFPS = ini.getValueU( "window", "frame_rate_limit", window.maxFPS );
	window.vsync = ini.getValueB( "window", "vsync", window.vsync );
	window.exitOnClose = ini.getValueB( "window", "exit_on_close", window.exitOnClose );
	window.benchmarkMode = ini.getValueB( "window", "benchmark_mode", window.benchmarkMode );
	window.warnBeforeClose =
		ini.getValueB( "window", "warn_before_closing", window.warnBeforeClose );
	window.alwaysShowTabBar =
		ini.getValueB( tabPreferencesSection(), "always_show_tab_bar", window.alwaysShowTabBar );
	window.showTabBarWhenSplit = ini.getValueB( tabPreferencesSection(), "show_tab_bar_when_split",
												window.showTabBarWhenSplit );
	window.tabPosition = UITabWidget::tabPositionFromString(
		ini.getValue( tabPreferencesSection(), "tab_position",
					  UITabWidget::tabPositionToString( window.tabPosition ) ) );
	window.rendererVersion = Renderer::glVersionFromString(
		ini.getValue( "window", "renderer_version",
					  Renderer::graphicsLibraryVersionToString( window.rendererVersion ) ) );
	window.multisamples = ini.getValueU( "window", "multisamples", window.multisamples );
	theme.colorScheme = ini.getValue( "theme", "color_scheme", theme.colorScheme );
	theme.uiColorScheme = ColorSchemePreferences::fromStringExt( ini.getValue(
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
	mSavedPreferences = preferences();
}

bool AppConfig::save() {
	if ( !FileSystem::isDirectory( mConfigPath ) && !FileSystem::makeDir( mConfigPath, true ) )
		return false;
	return savePreferences() && saveWindowState();
}

IniFile AppConfig::preferences() const {
	IniFile ini;
	ini.setValue( "terminal", "shell", terminal.shell );
	ini.setValue( "terminal", "shell_arguments", terminal.shellArguments );
	ini.setValue( "terminal", "default_working_directory", terminal.defaultWorkingDirectory );
	ini.setValueB( "terminal", "use_custom_working_directory", terminal.useCustomWorkingDirectory );
	ini.setValue( "terminal", "execute_in_shell", terminal.executeInShell );
	terminal.save( ini );
	ini.setValueB( "terminal", "framebuffer", terminal.useFrameBuffer );
	ini.setValueB( "terminal", "close_on_exit", terminal.closeOnExit );
	ini.setValueU( "terminal", "initial_tabs", terminal.initialTabs );
	ini.setValueB( "terminal", "exclusive_mode", terminal.exclusiveMode );
	ini.setValueB( "terminal", "inherit_working_directory", terminal.inheritWorkingDirectory );
	terminal.programStatus.save( ini );
	ini.setValueB( restorePreferencesSection(), "restore_last_session",
				   terminal.restoreLastSession );
	ini.setValueB( restorePreferencesSection(), "recover_harness_sessions",
				   terminal.recoverHarnessSessions );
	ini.setValueB( "ui", "smooth_scroll", ui.smoothScroll );
	ini.setValue( "terminal", "new_terminal_behavior",
				  terminal.newTerminalBehavior == NewTerminalBehavior::VerticalSplit ? "vertical"
				  : terminal.newTerminalBehavior == NewTerminalBehavior::HorizontalSplit
					  ? "horizontal"
					  : "current" );
	ini.setValue( "font", "path", font.path );
	ini.setValue( "font", "fallback_path", font.fallbackPath );
	ini.setValueF( "font", "size", font.size );
	ini.setValue( "font", "hinting", Font::fontHintingToString( font.hinting ) );
	ini.setValue( "font", "antialiasing", Font::fontAntialiasingToString( font.antialiasing ) );
	ini.setValue( "font", "ui_path", font.uiPath );
	ini.setValueF( "font", "ui_size", font.uiSize );
	ini.setValueF( "window", "pixel_density", window.pixelDensity );
	ini.setValue( "window", "pixel_density_mode",
				  EE::Window::Window::pixelDensityModeToString( window.pixelDensityMode ) );
	EE::Window::Window::saveDisplayDensityOverrides( ini, window.displayDensityOverrides );
	ini.setValueU( "window", "frame_rate_limit", window.maxFPS );
	ini.setValueB( "window", "vsync", window.vsync );
	ini.setValueB( "window", "exit_on_close", window.exitOnClose );
	ini.setValueB( "window", "benchmark_mode", window.benchmarkMode );
	ini.setValueB( "window", "warn_before_closing", window.warnBeforeClose );
	ini.setValueB( tabPreferencesSection(), "always_show_tab_bar", window.alwaysShowTabBar );
	ini.setValue( tabPreferencesSection(), "tab_position",
				  UITabWidget::tabPositionToString( window.tabPosition ) );
	ini.setValueB( tabPreferencesSection(), "show_tab_bar_when_split", window.showTabBarWhenSplit );
	ini.setValue( "window", "renderer_version",
				  Renderer::graphicsLibraryVersionToString( window.rendererVersion ) );
	ini.setValueU( "window", "multisamples", window.multisamples );
	ini.setValue( "theme", "color_scheme", theme.colorScheme );
	ini.setValue( "theme", "ui_color_scheme",
				  ColorSchemePreferences::toString( theme.uiColorScheme ) );
	if ( mQuakeMode ) {
		ini.setValue( "quake", "shortcut", quake.shortcut );
		ini.setValue( "quake", "secondary_shortcut", quake.secondaryShortcut );
		ini.setValueI( "quake", "width_percent", quake.widthPercent );
		ini.setValueI( "quake", "height_percent", quake.heightPercent );
		ini.setValueI( "quake", "animation_ms", quake.animationMs );
		ini.setValue( "quake", "horizontal_alignment",
					  UIHorizontalAlignmentHelper::toString( quake.horizontalAlignment ) );
		ini.setValueB( "quake", "launch_hidden", quake.launchHidden );
		ini.setValueB( "quake", "hide_on_focus_loss", quake.hideOnFocusLoss );
	}
	return ini;
}

bool AppConfig::savePreferences() {
	if ( !FileSystem::isDirectory( mConfigPath ) && !FileSystem::makeDir( mConfigPath, true ) )
		return false;
#if EE_PLATFORM == EE_PLATFORM_WIN ||                                          \
	( defined( EE_PLATFORM_POSIX ) && EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN && \
	  EE_PLATFORM != EE_PLATFORM_SOLARIS )
	FileLock lock;
	const auto lockPath = mConfigPath + "config.lock";
	auto result = lock.tryLock( lockPath );
	for ( int retry = 0; result == FileLock::Result::Busy && retry < 10; ++retry ) {
		Sys::sleep( Milliseconds( 5 ) );
		result = lock.tryLock( lockPath );
	}
	if ( result != FileLock::Result::Acquired )
		return false;
#endif
	const auto configFile = mConfigPath + "config.cfg";
	IniFile disk( configFile, false );
	if ( FileSystem::fileExists( configFile ) && !disk.loadFromFile( configFile ) )
		return false;
	auto current = preferences();
	bool changed = false;

	// Both modes edit the shared file. Merge changes from this process into the latest copy
	// so a tab-bar setting cannot overwrite preferences edited in the other instance.
	for ( unsigned sectionId = 0; sectionId < current.getNumKeys(); ++sectionId ) {
		const auto section = current.getKeyName( sectionId );
		const bool hasDefaults = section != "display_scale_overrides";
		for ( unsigned i = 0; i < current.getNumValues( sectionId ); ++i ) {
			const auto key = current.getValueName( sectionId, i );
			const auto value = current.getValue( sectionId, i );
			const bool localChange = !mSavedPreferences.keyValueExists( section, key ) ||
									 mSavedPreferences.getValue( section, key ) != value;
			if ( ( localChange && disk.getValue( section, key ) != value ) ||
				 ( ( hasDefaults || localChange ) && !disk.keyValueExists( section, key ) ) ) {
				disk.setValue( section, key, value );
				changed = true;
			}
		}
	}
	for ( unsigned sectionId = 0; sectionId < mSavedPreferences.getNumKeys(); ++sectionId ) {
		const auto section = mSavedPreferences.getKeyName( sectionId );
		for ( unsigned i = 0; i < mSavedPreferences.getNumValues( sectionId ); ++i ) {
			const auto key = mSavedPreferences.getValueName( sectionId, i );
			if ( !current.keyValueExists( section, key ) && disk.deleteValue( section, key ) )
				changed = true;
		}
	}
	changed |= disk.deleteValue( "theme", "language" );
	changed |= disk.deleteValue( "window", "max_fps" );
	changed |= disk.deleteValue( "terminal", "working_directory" );
	changed |= disk.deleteValue( "terminal", "smooth_scroll" );
	if ( changed && !disk.writeFileAtomic() )
		return false;
	mSavedPreferences = std::move( current );
	return true;
}

bool AppConfig::saveWindowState() {
	if ( !FileSystem::isDirectory( mStateDirectory ) &&
		 !FileSystem::makeDir( mStateDirectory, true ) )
		return false;
	if ( Runtime::mode() != RuntimeMode::Terminal ) {
		mState.setValueI( "window", "width", windowState.size.getWidth() );
		mState.setValueI( "window", "height", windowState.size.getHeight() );
		mState.setValueI( "window", "x", windowState.position.x );
		mState.setValueI( "window", "y", windowState.position.y );
		mState.setValueI( "window", "display_index", windowState.displayIndex );
		mState.setValueB( "window", "maximized", windowState.maximized );
	}
	mState.setValueF( "window", "tab_bar_width", windowState.tabBarWidth );
	return mState.writeFileAtomic();
}

bool AppConfig::takeQuakeShortcutUnavailableNotice() {
	if ( mState.getValueB( "quake", "shortcut_unavailable_notice_shown" ) )
		return false;
	mState.setValueB( "quake", "shortcut_unavailable_notice_shown", true );
	return true;
}

void AppConfig::captureWindowState( EE::Window::Window* window ) {
	if ( !window || mQuakeMode || Runtime::mode() == RuntimeMode::Terminal )
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
	if ( !FileSystem::isDirectory( mStateDirectory ) &&
		 !FileSystem::makeDir( mStateDirectory, true ) )
		return false;
	// Titles can come from programs, so an invalid sequence must not fail the whole session.
	return FileSystem::fileWriteAtomic(
		sessionPath(), session.dump( -1, ' ', false, nlohmann::json::error_handler_t::replace ),
		true );
}

} // namespace eterm
