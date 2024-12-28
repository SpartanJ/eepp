#pragma once

#include "../plugin.hpp"
#include "../pluginmanager.hpp"

using namespace EE::UI::Models;
using namespace EE::UI;

namespace ecode {

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
	DebuggerPlugin( PluginManager* pluginManager, bool sync );

	void load( PluginManager* pluginManager );

	PluginRequestHandle processMessage( const PluginMessage& msg );

};

} // namespace ecode
