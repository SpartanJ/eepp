#pragma once

#include "../plugin.hpp"
#include "../pluginmanager.hpp"
#include "protocol.hpp"

namespace ecode {

class AIAssistantPlugin : public PluginBase {
  public:
	static PluginDefinition Definition() {
		return { "aiassistant",			 "AI Assistant", "Chat with your favorite AI assistant",
				 AIAssistantPlugin::New, { 0, 0, 1 },	 AIAssistantPlugin::NewSync };
	}

	static Plugin* New( PluginManager* pluginManager );

	static Plugin* NewSync( PluginManager* pluginManager );

	virtual ~AIAssistantPlugin();

	std::string getId() override { return Definition().id; }

	std::string getTitle() override { return Definition().name; }

	std::string getDescription() override { return Definition().description; }

  protected:
	LLMProviders mProviders;

	AIAssistantPlugin( PluginManager* pluginManager, bool sync );

	void load( PluginManager* pluginManager );

	PluginRequestHandle processMessage( const PluginMessage& msg );

	void loadAIAssistantConfig( const std::string& path, bool updateConfigFile );

	void onRegisterEditor( UICodeEditor* editor ) override;

	void onUnregisterEditor( UICodeEditor* editor ) override;

	void onRegisterDocument( TextDocument* doc ) override;
};

} // namespace ecode
