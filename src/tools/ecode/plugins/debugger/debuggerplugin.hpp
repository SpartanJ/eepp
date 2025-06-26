#pragma once

#include "../../projectbuild.hpp"
#include "../plugin.hpp"
#include "../pluginmanager.hpp"
#include "config.hpp"
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
	std::string request;
	std::vector<std::string> cmdArgs;
	nlohmann::json args;
	bool runTarget{ false };
};

struct DapTool {
	std::string name;
	std::string url;
	std::vector<std::string> type;
	std::vector<std::string> languagesSupported;
	DapRunConfig run;
	std::vector<DapConfig> configurations;
	std::unordered_map<std::string, std::string> findBinary;
	std::string fallbackCommand;
	bool redirectStdout{ false };
	bool redirectStderr{ false };
	bool supportsSourceRequest{ false };
};

struct DapConfigurationInput {
	std::string id;
	std::string description;
	std::string type;
	std::string defaultValue;
	std::vector<std::string> options;
};

class DebuggerPlugin : public PluginBase {
  public:
	static PluginDefinition Definition() {
		return { "debugger",		  "Debugger",  "Debugger integration",
				 DebuggerPlugin::New, { 0, 0, 3 }, DebuggerPlugin::NewSync };
	}

	static Plugin* New( PluginManager* pluginManager );

	static Plugin* NewSync( PluginManager* pluginManager );

	virtual ~DebuggerPlugin();

	std::string getId() override { return Definition().id; }

	std::string getTitle() override { return Definition().name; }

	std::string getDescription() override { return Definition().description; }

	void onSaveProject( const std::string& projectFolder, const std::string& projectStatePath,
						bool rewriteStateOnlyIfNeeded ) override;

	void onLoadProject( const std::string& projectFolder,
						const std::string& projectStatePath ) override;

	std::vector<DapTool> getDebuggersForLang( const std::string& lang );

	std::optional<Command> debuggerBinaryExists( const std::string& debugger,
												 std::optional<DapRunConfig> runConfig = {} );

	void initStatusDebuggerController();

	bool isSilent() const { return mSilence; }

  protected:
	friend class DebuggerClientListener;

	bool mInitialized{ false };
	bool mFetchRegisters{ false };
	bool mFetchGlobals{ false };
	bool mChangingBreakpoint{ false };
	bool mSilence{ true };
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
	UIPushButton* mBuildAndRunButton{ nullptr };
	BreakpointsHolder mBreakpoints;
	UnorderedSet<std::string> mPendingBreakpoints;
	std::shared_ptr<BreakpointsModel> mBreakpointsModel;
	Mutex mDapsMutex;
	Mutex mBreakpointsMutex;
	StatusDebuggerController::State mDebuggingState{ StatusDebuggerController::State::NotStarted };
	std::vector<std::string> mExpressions;
	std::shared_ptr<VariablesHolder> mExpressionsHolder;
	std::shared_ptr<VariablesHolder> mHoverExpressionsHolder;
	UnorderedMap<std::string, DapConfigurationInput> mDapInputs;

	// Begin Hover Stuff
	Uint32 mHoverWaitCb{ 0 };
	TextRange mCurrentHover;
	Time mHoverDelay{ Seconds( 1.f ) };
	UIWindow* mHoverTooltip{ nullptr };
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
	std::string mLastStateJsonDump;
	std::string mCurDebugger;
	std::string mCurConfiguration;

	class DebuggerPluginClient : public TextDocument::Client {
	  public:
		explicit DebuggerPluginClient( DebuggerPlugin* parent, TextDocument* doc ) :
			mDoc( doc ), mParent( parent ) {}

		virtual void onDocumentTextChanged( const DocumentContentChange& ) {}
		virtual void onDocumentUndoRedo( const TextDocument::UndoRedo& ) {}
		virtual void onDocumentCursorChange( const TextPosition& ) {}
		virtual void onDocumentSelectionChange( const TextRange& ) {}
		virtual void onDocumentLineCountChange( const size_t&, const size_t& ) {}
		virtual void onDocumentLineChanged( const Int64& ) {}
		virtual void onDocumentSaved( TextDocument* ) {}
		virtual void onDocumentClosed( TextDocument* doc ) { onDocumentReset( doc ); }
		virtual void onDocumentDirtyOnFileSystem( TextDocument* ) {}
		virtual void onDocumentMoved( TextDocument* ) {}
		virtual void onDocumentReset( TextDocument* ) {}

		virtual void onDocumentLineMove( const Int64& fromLine, const Int64& toLine,
										 const Int64& numLines ) {
			mParent->onDocumentLineMove( mDoc, fromLine, toLine, numLines );
		}

	  protected:
		TextDocument* mDoc{ nullptr };
		DebuggerPlugin* mParent{ nullptr };
	};

	using ClientsMap = std::unordered_map<TextDocument*, std::unique_ptr<DebuggerPluginClient>>;
	ClientsMap mClients;
	Mutex mClientsMutex;

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

	void run( const std::string& debugger, ProtocolSettings&& protocolSettings,
			  DapRunConfig&& runConfig, int randPort, bool forceUseProgram, bool usesPorts );

	void exitDebugger( bool requestDisconnect = false );

	void replaceKeysInJson( nlohmann::json& json, int randomPort,
							const std::unordered_map<std::string, std::string>& solvedInputs );

	std::pair<bool, std::vector<std::string>>
	replaceKeyInString( std::string val, int randomPort,
						const std::unordered_map<std::string, std::string>& solvedInputs );

	void onRegisterEditor( UICodeEditor* ) override;

	void onUnregisterEditor( UICodeEditor* ) override;

	void onRegisterDocument( TextDocument* doc ) override;

	void drawLineNumbersBefore( UICodeEditor* editor, const DocumentLineRange& lineRange,
								const Vector2f& startScroll, const Vector2f& screenStart,
								const Float& lineHeight, const Float& lineNumberWidth,
								const int& lineNumberDigits, const Float& fontSize ) override;

	void drawBeforeLineText( UICodeEditor* editor, const Int64& index, Vector2f position,
							 const Float& /*fontSize*/, const Float& lineHeight ) override;

	bool setBreakpoint( UICodeEditor* editor, Uint32 lineNumber );

	bool setBreakpoint( TextDocument* doc, Uint32 lineNumber );

	bool breakpointToggleEnabled( TextDocument* doc, Uint32 lineNumber );

	bool setBreakpoint( const std::string& doc, Uint32 lineNumber );

	bool breakpointToggleEnabled( const std::string& doc, Uint32 lineNumber );

	bool breakpointSetEnabled( const std::string& doc, Uint32 lineNumber, bool enabled );

	bool hasBreakpoint( const std::string& doc, Uint32 lineNumber );

	bool onMouseDown( UICodeEditor*, const Vector2i&, const Uint32& flags ) override;

	bool isSupportedByAnyDebugger( const std::string& language );

	void runCurrentConfig();

	void sendFileBreakpoints( const std::string& filePath );

	void sendPendingBreakpoints();

	StatusDebuggerController* getStatusDebuggerController() const;

	void setUIDebuggingState( StatusDebuggerController::State state );

	void updatePanelUIState( StatusDebuggerController::State state );

	void displayTooltip( UICodeEditor* editor, const std::string& expression,
						 const EvaluateInfo& info, const Vector2f& position );

	bool onMouseMove( UICodeEditor* editor, const Vector2i& position,
					  const Uint32& flags ) override;

	void loadProjectConfiguration( const std::string& path );

	void loadProjectConfigurations();

	void openExpressionMenu( ModelIndex idx );

	void updateSelectedDebugConfig();

	void removeExpression( const std::string& name );

	void resetExpressions();

	void closeProject();

	std::unordered_map<std::string, DapConfigurationInput>
	needsToResolveInputs( nlohmann::json& json, const std::vector<std::string>& cmdArgs );

	void resolveInputsBeforeRun( std::unordered_map<std::string, DapConfigurationInput> inputs,
								 DapTool debugger, DapConfig config,
								 std::unordered_map<std::string, std::string> solvedInputs = {} );

	void prepareAndRun( DapTool debugger, DapConfig config,
						std::unordered_map<std::string, std::string> solvedInputs );

	UIWindow* processPicker();

	bool resume( int threadId, bool singleThread = false );

	virtual void onUnregisterDocument( TextDocument* doc ) override;

	void onDocumentLineMove( TextDocument* doc, const Int64& fromLine, const Int64& toLine,
							 const Int64& numLines );

	bool replaceInVal( std::string& val, const std::optional<ProjectBuildStep>& runConfig,
					   ProjectBuild* buildConfig, int randomPort );
};

} // namespace ecode
