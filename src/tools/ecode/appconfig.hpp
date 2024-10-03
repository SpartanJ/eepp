#ifndef ECODE_APPCONFIG_HPP
#define ECODE_APPCONFIG_HPP

#include <eepp/config.hpp>
#include <eepp/math/size.hpp>
#include <eepp/system/inifile.hpp>
#include <eepp/ui/css/stylesheetlength.hpp>
#include <eepp/ui/tools/uicodeeditorsplitter.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/window/window.hpp>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

using namespace EE;
using namespace EE::Math;
using namespace EE::UI;
using namespace EE::UI::CSS;
using namespace EE::UI::Tools;
using namespace EE::System;
using namespace EE::Window;

namespace ecode {
class App;
class PluginManager;

enum class PanelPosition { Left, Right };

struct UIConfig {
	StyleSheetLength fontSize{ 11, StyleSheetLength::Dp };
	StyleSheetLength panelFontSize{ 11, StyleSheetLength::Dp };
	bool showSidePanel{ true };
	bool showStatusBar{ true };
	bool showMenuBar{ false };
	bool welcomeScreen{ true };
	bool openFilesInNewWindow{ false };
	PanelPosition panelPosition{ PanelPosition::Left };
	std::string serifFont;
	std::string monospaceFont;
	std::string terminalFont;
	std::string fallbackFont;
	ColorSchemePreference colorScheme{ ColorSchemePreference::Dark };
	std::string theme;
	std::string language;
};

struct WindowStateConfig {
	Float pixelDensity{ 0 };
	Sizei size{ 1280, 720 };
	std::string winIcon;
	bool maximized{ false };
	std::string panelPartition;
	std::string statusBarPartition;
	int displayIndex{ 0 };
	Vector2i position{ -1, -1 };
	Uint32 lastRunVersion{ 0 };
};

struct CodeEditorConfig {
	std::string colorScheme{ "ecode" };
	StyleSheetLength fontSize{ 11, StyleSheetLength::Dp };
	StyleSheetLength lineSpacing{ 0, StyleSheetLength::Dp };
	bool showLineNumbers{ true };
	bool showWhiteSpaces{ true };
	bool showLineEndings{ false };
	bool showIndentationGuides{ false };
	bool highlightMatchingBracket{ true };
	bool verticalScrollbar{ true };
	bool horizontalScrollbar{ true };
	bool highlightCurrentLine{ true };
	bool highlightSelectionMatch{ true };
	bool colorPickerSelection{ false };
	bool colorPreview{ false };
	bool minimap{ true };
	bool showDocInfo{ true };
	bool hideTabBarOnSingleTab{ true };
	bool singleClickNavigation{ false };
	bool syncProjectTreeWithEditor{ true };
	bool autoCloseXMLTags{ true };
	bool linesRelativePosition{ false };
	bool autoReloadOnDiskChange{ false };
	bool codeFoldingEnabled{ true };
	bool codeFoldingAlwaysVisible{ false };
	LineWrapMode wrapMode{ LineWrapMode::NoWrap };
	LineWrapType wrapType{ LineWrapType::Viewport };
	bool wrapKeepIndentation{ true };
	std::string autoCloseBrackets{ "" };
	Time cursorBlinkingTime{ Seconds( 0.5f ) };
	Time codeFoldingRefreshFreq{ Seconds( 2.f ) };
	std::string tabIndentCharacter{ "" };
	CharacterAlignment tabIndentAlignment{ CharacterAlignment::Center };
};

struct DocumentConfig {
	bool trimTrailingWhitespaces{ false };
	bool forceNewLineAtEndOfFile{ false };
	bool autoDetectIndentType{ true };
	bool writeUnicodeBOM{ false };
	bool indentSpaces{ false };
	TextFormat::LineEnding lineEndings{ TextFormat::LineEnding::LF };
	int indentWidth{ 4 };
	int tabWidth{ 4 };
	int lineBreakingColumn{ 100 };
};

struct SearchBarConfig {
	bool caseSensitive{ false };
	bool regex{ false };
	bool luaPattern{ false };
	bool wholeWord{ false };
	bool escapeSequence{ false };
};

struct GlobalSearchBarConfig {
	bool caseSensitive{ false };
	bool regex{ false };
	bool luaPattern{ false };
	bool wholeWord{ false };
	bool escapeSequence{ false };
};

struct ProjectDocumentConfig {
	bool useGlobalSettings{ true };
	bool hAsCPP{ false };
	DocumentConfig doc;
	ProjectDocumentConfig() {}
	ProjectDocumentConfig( const DocumentConfig& doc ) { this->doc = doc; }
};

struct ProjectBuildConfiguration {
	std::string buildName;
	std::string buildType;
	std::string runName;
};

class NewTerminalOrientation {
  public:
	enum Orientation { Same, Vertical, Horizontal };

	static NewTerminalOrientation::Orientation fromString( const std::string& orientation ) {
		if ( "same" == orientation )
			return Orientation::Same;
		if ( "horizontal" == orientation )
			return Orientation::Horizontal;
		return Orientation::Vertical;
	}

	static std::string toString( const Orientation& orientation ) {
		switch ( orientation ) {
			case Orientation::Vertical:
				return "vertical";
			case Orientation::Horizontal:
				return "horizontal";
			case Orientation::Same:
			default:
				return "same";
		}
	}
};

struct TerminalConfig {
	std::string shell;
	std::string colorScheme{ "eterm" };
	StyleSheetLength fontSize{ 11, StyleSheetLength::Dp };
	NewTerminalOrientation::Orientation newTerminalOrientation{
		NewTerminalOrientation::Horizontal };
	size_t scrollback{ 10000 };
	bool unsupportedOSWarnDisabled{ false };
};

struct WorkspaceConfig {
	bool restoreLastSession{ false };
	bool checkForUpdatesAtStartup{ true };
	bool sessionSnapshot{ true };
};

struct LanguagesExtensions {
	std::map<std::string, std::string> priorities;
};

struct SessionSnapshotFile {
	std::string cachePath;
	std::string fspath;
	Uint64 fsmtime{ 0 };
	std::string fshash;
	std::string name;
	std::string selection;
};

class AppConfig {
  public:
	WindowStateConfig windowState;
	ContextSettings context;
	CodeEditorConfig editor;
	DocumentConfig doc;
	TerminalConfig term;
	UIConfig ui;
	IniFile ini;
	IniFile iniState;
	FileInfo iniInfo;
	SearchBarConfig searchBarConfig;
	GlobalSearchBarConfig globalSearchBarConfig;
	WorkspaceConfig workspace;
	LanguagesExtensions languagesExtensions;

	bool isNewVersion() const;

	void load( const std::string& confPath, std::string& keybindingsPath,
			   std::string& initColorScheme, std::vector<std::string>& recentFiles,
			   std::vector<std::string>& recentFolders, const std::string& resPath,
			   PluginManager* pluginManager, const Sizei& displaySize, bool sync );

	void save( const std::vector<std::string>& recentFiles,
			   const std::vector<std::string>& recentFolders, const std::string& panelPartition,
			   const std::string& statusBarPartition, EE::Window::Window* win,
			   const std::string& colorSchemeName, const SearchBarConfig& searchBarConfig,
			   const GlobalSearchBarConfig& globalSearchBarConfig, PluginManager* pluginManager );

	void saveProject( std::string projectFolder, UICodeEditorSplitter* editorSplitter,
					  const std::string& configPath, const ProjectDocumentConfig& docConfig,
					  const ProjectBuildConfiguration& buildConfig, bool onlyIfNeeded,
					  bool sessionSnapshot );

	void loadProject( std::string projectFolder, UICodeEditorSplitter* editorSplitter,
					  const std::string& configPath, ProjectDocumentConfig& docConfig,
					  ecode::App* app, bool sessionSnapshot );

  protected:
	Int64 editorsToLoad{ 0 };

	void loadDocuments( UICodeEditorSplitter* editorSplitter, json j, UITabWidget* curTabWidget,
						ecode::App* app,
						const std::vector<SessionSnapshotFile>& sessionSnapshotFiles );

	void editorLoadedCounter( ecode::App* app );
};

} // namespace ecode

#endif // ECODE_APPCONFIG_HPP
