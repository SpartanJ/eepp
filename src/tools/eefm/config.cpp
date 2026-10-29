#include "config.hpp"

#include <eepp/system/filesystem.hpp>

using namespace eterm::Terminal;

namespace eefm {

AppConfig::AppConfig( std::string configDirectory ) {
	FileSystem::dirAddSlashAtEnd( configDirectory );
	mPath = configDirectory + "config.cfg";
}

void AppConfig::load() {
	if ( !FileSystem::fileExists( mPath ) )
		return;
	mIni.loadFromFile( mPath );
	theme.uiColorScheme = ColorSchemePreferences::fromStringExt( mIni.getValue(
		"theme", "ui_color_scheme", ColorSchemePreferences::toString( theme.uiColorScheme ) ) );

	font.uiPath = mIni.getValue( "font", "ui_path", font.uiPath );
	font.uiSize = mIni.getValueF( "font", "ui_size", font.uiSize );
	font.fallbackPath = mIni.getValue( "font", "fallback_path", font.fallbackPath );
	font.hinting = Font::fontHintingFromString(
		mIni.getValue( "font", "hinting", Font::fontHintingToString( font.hinting ) ) );
	font.antialiasing = Font::fontAntialiasingFromString( mIni.getValue(
		"font", "antialiasing", Font::fontAntialiasingToString( font.antialiasing ) ) );

	files.singleClick = mIni.getValueB( "files", "single_click", files.singleClick );
	files.countFolderItems =
		mIni.getValueB( "files", "count_folder_items", files.countFolderItems );
	files.thumbnails = mIni.getValueB( "files", "thumbnails", files.thumbnails );
	files.cacheThumbnails = mIni.getValueB( "files", "cache_thumbnails", files.cacheThumbnails );
	files.remoteThumbnails = mIni.getValueB( "files", "remote_thumbnails", files.remoteThumbnails );
	files.thumbnailCacheFormat = ImageThumbnailer::cacheFormatFromString(
		mIni.getValue( "files", "thumbnail_cache_format",
					   ImageThumbnailer::cacheFormatToString( files.thumbnailCacheFormat ) ) );
	files.systemIcons = mIni.getValueB( "files", "system_icons", files.systemIcons );
	files.smoothScroll = mIni.getValueB( "files", "smooth_scroll", files.smoothScroll );

	terminal.fontPath = mIni.getValue( "terminal", "font_path", terminal.fontPath );
	terminal.fontSize = mIni.getValueF( "terminal", "font_size", terminal.fontSize );
	terminal.colorScheme = mIni.getValue( "terminal", "color_scheme", terminal.colorScheme );
	terminal.shell = mIni.getValue( "terminal", "shell", terminal.shell );
	terminal.shellArguments =
		mIni.getValue( "terminal", "shell_arguments", terminal.shellArguments );
	terminal.historySize = mIni.getValueU( "terminal", "scrollback", terminal.historySize );
	terminal.cursorStyle = TerminalCursorHelper::modeFromString( mIni.getValue(
		"terminal", "cursor_style", TerminalCursorHelper::modeToString( terminal.cursorStyle ) ) );
	const std::string scrollBarMode = mIni.getValue( "terminal", "scrollbar_mode", "auto" );
	terminal.scrollBarMode = scrollBarMode == "always_on"	 ? ScrollBarMode::AlwaysOn
							 : scrollBarMode == "always_off" ? ScrollBarMode::AlwaysOff
															 : ScrollBarMode::Auto;
	terminal.followActivePane =
		mIni.getValueB( "terminal", "follow_active_pane", terminal.followActivePane );

	window.pixelDensity = mIni.getValueF( "window", "pixel_density", window.pixelDensity );
	window.maxFPS = mIni.getValueU( "window", "frame_rate_limit", window.maxFPS );
	window.vsync = mIni.getValueB( "window", "vsync", window.vsync );
	window.exitOnClose = mIni.getValueB( "window", "exit_on_close", window.exitOnClose );
	window.showMenuBar = mIni.getValueB( "window", "show_menu_bar", window.showMenuBar );
	window.rendererVersion = Renderer::glVersionFromString(
		mIni.getValue( "window", "renderer_version",
					   Renderer::graphicsLibraryVersionToString( window.rendererVersion ) ) );
	window.multisamples = mIni.getValueU( "window", "multisamples", window.multisamples );
}

bool AppConfig::save() {
	std::string directory = FileSystem::fileRemoveFileName( mPath );
	if ( !FileSystem::isDirectory( directory ) && !FileSystem::makeDir( directory, true ) )
		return false;
	mIni.path( mPath );
	mIni.setValue( "theme", "ui_color_scheme",
				   ColorSchemePreferences::toString( theme.uiColorScheme ) );

	mIni.setValue( "font", "ui_path", font.uiPath );
	mIni.setValueF( "font", "ui_size", font.uiSize );
	mIni.setValue( "font", "fallback_path", font.fallbackPath );
	mIni.setValue( "font", "hinting", Font::fontHintingToString( font.hinting ) );
	mIni.setValue( "font", "antialiasing", Font::fontAntialiasingToString( font.antialiasing ) );

	mIni.setValueB( "files", "single_click", files.singleClick );
	mIni.setValueB( "files", "count_folder_items", files.countFolderItems );
	mIni.setValueB( "files", "system_icons", files.systemIcons );
	mIni.setValueB( "files", "thumbnails", files.thumbnails );
	mIni.setValueB( "files", "cache_thumbnails", files.cacheThumbnails );
	mIni.setValueB( "files", "remote_thumbnails", files.remoteThumbnails );
	mIni.setValue( "files", "thumbnail_cache_format",
				   ImageThumbnailer::cacheFormatToString( files.thumbnailCacheFormat ) );
	mIni.setValueB( "files", "smooth_scroll", files.smoothScroll );

	mIni.setValue( "terminal", "font_path", terminal.fontPath );
	mIni.setValueF( "terminal", "font_size", terminal.fontSize );
	mIni.setValue( "terminal", "color_scheme", terminal.colorScheme );
	mIni.setValue( "terminal", "shell", terminal.shell );
	mIni.setValue( "terminal", "shell_arguments", terminal.shellArguments );
	mIni.setValueU( "terminal", "scrollback", terminal.historySize );
	mIni.setValue( "terminal", "cursor_style",
				   TerminalCursorHelper::modeToString( terminal.cursorStyle ) );
	mIni.setValue( "terminal", "scrollbar_mode",
				   terminal.scrollBarMode == ScrollBarMode::AlwaysOn	? "always_on"
				   : terminal.scrollBarMode == ScrollBarMode::AlwaysOff ? "always_off"
																		: "auto" );
	mIni.setValueB( "terminal", "follow_active_pane", terminal.followActivePane );

	mIni.setValueF( "window", "pixel_density", window.pixelDensity );
	mIni.setValueU( "window", "frame_rate_limit", window.maxFPS );
	mIni.setValueB( "window", "vsync", window.vsync );
	mIni.setValueB( "window", "exit_on_close", window.exitOnClose );
	mIni.setValueB( "window", "show_menu_bar", window.showMenuBar );
	mIni.setValue( "window", "renderer_version",
				   Renderer::graphicsLibraryVersionToString( window.rendererVersion ) );
	mIni.setValueU( "window", "multisamples", window.multisamples );
	return mIni.writeFile();
}

} // namespace eefm
