#pragma once

#include "appconfig.hpp"
#include <args/args.hxx>
#include <eepp/core/small_vector.hpp>
#include <eepp/ee.hpp>
#include <eepp/ui/iconmanager.hpp>
#include <eepp/ui/tools/uifontpickerdialog.hpp>
#include <eepp/ui/tools/uisettingspanel.hpp>
#include <eepp/ui/tools/uitabwidgetsplitter.hpp>
#include <eepp/ui/tools/uiwidgetinspector.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uimessagebox.hpp>
#include <eterm/ui/uiterminal.hpp>

using namespace EE;
using namespace EE::Graphics;
using namespace EE::Scene;
using namespace EE::System;
using namespace EE::UI;
using namespace EE::UI::Tools;
using namespace EE::Window;
using namespace eterm::Terminal;
using namespace eterm::UI;
using namespace eterm;

namespace eterm {

struct TerminalLaunchConfig {
	std::string program;
	std::vector<std::string> arguments;
	std::string workingDirectory;
	std::string executeInShell;
	size_t historySize{ 10000 };
	TerminalCursorMode cursorStyle{ TerminalCursorMode::SteadyUnderline };
	FontHinting fontHinting{ FontHinting::Full };
	FontAntialiasing fontAntialiasing{ FontAntialiasing::Grayscale };
	bool useFrameBuffer{ false };
	bool keepAlive{ true };
	bool closeOnExit{ false };
};

class App {
  public:
	int run( int argc, char* argv[] );

	class TerminalSplitterClient : public UITabWidgetSplitter::Client {
	  public:
		explicit TerminalSplitterClient( App& app ) : mApp( app ) {}

		void onTabCreated( UITab* tab, UIWidget* ) override;
		void onWidgetFocusChange( UIWidget* ) override;

	  private:
		App& mApp;
	};

  private:
	friend struct SettingsPanel;

	std::string getResourcePath() const;

	String i18n( const std::string& key, const String& defaultValue ) const;

	void loadColorSchemes( const std::string& resPath );

	static UITerminal* terminalFromTab( UITab* tab );

	void updateWindowTitle();

	bool hasTerminals() const;

	static bool hasRunningChildren( UITab* tab );

	void closeTab( UITab* tab );

	void queueExitCloseTab( UITab* tab );

	void queueExitedTabs();

	void requestCloseTab( UITab* tab );

	void renameSession( UITerminal* terminal );

	void maximizeTabWidget( UITabWidget* tabWidget );

	void restoreMaximizedTabWidget();

	void configureTab( UITab* tab );

	UITerminal* createTerminal( UITabWidget* target = nullptr );

	UITerminal* createTerminalSplit( SplitDirection direction, UITerminal* terminal );

	void addTabKeyBindings( UITerminal* terminal );

	void showSettings();

	void openFontPicker( bool uiFont, bool fallbackFont = false );

	void forEachTerminal( const std::function<void( UITerminal* )>& fn );

	void savePreferences();

	void saveWindowState();

	void createNewTerminal();

	bool closeWindow( EE::Window::Window* );

	EE::Window::Window* appWindow{ nullptr };
	UISceneNode* scene{ nullptr };
	UILinearLayout* mainLayout{ nullptr };
	UITabWidgetSplitter* tabSplitter{ nullptr };
	FontTrueType* terminalFont{ nullptr };
	UIIcon* terminalIcon{ nullptr };
	UIMessageBox* closeDialog{ nullptr };
	UIWindow* settingsWindow{ nullptr };
	UIWidget* closeDialogWidget{ nullptr };
	UIWindow* maximizedTabWidgetWindow{ nullptr };
	UITabWidget* maximizedTabWidget{ nullptr };
	UINodeLink* maximizedTabWidgetLink{ nullptr };
	TerminalLaunchConfig terminalConfig;
	std::unique_ptr<eterm::AppConfig> config;
	std::map<std::string, TerminalColorScheme> terminalColorSchemes;
	const TerminalColorScheme* selectedColorScheme{ nullptr };
	Float terminalFontSize{ 12 };
	bool warnBeforeClose{ false };
	bool closeApproved{ false };
	bool benchmarkMode{ false };
	Clock secondsCounter;
	SmallVector<UITab*, 8> pendingExitCloseTabs;
	TerminalSplitterClient splitterClient{ *this };
};

} // namespace eterm
