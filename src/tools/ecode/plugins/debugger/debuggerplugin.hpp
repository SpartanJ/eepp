#pragma once

#include "../plugin.hpp"
#include "../pluginmanager.hpp"
#include "debuggerclientlistener.hpp"

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
	DapRunConfig run;
	std::vector<DapConfig> configurations;
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

	std::vector<DapTool> mDaps;
	std::unique_ptr<DebuggerClient> mDebugger;
	std::unique_ptr<DebuggerClientListener> mListener;

	// Sidepanel
	UITabWidget* mSidePanel{ nullptr };
	UITab* mTab{ nullptr };
	UIWidget* mTabContents{ nullptr };
	UIDropDownList* mUIDebuggerList{ nullptr };
	UIDropDownList* mUIDebuggerConfList{ nullptr };
	UIPushButton* mRunButton{ nullptr };

	DebuggerPlugin( PluginManager* pluginManager, bool sync );

	void load( PluginManager* pluginManager );

	PluginRequestHandle processMessage( const PluginMessage& msg );

	void updateUI();

	void buildSidePanelTab();

	void updateSidePanelTab();

	void updateDebuggerConfigurationList();

	void hideSidePanel();

	void loadDAPConfig( const std::string& path, bool updateConfigFile );

	void runConfig( const std::string& debugger, const std::string& configuration );

	void exitDebugger();

	void replaceKeysInJson( nlohmann::json& json );
};

} // namespace ecode
