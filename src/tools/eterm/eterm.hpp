#pragma once

#include "appconfig.hpp"
#include "harnesssessionrecovery.hpp"
#include "programstatusalerts.hpp"
#include "settingsactions.hpp"
#include "uimainlayout.hpp"
#include <args/args.hxx>
#include <atomic>
#include <eepp/core/small_vector.hpp>
#include <eepp/ee.hpp>
#include <eepp/ui/iconmanager.hpp>
#include <eepp/ui/tools/keybindingsconfig.hpp>
#include <eepp/ui/tools/uisettingspanel.hpp>
#include <eepp/ui/tools/uitabwidgetsplitter.hpp>
#include <eepp/ui/tools/uiwidgetinspector.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uimessagebox.hpp>
#include <efsw/efsw.hpp>
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
	bool closeOnExit{ true };
	bool launchExecutable{ false };
};

class App : private efsw::FileWatchListener {
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
	friend class SettingsActions;

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

	/** Shows the terminal's program status in its tab, leaving the title and the terminal alone. */
	void updateTabProgramStatus( UITab* tab );

	/** Only the terminal the user is using can have its status seen: not a visible split, and not
	 * one in a window that lost the keyboard to another application. */
	bool isTerminalActive( const UITerminal* terminal ) const;

	void alertProgramStatus( UITab* tab );

	void updateWidgetProgramStatus( UIWidget* widget );

	String describeProgramStatus( const PresentedProgramStatus& status ) const;

	/** Where the terminal's shell reported to be, else where it started. Empty if neither of them
	 * can be used as a working directory. */
	static std::string terminalDirectory( UITerminal* terminal );

	std::string resolveWorkingDirectory( UITerminal* source ) const;

	/** Builds a configured terminal that isn't in any tab yet: see configureTab(). */
	UITerminal* newTerminal( const Sizef& initialSize, const std::string& workingDirectory,
							 const nlohmann::json* resume = nullptr );

	UITerminal* createTerminal( UITabWidget* target = nullptr, std::string workingDirectory = {} );

	UITerminal* createTerminalSplit( SplitDirection direction, UIWidget* widget );

	void moveTab( UIWidget* widget, int offset );

	template <typename T> void registerTabCommands( T& commandTarget, UIWidget* widget ) {
		tabSplitter->registerSplitterCommands( commandTarget );
		commandTarget.setCommand( "create-new-terminal", [this] { createNewTerminal(); } );
		commandTarget.setCommand( "open-settings", [this] { settingsActions->showSettings(); } );
		commandTarget.setCommand( "open-keybindings", [this] {
			settingsActions->showSettings( "general.keybindings" );
		} );
		commandTarget.setCommand( "open-keybindings-file", [this] { openKeybindingsFile(); } );
		commandTarget.setCommand( "debug-widget-tree-view",
								  [this] { UIWidgetInspector::create( scene ); } );
		commandTarget.setCommand( "move-tab-left", [this, widget] { moveTab( widget, -1 ); } );
		commandTarget.setCommand( "move-tab-right", [this, widget] { moveTab( widget, 1 ); } );
		commandTarget.setCommand( "split-right", [this, widget] {
			createTerminalSplit( SplitDirection::Right, widget );
		} );
		commandTarget.setCommand( "split-bottom", [this, widget] {
			createTerminalSplit( SplitDirection::Bottom, widget );
		} );
		commandTarget.setCommand(
			"split-left", [this, widget] { createTerminalSplit( SplitDirection::Left, widget ); } );
		commandTarget.setCommand(
			"split-top", [this, widget] { createTerminalSplit( SplitDirection::Top, widget ); } );
	}

	void addTabKeyBindings( UITerminal* terminal );

#if EE_PLATFORM == EE_PLATFORM_MACOS
	void createGlobalMenuBar();

	void executeMenuCommand( const std::string& command );

	void syncGlobalMenuKeybindings();
#endif

	KeyBindings::ShortcutMap getDefaultKeybindings() const;

	bool loadKeybindings( bool defaultsOnly = false, std::string* error = nullptr );

	bool reloadKeybindings( std::string* error = nullptr );

	UIWidget* createKeybindingsEditor( UIWidget* parent );

	void applyKeybindings( UITerminal* terminal );

	void applyKeybindings();

	void applyKeybindings( UICodeEditor* editor );

	void openKeybindingsFile();

	void handleFileAction( efsw::WatchID, const std::string& dir, const std::string& filename,
						   efsw::Action action, const std::string& oldFilename ) override;

	void forEachTerminal( const std::function<void( UITerminal* )>& fn );

	void savePreferences();

	void saveWindowState();

	void registerSessionWidgetType();

	void saveSession();

	/** @return Whether the session left terminals open: otherwise the initial tabs are needed. */
	bool restoreSession();

	void createNewTerminal();

	bool closeWindow( EE::Window::Window* );

	EE::Window::Window* appWindow{ nullptr };
	UIApplication* application{ nullptr };
	UISceneNode* scene{ nullptr };
	UIMainLayout* mainLayout{ nullptr };
#if EE_PLATFORM == EE_PLATFORM_MACOS
	UIMenuBar* menuBar{ nullptr };
#endif
	UITabWidgetSplitter* tabSplitter{ nullptr };
	FontTrueType* terminalFont{ nullptr };
	UIIcon* terminalIcon{ nullptr };
	UIMessageBox* closeDialog{ nullptr };
	UICodeEditor* keybindingsFileEditor{ nullptr };
	UIWidget* closeDialogWidget{ nullptr };
	UIWindow* maximizedTabWidgetWindow{ nullptr };
	UITabWidget* maximizedTabWidget{ nullptr };
	UINodeLink* maximizedTabWidgetLink{ nullptr };
	TerminalLaunchConfig terminalConfig;
	std::unique_ptr<eterm::AppConfig> config;
	std::unique_ptr<SettingsActions> settingsActions;
	std::map<std::string, TerminalColorScheme> terminalColorSchemes;
	KeybindingsConfig::Section keybindings;
	std::string keybindingsPath;
	std::unique_ptr<efsw::FileWatcher> fileWatcher;
	std::atomic<bool> keybindingsChanged{ false };
	const TerminalColorScheme* selectedColorScheme{ nullptr };
	Float terminalFontSize{ 12 };
	bool warnBeforeClose{ false };
	bool closeApproved{ false };
	bool benchmarkMode{ false };
	/** A launch for a directory, a program or a tab count neither resumes nor replaces the
	 * session. */
	bool managesSession{ false };
	Clock secondsCounter;
	SmallVector<UITab*, 8> pendingExitCloseTabs;
	/** UI-only memory of the done or error each tab's user has seen: the programs' records stay. */
	UnorderedMap<UITab*, std::shared_ptr<const ProgramStatusSnapshot>> seenProgramStatus;
	UnorderedMap<UITab*, std::shared_ptr<const ProgramStatusSnapshot>> lastAlertStatus;
	ProgramStatusAlerts programStatusAlerts;
	HarnessSessionRecovery harnessRecovery;
	TerminalSplitterClient splitterClient{ *this };
};

} // namespace eterm
