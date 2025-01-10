#pragma once

#include "../plugin.hpp"
#include "../pluginmanager.hpp"
#include "debuggerclientlistener.hpp"
#include "models/breakpointsmodel.hpp"

using namespace EE::UI::Models;
using namespace EE::UI;

namespace ecode {

struct DapRunConfig {
	std::string command;
	std::vector<std::string> args;
};

struct DapConfig {
	std::string name;
	std::string command;
	nlohmann::json args;
};

struct DapTool {
	std::string name;
	std::string url;
	std::string type;
	std::vector<std::string> languagesSupported;
	DapRunConfig run;
	std::vector<DapConfig> configurations;
	std::unordered_map<std::string, std::string> findBinary;
	std::string fallbackCommand;
};

class DebuggerPlugin : public PluginBase {
  public:
	static PluginDefinition Definition() {
		return { "debugger",		  "Debugger",  "Debugger integration",
				 DebuggerPlugin::New, { 0, 0, 1 }, DebuggerPlugin::NewSync };
	}

	static Plugin* New( PluginManager* pluginManager );

	static Plugin* NewSync( PluginManager* pluginManager );

	virtual ~DebuggerPlugin();

	std::string getId() override { return Definition().id; }

	std::string getTitle() override { return Definition().name; }

	std::string getDescription() override { return Definition().description; }

  protected:
	friend class DebuggerClientListener;

	bool mInitialized{ false };
	std::string mProjectPath;

	std::vector<DapTool> mDaps;
	std::vector<DapConfig> mDapConfigs;
	std::unique_ptr<DebuggerClient> mDebugger;
	std::unique_ptr<DebuggerClientListener> mListener;

	// Sidepanel
	UITabWidget* mSidePanel{ nullptr };
	UITab* mTab{ nullptr };
	UIWidget* mTabContents{ nullptr };
	UIDropDownList* mUIDebuggerList{ nullptr };
	UIDropDownList* mUIDebuggerConfList{ nullptr };
	UIPushButton* mRunButton{ nullptr };
	UnorderedMap<std::string, UnorderedSet<SourceBreakpointStateful>> mBreakpoints;
	UnorderedSet<std::string> mPendingBreakpoints;
	std::shared_ptr<BreakpointsModel> mBreakpointsModel;
	Mutex mDapsMutex;
	Mutex mBreakpointsMutex;
	StatusDebuggerController::State mDebuggingState{ StatusDebuggerController::State::NotStarted };

	// Begin Hover Stuff
	Uint32 mHoverWaitCb{ 0 };
	TextRange mCurrentHover;
	Time mHoverDelay{ Seconds( 1.f ) };
	bool mOldDontAutoHideOnMouseMove{ false };
	bool mOldUsingCustomStyling{ false };
	bool mOldWordWrap{ false };
	Uint32 mOldTextStyle{ 0 };
	Uint32 mOldTextAlign{ 0 };
	Color mOldBackgroundColor;
	std::string mOldMaxWidth;
	// End hover stuff

	struct PanelBoxButtons {
		UILinearLayout* box{ nullptr };
		UIPushButton* resume{ nullptr };
		UIPushButton* pause{ nullptr };
		UIPushButton* stepOver{ nullptr };
		UIPushButton* stepInto{ nullptr };
		UIPushButton* stepOut{ nullptr };
	};
	PanelBoxButtons mPanelBoxButtons;

	DebuggerPlugin( PluginManager* pluginManager, bool sync );

	void load( PluginManager* pluginManager );

	PluginRequestHandle processMessage( const PluginMessage& msg );

	void updateUI();

	void buildSidePanelTab();

	void buildStatusBar();

	void updateSidePanelTab();

	void updateDebuggerConfigurationList();

	void hideSidePanel();

	void hideStatusBarElement();

	void loadDAPConfig( const std::string& path, bool updateConfigFile );

	void runConfig( const std::string& debugger, const std::string& configuration );

	void exitDebugger();

	void replaceKeysInJson( nlohmann::json& json );

	void onRegisterEditor( UICodeEditor* ) override;

	void onUnregisterEditor( UICodeEditor* ) override;

	void onRegisterDocument( TextDocument* doc ) override;

	void drawLineNumbersBefore( UICodeEditor* editor, const DocumentLineRange& lineRange,
								const Vector2f& startScroll, const Vector2f& screenStart,
								const Float& lineHeight, const Float& lineNumberWidth,
								const int& lineNumberDigits, const Float& fontSize ) override;

	bool setBreakpoint( UICodeEditor* editor, Uint32 lineNumber );

	bool setBreakpoint( TextDocument* doc, Uint32 lineNumber );

	bool breakpointToggleEnabled( TextDocument* doc, Uint32 lineNumber );

	bool setBreakpoint( const std::string& doc, Uint32 lineNumber );

	bool breakpointToggleEnabled( const std::string& doc, Uint32 lineNumber );

	bool breakpointSetEnabled( const std::string& doc, Uint32 lineNumber, bool enabled );

	bool onMouseDown( UICodeEditor*, const Vector2i&, const Uint32& flags ) override;

	bool isSupportedByAnyDebugger( const std::string& language );

	void runCurrentConfig();

	void sendFileBreakpoints( const std::string& filePath );

	void sendPendingBreakpoints();

	StatusDebuggerController* getStatusDebuggerController() const;

	void setUIDebuggingState( StatusDebuggerController::State state );

	void updatePanelUIState( StatusDebuggerController::State state );

	void hideTooltip( UICodeEditor* editor );

	void displayTooltip( UICodeEditor* editor, const EvaluateInfo& resp, const Vector2f& position );

	void tryHideTooltip( UICodeEditor* editor, const Vector2i& position );

	bool onMouseMove( UICodeEditor* editor, const Vector2i& position,
					  const Uint32& flags ) override;

	void loadProjectConfiguration( const std::string& path );

	void loadProjectConfigurations();
};

} // namespace ecode
