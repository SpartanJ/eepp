#include "appconfig.hpp"
#include <eepp/network/uri.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/md5.hpp>
#include <eepp/system/sys.hpp>

using namespace EE::Network;

static std::vector<std::string> urlEncode( const std::vector<std::string>& vec ) {
	std::vector<std::string> encoded;
	for ( const auto& item : vec )
		encoded.emplace_back( URI::encode( item ) );
	return encoded;
}

static std::vector<std::string> urlDecode( const std::vector<std::string>& vec ) {
	std::vector<std::string> decoded;
	for ( const auto& item : vec )
		decoded.emplace_back( URI::decode( item ) );
	return decoded;
}

void AppConfig::load( std::string& confPath, std::string& keybindingsPath,
					  std::string& initColorScheme, std::vector<std::string>& recentFiles,
					  std::vector<std::string>& recentFolders, const std::string& resPath,
					  const Float& displayDPI ) {
	confPath = Sys::getConfigPath( "ecode" );
	if ( !FileSystem::fileExists( confPath ) )
		FileSystem::makeDir( confPath );
	FileSystem::dirAddSlashAtEnd( confPath );
	keybindingsPath = confPath + "keybindings.cfg";
	ini.loadFromFile( confPath + "config.cfg" );
	iniState.loadFromFile( confPath + "state.cfg" );
	std::string recent = iniState.getValue( "files", "recentfiles", "" );
	recentFiles = urlDecode( String::split( recent, ';' ) );
	std::string recentFol = iniState.getValue( "folders", "recentfolders", "" );
	recentFolders = urlDecode( String::split( recentFol, ';' ) );
	initColorScheme = editor.colorScheme = ini.getValue( "editor", "colorscheme", "eepp" );
	editor.fontSize = ini.getValue( "editor", "font_size", "11dp" );
	window.size.setWidth( iniState.getValueI( "window", "width", displayDPI > 105 ? 1920 : 1280 ) );
	window.size.setHeight(
		iniState.getValueI( "window", "height", displayDPI > 105 ? 1080 : 720 ) );
	window.maximized = iniState.getValueB( "window", "maximized", false );
	window.pixelDensity = iniState.getValueF( "window", "pixeldensity" );
	window.winIcon = ini.getValue( "window", "winicon", resPath + "assets/icon/ee.png" );
	window.panelPartition = iniState.getValue( "window", "panel_partition", "15%" );
	editor.showLineNumbers = ini.getValueB( "editor", "show_line_numbers", true );
	editor.showWhiteSpaces = ini.getValueB( "editor", "show_white_spaces", true );
	editor.highlightMatchingBracket =
		ini.getValueB( "editor", "highlight_matching_brackets", true );
	editor.highlightCurrentLine = ini.getValueB( "editor", "highlight_current_line", true );
	editor.horizontalScrollbar = ini.getValueB( "editor", "horizontal_scrollbar", false );
	ui.fontSize = ini.getValue( "ui", "font_size", "11dp" );
	ui.showSidePanel = ini.getValueB( "ui", "show_side_panel", true );
	ui.serifFont = ini.getValue( "ui", "serif_font", "assets/fonts/NotoSans-Regular.ttf" );
	ui.monospaceFont = ini.getValue( "ui", "monospace_font", "assets/fonts/DejaVuSansMono.ttf" );
	editor.trimTrailingWhitespaces = ini.getValueB( "editor", "trim_trailing_whitespaces", false );
	editor.forceNewLineAtEndOfFile =
		ini.getValueB( "editor", "force_new_line_at_end_of_file", false );
	editor.autoDetectIndentType = ini.getValueB( "editor", "auto_detect_indent_type", true );
	editor.writeUnicodeBOM = ini.getValueB( "editor", "write_bom", false );
	editor.autoCloseBrackets = ini.getValue( "editor", "auto_close_brackets", "" );
	editor.indentWidth = ini.getValueI( "editor", "indent_width", 4 );
	editor.indentSpaces = ini.getValueB( "editor", "indent_spaces", false );
	editor.windowsLineEndings = ini.getValueB( "editor", "windows_line_endings", false );
	editor.tabWidth = eemax( 2, ini.getValueI( "editor", "tab_width", 4 ) );
	editor.lineBreakingColumn = eemax( 0, ini.getValueI( "editor", "line_breaking_column", 100 ) );
	editor.highlightSelectionMatch = ini.getValueB( "editor", "highlight_selection_match", true );
	editor.colorPickerSelection = ini.getValueB( "editor", "color_picker_selection", true );
	editor.colorPreview = ini.getValueB( "editor", "color_preview", true );
	editor.autoComplete = ini.getValueB( "editor", "auto_complete", true );
	editor.linter = ini.getValueB( "editor", "linter", true );
	editor.formatter = ini.getValueB( "editor", "formatter", true );
	editor.showDocInfo = ini.getValueB( "editor", "show_doc_info", true );
	editor.hideTabBarOnSingleTab = ini.getValueB( "editor", "hide_tab_bar_on_single_tab", true );
	editor.singleClickTreeNavigation = ini.getValueB( "editor", "single_click_tree_navigation", false );
}

void AppConfig::save( const std::vector<std::string>& recentFiles,
					  const std::vector<std::string>& recentFolders,
					  const std::string& panelPartition, EE::Window::Window* win,
					  const std::string& colorSchemeName ) {
	editor.colorScheme = colorSchemeName;
	window.size = win->getLastWindowedSize();
	window.maximized = win->isMaximized();
	ini.setValue( "editor", "colorscheme", editor.colorScheme );
	iniState.setValueI( "window", "width", window.size.getWidth() );
	iniState.setValueI( "window", "height", window.size.getHeight() );
	iniState.setValueB( "window", "maximized", window.maximized );
	iniState.setValueF( "window", "pixeldensity", window.pixelDensity );
	iniState.setValue( "window", "panel_partition", panelPartition );
	iniState.setValue( "files", "recentfiles", String::join( urlEncode( recentFiles ), ';' ) );
	iniState.setValue( "folders", "recentfolders",
					   String::join( urlEncode( recentFolders ), ';' ) );
	ini.setValueB( "editor", "show_line_numbers", editor.showLineNumbers );
	ini.setValueB( "editor", "show_white_spaces", editor.showWhiteSpaces );
	ini.setValueB( "editor", "highlight_matching_brackets", editor.highlightMatchingBracket );
	ini.setValueB( "editor", "highlight_current_line", editor.highlightCurrentLine );
	ini.setValueB( "editor", "horizontal_scrollbar", editor.horizontalScrollbar );
	ini.setValue( "editor", "font_size", editor.fontSize.toString() );
	ini.setValue( "ui", "font_size", ui.fontSize.toString() );
	ini.setValueB( "ui", "show_side_panel", ui.showSidePanel );
	ini.setValue( "ui", "serif_font", ui.serifFont );
	ini.setValue( "ui", "monospace_font", ui.monospaceFont );
	ini.setValueB( "editor", "trim_trailing_whitespaces", editor.trimTrailingWhitespaces );
	ini.setValueB( "editor", "force_new_line_at_end_of_file", editor.forceNewLineAtEndOfFile );
	ini.setValueB( "editor", "auto_detect_indent_type", editor.autoDetectIndentType );
	ini.setValueB( "editor", "write_bom", editor.writeUnicodeBOM );
	ini.setValue( "editor", "auto_close_brackets", editor.autoCloseBrackets );
	ini.setValueI( "editor", "indent_width", editor.indentWidth );
	ini.setValueB( "editor", "indent_spaces", editor.indentSpaces );
	ini.setValueB( "editor", "windows_line_endings", editor.windowsLineEndings );
	ini.setValueI( "editor", "tab_width", editor.tabWidth );
	ini.setValueI( "editor", "line_breaking_column", editor.lineBreakingColumn );
	ini.setValueB( "editor", "highlight_selection_match", editor.highlightSelectionMatch );
	ini.setValueB( "editor", "color_picker_selection", editor.colorPickerSelection );
	ini.setValueB( "editor", "color_preview", editor.colorPreview );
	ini.setValueB( "editor", "auto_complete", editor.autoComplete );
	ini.setValueB( "editor", "linter", editor.linter );
	ini.setValueB( "editor", "formatter", editor.formatter );
	ini.setValueB( "editor", "show_doc_info", editor.showDocInfo );
	ini.setValueB( "editor", "hide_tab_bar_on_single_tab", editor.hideTabBarOnSingleTab );
	ini.setValueB( "editor", "single_click_tree_navigation", editor.singleClickTreeNavigation );
	ini.writeFile();
	iniState.writeFile();
}

void AppConfig::saveProject( std::string projectFolder, UICodeEditorSplitter* editorSplitter,
							 const std::string& configPath ) {
	FileSystem::dirAddSlashAtEnd( projectFolder );
	std::vector<UICodeEditor*> editors = editorSplitter->getAllEditors();
	std::vector<std::string> paths;
	for ( auto editor : editors )
		if ( editor->getDocument().hasFilepath() )
			paths.emplace_back( editor->getDocument().getFilePath() );
	std::string projectsPath( configPath + "projects" + FileSystem::getOSSlash() );
	if ( !FileSystem::fileExists( projectsPath ) )
		FileSystem::makeDir( projectsPath );
	MD5::Result hash = MD5::fromString( projectFolder );
	std::string projectCfgPath( projectsPath + hash.toHexString() + ".cfg" );
	IniFile ini( projectCfgPath, false );
	ini.setValue( "path", "folder_path", projectFolder );
	for ( size_t i = 0; i < paths.size(); i++ )
		ini.setValue( "files", String::format( "file_name_%lu", i ), paths[i] );
	ini.writeFile();
}

void AppConfig::loadProject( std::string projectFolder, UICodeEditorSplitter* editorSplitter,
							 const std::string& configPath ) {
	FileSystem::dirAddSlashAtEnd( projectFolder );
	std::string projectsPath( configPath + "projects" + FileSystem::getOSSlash() );
	MD5::Result hash = MD5::fromString( projectFolder );
	std::string projectCfgPath( projectsPath + hash.toHexString() + ".cfg" );
	if ( !FileSystem::fileExists( projectCfgPath ) )
		return;
	IniFile ini( projectCfgPath );
	bool found;
	size_t i = 0;
	do {
		std::string val( ini.getValue( "files", String::format( "file_name_%lu", i ) ) );
		found = !val.empty();
		if ( found )
			editorSplitter->loadFileFromPathInNewTab( val );
		i++;
	} while ( found );
}
