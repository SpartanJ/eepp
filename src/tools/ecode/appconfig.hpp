#ifndef APPCONFIG_HPP
#define APPCONFIG_HPP

#include <eepp/config.hpp>
#include <eepp/math/size.hpp>
#include <eepp/system/inifile.hpp>
#include <eepp/ui/css/stylesheetlength.hpp>
#include <eepp/ui/tools/uicodeeditorsplitter.hpp>
#include <eepp/window/window.hpp>

using namespace EE;
using namespace EE::Math;
using namespace EE::UI;
using namespace EE::UI::CSS;
using namespace EE::UI::Tools;
using namespace EE::System;
using namespace EE::Window;

enum class PanelPosition { Left, Right };

struct UIConfig {
	StyleSheetLength fontSize{ 12, StyleSheetLength::Dp };
	bool showSidePanel{ true };
	PanelPosition panelPosition{ PanelPosition::Left };
	std::string serifFont;
	std::string monospaceFont;
	ColorSchemePreference colorScheme{ ColorSchemePreference::Dark };
};

struct WindowConfig {
	Float pixelDensity{ 0 };
	Sizei size{ 1280, 720 };
	std::string winIcon;
	bool maximized{ false };
	std::string panelPartition;
	int displayIndex{ 0 };
	Vector2i position{ -1, -1 };
};

struct CodeEditorConfig {
	std::string colorScheme{ "lite" };
	StyleSheetLength fontSize{ 12, StyleSheetLength::Dp };
	bool showLineNumbers{ true };
	bool showWhiteSpaces{ true };
	bool highlightMatchingBracket{ true };
	bool verticalScrollbar{ true };
	bool horizontalScrollbar{ true };
	bool highlightCurrentLine{ true };
	bool highlightSelectionMatch{ true };
	bool colorPickerSelection{ false };
	bool colorPreview{ false };
	bool minimap{ true };
	bool autoComplete{ true };
	bool showDocInfo{ true };
	bool linter{ true };
	bool formatter{ true };
	bool hideTabBarOnSingleTab{ true };
	bool singleClickTreeNavigation{ false };
	bool syncProjectTreeWithEditor{ true };
	bool autoCloseXMLTags{ true };
	std::string autoCloseBrackets{ "" };
};

struct DocumentConfig {
	bool trimTrailingWhitespaces{ false };
	bool forceNewLineAtEndOfFile{ false };
	bool autoDetectIndentType{ true };
	bool writeUnicodeBOM{ false };
	bool indentSpaces{ false };
	bool windowsLineEndings{ false };
	int indentWidth{ 4 };
	int tabWidth{ 4 };
	int lineBreakingColumn{ 100 };
};

struct AppConfig {
	WindowConfig window;
	CodeEditorConfig editor;
	DocumentConfig doc;
	UIConfig ui;
	IniFile ini;
	IniFile iniState;
	FileInfo iniInfo;

	void load( const std::string& confPath, std::string& keybindingsPath,
			   std::string& initColorScheme, std::vector<std::string>& recentFiles,
			   std::vector<std::string>& recentFolders, const std::string& resPath,
			   const Float& displayDPI );

	void save( const std::vector<std::string>& recentFiles,
			   const std::vector<std::string>& recentFolders, const std::string& panelPartition,
			   EE::Window::Window* win, const std::string& colorSchemeName );

	void saveProject( std::string projectFolder, UICodeEditorSplitter* editorSplitter,
					  const std::string& configPath );

	void loadProject( std::string projectFolder, UICodeEditorSplitter* editorSplitter,
					  const std::string& configPath, std::shared_ptr<ThreadPool> pool );
};

#endif // APPCONFIG_HPP
