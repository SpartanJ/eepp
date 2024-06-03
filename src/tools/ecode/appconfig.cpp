#include "appconfig.hpp"
#include "ecode.hpp"
#include "plugins/pluginmanager.hpp"
#include "version.hpp"
#include <eepp/network/uri.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/md5.hpp>
#include <eepp/system/sys.hpp>
#include <eterm/ui/uiterminal.hpp>

using namespace EE::Network;
using namespace eterm::UI;
using json = nlohmann::json;
using namespace std::literals;

namespace ecode {

static PanelPosition panelPositionFromString( const std::string& str ) {
	if ( String::toLower( str ) == "right" )
		return PanelPosition::Right;
	return PanelPosition::Left;
}

static std::string panelPositionToString( const PanelPosition& pos ) {
	if ( pos == PanelPosition::Right )
		return "right";
	return "left";
}

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

void AppConfig::load( const std::string& confPath, std::string& keybindingsPath,
					  std::string& initColorScheme, std::vector<std::string>& recentFiles,
					  std::vector<std::string>& recentFolders, const std::string& resPath,
					  PluginManager* pluginManager, const Sizei& displaySize, bool sync ) {
	keybindingsPath = confPath + "keybindings.cfg";
	ini.loadFromFile( confPath + "config.cfg" );
	iniState.loadFromFile( confPath + "state.cfg" );
	std::string recent = iniState.getValue( "files", "recentfiles", "" );
	recentFiles = urlDecode( String::split( recent, ';' ) );
	std::string recentFol = iniState.getValue( "folders", "recentfolders", "" );
	recentFolders = urlDecode( String::split( recentFol, ';' ) );
	initColorScheme = editor.colorScheme = ini.getValue( "editor", "colorscheme", "eepp" );
	editor.fontSize = ini.getValue( "editor", "font_size", "11dp" );
	const Sizei desiredRes( 1280, 720 );
	Sizei defWinSize( desiredRes.getWidth(), desiredRes.getHeight() );
	if ( displaySize.getWidth() > desiredRes.getWidth() &&
		 displaySize.getWidth() - desiredRes.getWidth() > 60 ) {
		defWinSize.setWidth( desiredRes.getWidth() );
	} else {
		defWinSize.setWidth( displaySize.getWidth() - 60 );
	}
	if ( displaySize.getHeight() > desiredRes.getHeight() &&
		 displaySize.getHeight() - desiredRes.getHeight() > 60 ) {
		defWinSize.setHeight( desiredRes.getHeight() );
	} else {
		defWinSize.setHeight( displaySize.getHeight() - 60 );
	}
	windowState.size.setWidth( iniState.getValueI( "window", "width", defWinSize.getWidth() ) );
	windowState.size.setHeight( iniState.getValueI( "window", "height", defWinSize.getHeight() ) );
	windowState.maximized = iniState.getValueB( "window", "maximized", false );
	windowState.pixelDensity = iniState.getValueF( "window", "pixeldensity" );
	windowState.winIcon = ini.getValue( "window", "winicon", resPath + "icon/ecode.png" );
	windowState.panelPartition = iniState.getValue( "window", "panel_partition", "15%" );
	windowState.statusBarPartition = iniState.getValue( "window", "status_bar_partition", "85%" );
	windowState.displayIndex = iniState.getValueI( "window", "display_index", 0 );
	windowState.position.x = iniState.getValueI( "window", "x", -1 );
	windowState.position.y = iniState.getValueI( "window", "y", -1 );
	windowState.lastRunVersion = iniState.getValueU( "editor", "last_run_version", 0 );
	editor.showLineNumbers = ini.getValueB( "editor", "show_line_numbers", true );
	editor.showWhiteSpaces = ini.getValueB( "editor", "show_white_spaces", true );
	editor.showLineEndings = ini.getValueB( "editor", "show_line_endings", false );
	editor.showIndentationGuides = ini.getValueB( "editor", "show_indentation_guides", false );
	editor.highlightMatchingBracket =
		ini.getValueB( "editor", "highlight_matching_brackets", true );
	editor.highlightCurrentLine = ini.getValueB( "editor", "highlight_current_line", true );
	editor.verticalScrollbar = ini.getValueB( "editor", "vertical_scrollbar", true );
	editor.horizontalScrollbar = ini.getValueB( "editor", "horizontal_scrollbar", true );
	ui.fontSize = ini.getValue( "ui", "font_size", "11dp" );
	ui.panelFontSize = ini.getValue( "ui", "panel_font_size", "11dp" );
	ui.showSidePanel = ini.getValueB( "ui", "show_side_panel", true );
	ui.showStatusBar = ini.getValueB( "ui", "show_status_bar", true );
	ui.showMenuBar = ini.getValueB( "ui", "show_menu_bar", false );
	ui.panelPosition = panelPositionFromString( ini.getValue( "ui", "panel_position", "left" ) );
	ui.serifFont = ini.getValue( "ui", "serif_font", "fonts/NotoSans-Regular.ttf" );
	ui.monospaceFont = ini.getValue( "ui", "monospace_font", "fonts/DejaVuSansMono.ttf" );
	ui.terminalFont =
		ini.getValue( "ui", "terminal_font", "fonts/DejaVuSansMonoNerdFontComplete.ttf" );
	ui.fallbackFont = ini.getValue( "ui", "fallback_font", "fonts/DroidSansFallbackFull.ttf" );
	ui.theme = ini.getValue( "ui", "theme" );
	ui.language = ini.getValue( "ui", "language" );
	ui.colorScheme = ini.getValue( "ui", "ui_color_scheme", "dark" ) == "light"
						 ? ColorSchemePreference::Light
						 : ColorSchemePreference::Dark;
	doc.trimTrailingWhitespaces = ini.getValueB( "document", "trim_trailing_whitespaces", false );
	doc.forceNewLineAtEndOfFile =
		ini.getValueB( "document", "force_new_line_at_end_of_file", false );
	doc.autoDetectIndentType = ini.getValueB( "document", "auto_detect_indent_type", true );
	doc.writeUnicodeBOM = ini.getValueB( "document", "write_bom", false );
	doc.indentWidth = ini.getValueI( "document", "indent_width", 4 );
	doc.indentSpaces = ini.getValueB( "document", "indent_spaces", false );
	doc.lineEndings =
		TextFormat::stringToLineEnding( ini.getValue( "document", "line_endings", "LF" ) );
	// Migrate old data
	if ( ini.keyValueExists( "document", "windows_line_endings" ) &&
		 !ini.keyValueExists( "document", "line_endings" ) &&
		 ini.getValueB( "document", "windows_line_endings" ) == true ) {
		doc.lineEndings = TextFormat::LineEnding::CRLF;
	}

	doc.tabWidth = eemax( 2, ini.getValueI( "document", "tab_width", 4 ) );
	doc.lineBreakingColumn = eemax( 0, ini.getValueI( "document", "line_breaking_column", 100 ) );
	editor.autoCloseBrackets = ini.getValue( "editor", "auto_close_brackets", "" );
	editor.highlightSelectionMatch = ini.getValueB( "editor", "highlight_selection_match", true );
	editor.colorPickerSelection = ini.getValueB( "editor", "color_picker_selection", true );
	editor.colorPreview = ini.getValueB( "editor", "color_preview", true );
	editor.minimap = ini.getValueB( "editor", "minimap", true );
	editor.showDocInfo = ini.getValueB( "editor", "show_doc_info", true );
	editor.hideTabBarOnSingleTab = ini.getValueB( "editor", "hide_tab_bar_on_single_tab", false );
	editor.singleClickNavigation = ini.getValueB( "editor", "single_click_tree_navigation", false );
	editor.syncProjectTreeWithEditor =
		ini.getValueB( "editor", "sync_project_tree_with_editor", true );
	editor.autoCloseXMLTags = ini.getValueB( "editor", "auto_close_xml_tags", true );
	editor.lineSpacing = ini.getValue( "editor", "line_spacing", "0dp" );
	editor.cursorBlinkingTime =
		Time::fromString( ini.getValue( "editor", "cursor_blinking_time", "0.5s" ) );
	editor.linesRelativePosition = ini.getValueB( "editor", "lines_relative_position", false );
	editor.autoReloadOnDiskChange = ini.getValueB( "editor", "auto_reload_on_disk_change", false );

	editor.wrapMode =
		DocumentView::toLineWrapMode( ini.getValue( "editor", "wrap_mode", "nowrap" ) );
	editor.wrapType =
		DocumentView::toLineWrapType( ini.getValue( "editor", "wrap_type", "viewport" ) );
	editor.wrapKeepIndentation = ini.getValueB( "editor", "wrap_keep_indentation", true );

	editor.codeFoldingEnabled = ini.getValueB( "editor", "code_folding_enabled", true );
	editor.codeFoldingAlwaysVisible =
		ini.getValueB( "editor", "code_folding_always_visible", false );
	editor.codeFoldingRefreshFreq =
		Time::fromString( ini.getValue( "editor", "code_folding_refresh_frequency", "2s" ) );

	searchBarConfig.caseSensitive = ini.getValueB( "search_bar", "case_sensitive", false );
	searchBarConfig.luaPattern = ini.getValueB( "search_bar", "lua_pattern", false );
	searchBarConfig.wholeWord = ini.getValueB( "search_bar", "whole_word", false );
	searchBarConfig.escapeSequence = ini.getValueB( "search_bar", "escape_sequence", false );

	globalSearchBarConfig.caseSensitive =
		ini.getValueB( "global_search_bar", "case_sensitive", false );
	globalSearchBarConfig.luaPattern = ini.getValueB( "global_search_bar", "lua_pattern", false );
	globalSearchBarConfig.wholeWord = ini.getValueB( "global_search_bar", "whole_word", false );
	globalSearchBarConfig.escapeSequence =
		ini.getValueB( "global_search_bar", "escape_sequence", false );

	term.shell = ini.getValue( "terminal", "shell" );
	term.fontSize = ini.getValue( "terminal", "font_size", "11dp" );
	term.colorScheme = ini.getValue( "terminal", "colorscheme", "eterm" );
	term.newTerminalOrientation = NewTerminalOrientation::fromString(
		ini.getValue( "terminal", "new_terminal_orientation", "vertical" ) );
	term.scrollback = ini.getValueI( "terminal", "scrollback", 10000 );
	term.unsupportedOSWarnDisabled = ini.getValueB( "terminal", "unsupported_os_warn_disabled" );

	workspace.restoreLastSession = ini.getValueB( "workspace", "restore_last_session", false );
	workspace.checkForUpdatesAtStartup =
		ini.getValueB( "workspace", "check_for_updates_at_startup", true );

	std::map<std::string, bool> pluginsEnabled;
	const auto& creators = pluginManager->getDefinitions();
	for ( const auto& creator : creators ) {
		pluginsEnabled[creator.first] =
			ini.getValueB( "plugins", creator.first,
						   "autocomplete" == creator.first || "linter" == creator.first ||
							   "autoformatter" == creator.first || "lspclient" == creator.first ||
							   "git" == creator.first );
	}
	pluginManager->setPluginsEnabled( pluginsEnabled, sync );

	languagesExtensions.priorities = ini.getKeyMap( "languages_extensions" );

	iniInfo = FileInfo( ini.path() );
}

void AppConfig::save( const std::vector<std::string>& recentFiles,
					  const std::vector<std::string>& recentFolders,
					  const std::string& panelPartition, const std::string& statusBarPartition,
					  EE::Window::Window* win, const std::string& colorSchemeName,
					  const SearchBarConfig& searchBarConfig,
					  const GlobalSearchBarConfig& globalSearchBarConfig,
					  PluginManager* pluginManager ) {

	FileInfo configInfo( ini.path() );
	if ( iniInfo.getModificationTime() != 0 &&
		 iniInfo.getModificationTime() != configInfo.getModificationTime() ) {
		ini.loadFromFile( ini.path() );
	}

	editor.colorScheme = colorSchemeName;
	windowState.size = win->getLastWindowedSize();
	windowState.maximized = win->isMaximized();
	windowState.displayIndex = win->getCurrentDisplayIndex();
	windowState.position = win->getPosition();
	if ( windowState.position.x < 0 )
		windowState.position.x = 0;
	if ( windowState.position.y < 0 )
		windowState.position.y = 0;
	ini.setValue( "editor", "colorscheme", editor.colorScheme );
	iniState.setValueI( "window", "width", windowState.size.getWidth() );
	iniState.setValueI( "window", "height", windowState.size.getHeight() );
	iniState.setValueB( "window", "maximized", windowState.maximized );
	iniState.setValueF( "window", "pixeldensity", windowState.pixelDensity );
	iniState.setValue( "window", "panel_partition", panelPartition );
	iniState.setValue( "window", "status_bar_partition", statusBarPartition );
	iniState.setValueI( "window", "display_index", windowState.displayIndex );
	iniState.setValueI( "window", "x", windowState.position.x );
	iniState.setValueI( "window", "y", windowState.position.y );
	iniState.setValue( "files", "recentfiles", String::join( urlEncode( recentFiles ), ';' ) );
	iniState.setValue( "folders", "recentfolders",
					   String::join( urlEncode( recentFolders ), ';' ) );
	iniState.setValueU( "editor", "last_run_version", ecode::Version::getVersionNum() );
	ini.setValueB( "editor", "show_line_numbers", editor.showLineNumbers );
	ini.setValueB( "editor", "show_white_spaces", editor.showWhiteSpaces );
	ini.setValueB( "editor", "show_indentation_guides", editor.showIndentationGuides );
	ini.setValueB( "editor", "show_line_endings", editor.showLineEndings );
	ini.setValueB( "editor", "highlight_matching_brackets", editor.highlightMatchingBracket );
	ini.setValueB( "editor", "highlight_current_line", editor.highlightCurrentLine );
	ini.setValueB( "editor", "vertical_scrollbar", editor.verticalScrollbar );
	ini.setValueB( "editor", "horizontal_scrollbar", editor.horizontalScrollbar );
	ini.setValue( "editor", "font_size", editor.fontSize.toString() );
	ini.setValue( "ui", "font_size", ui.fontSize.toString() );
	ini.setValue( "ui", "panel_font_size", ui.panelFontSize.toString() );
	ini.setValueB( "ui", "show_side_panel", ui.showSidePanel );
	ini.setValueB( "ui", "show_status_bar", ui.showStatusBar );
	ini.setValueB( "ui", "show_menu_bar", ui.showMenuBar );
	ini.setValue( "ui", "panel_position", panelPositionToString( ui.panelPosition ) );
	ini.setValue( "ui", "serif_font", ui.serifFont );
	ini.setValue( "ui", "monospace_font", ui.monospaceFont );
	ini.setValue( "ui", "terminal_font", ui.terminalFont );
	ini.setValue( "ui", "theme", ui.theme );
	ini.setValue( "ui", "language", ui.language );
	ini.setValue( "ui", "fallback_font", ui.fallbackFont );
	ini.setValue(
		"ui", "ui_color_scheme",
		std::string_view{ ui.colorScheme == ColorSchemePreference::Light ? "light" : "dark" } );
	ini.setValueB( "document", "trim_trailing_whitespaces", doc.trimTrailingWhitespaces );
	ini.setValueB( "document", "force_new_line_at_end_of_file", doc.forceNewLineAtEndOfFile );
	ini.setValueB( "document", "auto_detect_indent_type", doc.autoDetectIndentType );
	ini.setValueB( "document", "write_bom", doc.writeUnicodeBOM );
	ini.setValueI( "document", "indent_width", doc.indentWidth );
	ini.setValueB( "document", "indent_spaces", doc.indentSpaces );
	ini.setValue( "document", "line_endings", TextFormat::lineEndingToString( doc.lineEndings ) );
	ini.setValueI( "document", "tab_width", doc.tabWidth );
	ini.setValueI( "document", "line_breaking_column", doc.lineBreakingColumn );
	ini.setValue( "editor", "auto_close_brackets", editor.autoCloseBrackets );
	ini.setValueB( "editor", "highlight_selection_match", editor.highlightSelectionMatch );
	ini.setValueB( "editor", "color_picker_selection", editor.colorPickerSelection );
	ini.setValueB( "editor", "color_preview", editor.colorPreview );
	ini.setValueB( "editor", "minimap", editor.minimap );
	ini.setValueB( "editor", "show_doc_info", editor.showDocInfo );
	ini.setValueB( "editor", "hide_tab_bar_on_single_tab", editor.hideTabBarOnSingleTab );
	ini.setValueB( "editor", "single_click_tree_navigation", editor.singleClickNavigation );
	ini.setValueB( "editor", "sync_project_tree_with_editor", editor.syncProjectTreeWithEditor );
	ini.setValueB( "editor", "auto_close_xml_tags", editor.autoCloseXMLTags );
	ini.setValue( "editor", "line_spacing", editor.lineSpacing.toString() );
	ini.setValue( "editor", "cursor_blinking_time", editor.cursorBlinkingTime.toString() );
	ini.setValueB( "editor", "lines_relative_position", editor.linesRelativePosition );
	ini.setValueB( "editor", "auto_reload_on_disk_change", editor.autoReloadOnDiskChange );

	ini.setValue( "editor", "wrap_mode", DocumentView::fromLineWrapMode( editor.wrapMode ) );
	ini.setValue( "editor", "wrap_type", DocumentView::fromLineWrapType( editor.wrapType ) );
	ini.setValueB( "editor", "wrap_keep_indentation", editor.wrapKeepIndentation );

	ini.setValueB( "editor", "code_folding_enabled", editor.codeFoldingEnabled );
	ini.setValueB( "editor", "code_folding_always_visible", editor.codeFoldingAlwaysVisible );
	ini.setValue( "editor", "code_folding_refresh_frequency",
				  editor.codeFoldingRefreshFreq.toString() );

	ini.setValueB( "search_bar", "case_sensitive", searchBarConfig.caseSensitive );
	ini.setValueB( "search_bar", "lua_pattern", searchBarConfig.luaPattern );
	ini.setValueB( "search_bar", "whole_word", searchBarConfig.wholeWord );
	ini.setValueB( "search_bar", "escape_sequence", searchBarConfig.escapeSequence );

	ini.setValueB( "global_search_bar", "case_sensitive", globalSearchBarConfig.caseSensitive );
	ini.setValueB( "global_search_bar", "lua_pattern", globalSearchBarConfig.luaPattern );
	ini.setValueB( "global_search_bar", "whole_word", globalSearchBarConfig.wholeWord );
	ini.setValueB( "global_search_bar", "escape_sequence", globalSearchBarConfig.escapeSequence );

	ini.setValue( "terminal", "shell", term.shell );
	ini.setValue( "terminal", "font_size", term.fontSize.toString() );
	ini.setValue( "terminal", "colorscheme", term.colorScheme );
	ini.setValue( "terminal", "new_terminal_orientation",
				  NewTerminalOrientation::toString( term.newTerminalOrientation ) );
	ini.setValue( "terminal", "scrollback", String::toString( term.scrollback ) );
	ini.setValueB( "terminal", "unsupported_os_warn_disabled", term.unsupportedOSWarnDisabled );

	ini.setValueB( "window", "vsync", context.VSync );
	ini.setValue( "window", "glversion",
				  Renderer::graphicsLibraryVersionToString( context.Version ) );
	ini.setValueI( "window", "multisamples", context.Multisamples );
	ini.setValueI( "window", "frameratelimit", context.FrameRateLimit );

	ini.setValueB( "workspace", "restore_last_session", workspace.restoreLastSession );
	ini.setValueB( "workspace", "check_for_updates_at_startup",
				   workspace.checkForUpdatesAtStartup );

	const auto& pluginsEnabled = pluginManager->getPluginsEnabled();
	for ( const auto& plugin : pluginsEnabled )
		ini.setValueB( "plugins", plugin.first, plugin.second );

	for ( const auto& langExt : languagesExtensions.priorities )
		ini.setValue( "languages_extensions", langExt.first, langExt.second );

	ini.writeFile();
	iniState.writeFile();
}

struct ProjectPath {
	std::string path;
	TextRange selection{ { 0, 0 }, { 0, 0 } };
	ProjectPath() {}
	ProjectPath( const std::string& path, const TextRange& selection ) :
		path( path ), selection( selection ) {}

	std::string toString() { return URI::encode( path ) + ";" + selection.toString(); }

	static ProjectPath fromString( const std::string& str ) {
		auto split = String::split( str, ';' );
		if ( !split.empty() ) {
			ProjectPath pp;
			pp.path = URI::decode( split[0] );
			pp.selection = split.size() >= 2
							   ? TextRange::fromString( split[1] )
							   : TextRange( TextPosition( 0, 0 ), TextPosition( 0, 0 ) );
			return pp;
		}
		return {};
	}
};

json saveNode( Node* node ) {
	json res;
	if ( node->isType( UI_TYPE_SPLITTER ) ) {
		UISplitter* splitter = node->asType<UISplitter>();
		res["type"] = "splitter";
		res["split"] = splitter->getSplitPartition().toString();
		res["orientation"] =
			splitter->getOrientation() == UIOrientation::Horizontal ? "horizontal" : "vertical";
		res["first"] = saveNode( splitter->getFirstWidget() );
		res["last"] = saveNode( splitter->getLastWidget() );
	} else if ( node->isType( UI_TYPE_TABWIDGET ) ) {
		UITabWidget* tabWidget = node->asType<UITabWidget>();
		std::vector<json> files;
		for ( size_t i = 0; i < tabWidget->getTabCount(); ++i ) {
			Node* ownedWidget = tabWidget->getTab( i )->getOwnedWidget();
			if ( !ownedWidget )
				continue;
			if ( ownedWidget->isType( UI_TYPE_CODEEDITOR ) ) {
				UICodeEditor* editor = ownedWidget->asType<UICodeEditor>();
				if ( !editor->getDocument().getFilePath().empty() ) {
					json f;
					f["type"] = "editor";
					f["path"] = editor->getDocument().getFilePath();
					f["selection"] = editor->getDocument().getSelections().toString();
					files.emplace_back( f );
				}
			} else if ( ownedWidget->isType( UI_TYPE_TERMINAL ) ) {
				UITerminal* term = ownedWidget->asType<UITerminal>();
				json f;
				f["type"] = "terminal";
				if ( term->isUsingCustomTitle() )
					f["title"] = term->getTitle();
				files.emplace_back( f );
			}
		}
		res["type"] = "tabwidget";
		res["files"] = files;
		res["current_page"] = tabWidget->getTabSelectedIndex();
	}
	return res;
}

void AppConfig::saveProject( std::string projectFolder, UICodeEditorSplitter* editorSplitter,
							 const std::string& configPath, const ProjectDocumentConfig& docConfig,
							 const ProjectBuildConfiguration& buildConfig ) {
	FileSystem::dirAddSlashAtEnd( projectFolder );
	std::string projectsPath( configPath + "projects" + FileSystem::getOSSlash() );
	if ( !FileSystem::fileExists( projectsPath ) )
		FileSystem::makeDir( projectsPath );
	MD5::Result hash = MD5::fromString( projectFolder );
	std::string projectCfgPath( projectsPath + hash.toHexString() + ".cfg" );
	IniFile cfg( projectCfgPath, false );
	cfg.setValue( "path", "folder_path", projectFolder );
	cfg.setValueB( "document", "use_global_settings", docConfig.useGlobalSettings );
	cfg.setValueB( "document", "h_as_cpp", docConfig.hAsCPP );
	cfg.setValueB( "document", "trim_trailing_whitespaces", docConfig.doc.trimTrailingWhitespaces );
	cfg.setValueB( "document", "force_new_line_at_end_of_file",
				   docConfig.doc.forceNewLineAtEndOfFile );
	cfg.setValueB( "document", "auto_detect_indent_type", docConfig.doc.autoDetectIndentType );
	cfg.setValueB( "document", "write_bom", docConfig.doc.writeUnicodeBOM );
	cfg.setValueI( "document", "indent_width", docConfig.doc.indentWidth );
	cfg.setValueB( "document", "indent_spaces", docConfig.doc.indentSpaces );
	cfg.setValue( "document", "line_endings",
				  TextFormat::lineEndingToString( docConfig.doc.lineEndings ) );
	cfg.setValueI( "document", "tab_width", docConfig.doc.tabWidth );
	cfg.setValueI( "document", "line_breaking_column", docConfig.doc.lineBreakingColumn );
	cfg.setValue( "build", "build_name", buildConfig.buildName );
	cfg.setValue( "build", "build_type", buildConfig.buildType );
	cfg.setValue( "build", "run_name", buildConfig.runName );
	cfg.setValue( "nodes", "documents",
				  saveNode( editorSplitter->getBaseLayout()->getFirstChild() ).dump() );
	cfg.deleteKey( "files" );
	cfg.writeFile();
}

static void countTotalEditors( json j, size_t& curTotal ) {
	if ( j["type"] == "tabwidget" ) {
		for ( const auto& file : j["files"] )
			if ( !file.contains( "type" ) || file["type"] == "editor" )
				curTotal++;
	} else if ( j["type"] == "splitter" ) {
		countTotalEditors( j["first"], curTotal );
		countTotalEditors( j["last"], curTotal );
	}
}

static int countTotalEditors( json j ) {
	size_t total = 0;
	countTotalEditors( j, total );
	return total;
}

void AppConfig::editorLoadedCounter( ecode::App* app ) {
	editorsToLoad--;
	if ( editorsToLoad <= 0 ) {
		app->getUISceneNode()->runOnMainThread( [app] {
			if ( !app->getFileToOpen().empty() ) {
				app->loadFileDelayed();
			} else {
				app->getSplitter()->addCurrentPositionToNavigationHistory();
			}
		} );
	}
}

void AppConfig::loadDocuments( UICodeEditorSplitter* editorSplitter, json j,
							   UITabWidget* curTabWidget, ecode::App* app ) {
	if ( j["type"] == "tabwidget" ) {
		Int64 currentPage = j["current_page"];
		size_t totalToLoad = j["files"].size();
		for ( const auto& file : j["files"] ) {
			if ( !file.contains( "type" ) || file["type"] == "editor" ) {
				std::string path( file["path"] );
				if ( !FileSystem::fileExists( path ) ) {
					editorLoadedCounter( app );
					return;
				}
				TextRanges selection( TextRanges::fromString( file["selection"] ) );
				UITab* tab = nullptr;
				if ( ( tab = editorSplitter->isDocumentOpen( path, false, true ) ) != nullptr ) {
					auto tabAndEditor = editorSplitter->createCodeEditorInTabWidget( curTabWidget );
					UICodeEditor* teditor = tabAndEditor.second;
					teditor->setDocument(
						tab->getOwnedWidget()->asType<UICodeEditor>()->getDocumentRef() );
					editorSplitter->removeUnusedTab( curTabWidget );
					if ( !teditor->getDocument().getSelection().isValid() ||
						 teditor->getDocument().getSelection() ==
							 TextRange( { 0, 0 }, { 0, 0 } ) ) {
						teditor->getDocument().setSelection( selection );
						teditor->scrollToCursor();
					}
					if ( curTabWidget->getTabCount() == totalToLoad )
						curTabWidget->setTabSelected(
							eeclamp<Int32>( currentPage, 0, curTabWidget->getTabCount() - 1 ) );

					editorLoadedCounter( app );
				} else {
					editorSplitter->loadAsyncFileFromPathInNewTab(
						path,
						[this, curTabWidget, selection, totalToLoad, currentPage,
						 app]( UICodeEditor* editor, const std::string& ) {
							if ( !editor->getDocument().getSelection().isValid() ||
								 editor->getDocument().getSelection() ==
									 TextRange( { 0, 0 }, { 0, 0 } ) ) {
								editor->getDocument().setSelection( selection );
								editor->scrollToCursor();
							}
							if ( curTabWidget->getTabCount() == totalToLoad )
								curTabWidget->setTabSelected( eeclamp<Int32>(
									currentPage, 0, curTabWidget->getTabCount() - 1 ) );

							editorLoadedCounter( app );
						},
						curTabWidget );
				}
			} else if ( file["type"] == "terminal" ) {
				app->getTerminalManager()->createNewTerminal(
					file.contains( "title" ) ? file["title"] : "", curTabWidget );

				if ( curTabWidget->getTabCount() == totalToLoad )
					curTabWidget->setTabSelected(
						eeclamp<Int32>( currentPage, 0, curTabWidget->getTabCount() - 1 ) );
			}
		}
	} else if ( j["type"] == "splitter" ) {
		UISplitter* splitter = editorSplitter->split(
			j["orientation"] == "horizontal" ? UICodeEditorSplitter::SplitDirection::Right
											 : UICodeEditorSplitter::SplitDirection::Bottom,
			curTabWidget->getTabSelected()->getOwnedWidget()->asType<UICodeEditor>(), false );

		if ( nullptr == splitter )
			return;

		loadDocuments( editorSplitter, j["first"], curTabWidget, app );
		UITabWidget* tabWidget = splitter->getLastWidget()->asType<UITabWidget>();
		loadDocuments( editorSplitter, j["last"], tabWidget, app );

		splitter->setSplitPartition( StyleSheetLength( j["split"] ) );
	}
}

void AppConfig::loadProject( std::string projectFolder, UICodeEditorSplitter* editorSplitter,
							 const std::string& configPath, ProjectDocumentConfig& docConfig,
							 ecode::App* app ) {
	FileSystem::dirAddSlashAtEnd( projectFolder );
	std::string projectsPath( configPath + "projects" + FileSystem::getOSSlash() );
	MD5::Result hash = MD5::fromString( projectFolder );
	std::string projectCfgPath( projectsPath + hash.toHexString() + ".cfg" );
	if ( !FileSystem::fileExists( projectCfgPath ) )
		return;
	IniFile cfg( projectCfgPath );

	docConfig.useGlobalSettings = cfg.getValueB( "document", "use_global_settings", true );
	docConfig.hAsCPP = cfg.getValueB( "document", "h_as_cpp", false );
	docConfig.doc.trimTrailingWhitespaces =
		cfg.getValueB( "document", "trim_trailing_whitespaces", false );
	docConfig.doc.forceNewLineAtEndOfFile =
		cfg.getValueB( "document", "force_new_line_at_end_of_file", false );
	docConfig.doc.autoDetectIndentType =
		cfg.getValueB( "document", "auto_detect_indent_type", true );
	docConfig.doc.writeUnicodeBOM = cfg.getValueB( "document", "write_bom", false );
	docConfig.doc.indentWidth = cfg.getValueI( "document", "indent_width", 4 );
	docConfig.doc.indentSpaces = cfg.getValueB( "document", "indent_spaces", false );
	docConfig.doc.lineEndings =
		TextFormat::stringToLineEnding( cfg.getValue( "document", "line_endings", "LF" ) );

	docConfig.doc.tabWidth = eemax( 2, cfg.getValueI( "document", "tab_width", 4 ) );
	docConfig.doc.lineBreakingColumn =
		eemax( 0, cfg.getValueI( "document", "line_breaking_column", 100 ) );

	if ( app->getProjectBuildManager() ) {
		ProjectBuildConfiguration prjCfg;
		prjCfg.buildName = cfg.getValue( "build", "build_name", "" );
		prjCfg.buildType = cfg.getValue( "build", "build_type", "" );
		prjCfg.runName = cfg.getValue( "build", "run_name", "" );
		app->getProjectBuildManager()->setConfig( prjCfg );
	}

	if ( cfg.keyValueExists( "nodes", "documents" ) ) {
		json j;
		try {
			j = json::parse( cfg.getValue( "nodes", "documents" ) );
		} catch ( const json::exception& e ) {
			Log::error( "AppConfig::loadProject: error loading project: %s", e.what() );
			return;
		}
		if ( j.is_discarded() )
			return;

		editorsToLoad = countTotalEditors( j );
		UITabWidget* curTabWidget =
			editorSplitter->tabWidgetFromWidget( editorSplitter->getCurWidget() );
		loadDocuments( editorSplitter, j, curTabWidget, app );
	}
}

bool AppConfig::isNewVersion() const {
	return windowState.lastRunVersion != ecode::Version::getVersionNum();
}

} // namespace ecode
