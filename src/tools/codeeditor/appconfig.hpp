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

struct UIConfig {
	StyleSheetLength fontSize{ 12, StyleSheetLength::Dp };
	bool showSidePanel{ true };
	std::string serifFont;
	std::string monospaceFont;
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
	bool horizontalScrollbar{ false };
	bool highlightCurrentLine{ true };
	bool trimTrailingWhitespaces{ false };
	bool forceNewLineAtEndOfFile{ false };
	bool autoDetectIndentType{ true };
	bool writeUnicodeBOM{ false };
	bool indentSpaces{ false };
	bool windowsLineEndings{ false };
	bool highlightSelectionMatch{ true };
	bool colorPickerSelection{ false };
	bool colorPreview{ false };
	bool autoComplete{ true };
	bool showDocInfo{ true };
	bool linter{ true };
	bool formatter{ true };
	bool hideTabBarOnSingleTab{ true };
	bool singleClickTreeNavigation{ false };
	bool syncProjectTreeWithEditor{ true };
	std::string autoCloseBrackets{ "" };
	int indentWidth{ 4 };
	int tabWidth{ 4 };
	int lineBreakingColumn{ 100 };
};

struct AppConfig {
	WindowConfig window;
	CodeEditorConfig editor;
	UIConfig ui;
	IniFile ini;
	IniFile iniState;
	FileInfo iniInfo;

	void load( std::string& confPath, std::string& keybindingsPath, std::string& initColorScheme,
			   std::vector<std::string>& recentFiles, std::vector<std::string>& recentFolders,
			   const std::string& resPath, const Float& displayDPI );

	void save( const std::vector<std::string>& recentFiles,
			   const std::vector<std::string>& recentFolders, const std::string& panelPartition,
			   EE::Window::Window* win, const std::string& colorSchemeName );

	void saveProject( std::string projectFolder, UICodeEditorSplitter* editorSplitter,
					  const std::string& configPath );

	void loadProject( std::string projectFolder, UICodeEditorSplitter* editorSplitter,
					  const std::string& configPath );
};

#endif // APPCONFIG_HPP
