#pragma once

#include "../plugin.hpp"
#include "../pluginmanager.hpp"
#include "protocol.hpp"

namespace ecode {

class LLMChatUI;

class AIAssistantPlugin : public PluginBase {
  public:
	struct AIAssistantConfig {
		StyleSheetLength partition;
		std::string modelProvider;
		std::string modelName;
		std::string agentName;
	};

	static PluginDefinition Definition() {
		return { "aiassistant",			 "AI Assistant", "Chat with your favorite AI assistant",
				 AIAssistantPlugin::New, { 0, 1, 0 },	 AIAssistantPlugin::NewSync };
	}

	static Plugin* New( PluginManager* pluginManager );

	static Plugin* NewSync( PluginManager* pluginManager );

	virtual ~AIAssistantPlugin();

	static std::optional<std::string> getApiKeyFromProvider( const std::string& provider,
															 AIAssistantPlugin* instance );

	std::string getId() override { return Definition().id; }

	std::string getTitle() override { return Definition().name; }

	std::string getDescription() override { return Definition().description; }

	const LLMProviders& getProviders() { return mProviders; }
	const ACPAgents& getAgents() { return mAgents; }

	std::string getPluginStatePath() const;

	std::string getConversationsPath() const;

	LLMChatUI* newAIAssistant();

	void onSaveState( IniFile* state ) override;

	void setConfig( AIAssistantConfig&& config ) { mConfig = std::move( config ); }

  protected:
	LLMProviders mProviders;
	ACPAgents mAgents;
	bool mUIInit{ false };
	bool mBrokenUserConfigFile{ false };
	UIWidget* mStatusBar{ nullptr };
	UIPushButton* mAIChatButton{ nullptr };
	UnorderedMap<std::string, std::string> mApiKeys;
	AIAssistantConfig mConfig;
	Uint32 mAIChatButtonPosCbId{ 0 };
	std::string mConfigFileError;

	AIAssistantPlugin( PluginManager* pluginManager, bool sync );

	void load( PluginManager* pluginManager );

	PluginRequestHandle processMessage( const PluginMessage& msg );

	void loadAIAssistantConfig( const std::string& path, bool updateConfigFile );

	void onRegisterEditor( UICodeEditor* editor ) override;

	void onUnregisterEditor( UICodeEditor* editor ) override;

	void onRegisterDocument( TextDocument* doc ) override;

	void initUI();

	void displayBrokenUserConfigFileWarning();
};

} // namespace ecode
