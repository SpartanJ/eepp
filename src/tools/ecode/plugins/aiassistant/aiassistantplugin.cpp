#include "aiassistantplugin.hpp"
#include "chatui.hpp"
#include "protocol.hpp"

#include "../../appconfig.hpp"
#include "../../notificationcenter.hpp"
#include "../../widgetcommandexecuter.hpp"

#include <eepp/system/filesystem.hpp>
#include <eepp/system/scopedop.hpp>

using json = nlohmann::json;

namespace ecode {

static std::initializer_list<std::string> AIAssistantUnlockedCommandList = {

	"new-ai-assistant"

};

static std::initializer_list<std::string> AIAssistantCommandList = {

	"ai-prompt",	   "ai-chat-history", "ai-attach-file",			"ai-link-file",
	"ai-clone-chat",   "ai-settings",	  "ai-toggle-private-chat", "ai-save-chat",
	"ai-rename-chat",  "ai-show-menu",	  "ai-chat-toggle-role",	"ai-refresh-local-models",
	"ai-add-chat",	   "ai-select-model", "ai-toggle-agent-mode",	"ai-agent-config",
	"new-ai-assistant"

};

static std::map<std::string, LLMProvider> parseLLMProviders( const nlohmann::json& j ) {
	std::map<std::string, LLMProvider> providers;
	for ( const auto& item : j.items() ) {
		std::string providerName = item.key();
		const auto& providerJson = item.value();

		LLMProvider provider;
		provider.name = providerName;
		provider.enabled = providerJson.value( "enabled", true );
		provider.openApi = providerJson.value( "open_api", false );

		if ( providerJson.contains( "display_name" ) )
			provider.displayName = providerJson["display_name"].get<std::string>();

		provider.apiUrl = providerJson["api_url"].get<std::string>();

		if ( providerJson.contains( "fetch_models_url" ) ) {
			provider.fetchModelsUrl = providerJson["fetch_models_url"].get<std::string>();
		}

		if ( providerJson.contains( "version" ) ) {
			provider.version = providerJson["version"].get<int>();
		}
		if ( providerJson.contains( "api_key_env_vars" ) &&
			 providerJson["api_key_env_vars"].is_array() ) {
			for ( const auto& env : providerJson["api_key_env_vars"] )
				if ( env.is_string() )
					provider.apiKeyEnvVars.emplace_back( env.get<std::string>() );
		}

		if ( providerJson.contains( "models" ) ) {
			const auto& modelsJson = providerJson["models"];
			for ( const auto& modelJson : modelsJson ) {
				LLMModel model;
				model.name = modelJson["name"].get<std::string>();
				model.provider = providerName;

				// Optional fields for the model
				if ( modelJson.contains( "display_name" ) ) {
					model.displayName = modelJson["display_name"].get<std::string>();
				}

				if ( modelJson.contains( "max_tokens" ) ) {
					model.maxTokens = modelJson["max_tokens"].get<std::size_t>();
				}

				if ( modelJson.contains( "max_output_tokens" ) ) {
					model.maxOutputTokens = modelJson["max_output_tokens"].get<std::size_t>();
				}

				if ( modelJson.contains( "default_temperature" ) ) {
					model.defaultTemperature = modelJson["default_temperature"].get<double>();
				}

				if ( modelJson.contains( "cheapest" ) ) {
					model.cheapest = modelJson.value( "cheapest", false );
				}

				if ( modelJson.contains( "reasoning" ) ) {
					model.reasoning = modelJson.value( "reasoning", false );
				}

				if ( modelJson.contains( "tool_calling" ) ) {
					model.toolCalling = modelJson.value( "tool_calling", false );
				}
				if ( modelJson.contains( "reasoning_options" ) )
					model.reasoningConfiguration = LLMModelCatalog::parseReasoningConfiguration(
						modelJson["reasoning_options"] );

				if ( modelJson.contains( "cache_configuration" ) &&
					 !modelJson["cache_configuration"].is_null() ) {
					const auto& cacheJson = modelJson["cache_configuration"];
					LLMCacheConfiguration cache;
					cache.maxCacheAnchors = cacheJson["max_cache_anchors"].get<int>();
					cache.minTotalToken = cacheJson["min_total_token"].get<int>();
					cache.shouldSpeculate = cacheJson["should_speculate"].get<bool>();
					model.cacheConfiguration = cache;
				}

				model.hash = hashCombine( std::hash<std::string>()( model.name ),
										  std::hash<std::string>()( model.provider ) );

				provider.models.emplace_back( std::move( model ) );
			}
		}

		providers[providerName] = provider;
	}

	return providers;
}

static void mergeLLMProviders( LLMProviders& destination, LLMProviders providers,
							   bool overrideModels ) {
	for ( auto& [key, value] : providers ) {
		auto providerIt = destination.find( key );
		if ( providerIt == destination.end() ) {
			destination.insert( { key, std::move( value ) } );
			continue;
		}
		auto& provider = providerIt->second;
		if ( !value.apiUrl.empty() )
			provider.apiUrl = std::move( value.apiUrl );
		if ( !value.name.empty() )
			provider.name = std::move( value.name );
		if ( value.displayName )
			provider.displayName = std::move( value.displayName );
		if ( value.fetchModelsUrl )
			provider.fetchModelsUrl = std::move( value.fetchModelsUrl );
		if ( !value.apiKeyEnvVars.empty() )
			provider.apiKeyEnvVars = std::move( value.apiKeyEnvVars );
		for ( auto& model : value.models ) {
			auto current = std::find_if( provider.models.begin(), provider.models.end(),
										 [&model]( const LLMModel& candidate ) {
											 return candidate.provider == model.provider &&
													candidate.name == model.name;
										 } );
			if ( current == provider.models.end() )
				provider.models.emplace_back( std::move( model ) );
			else if ( overrideModels )
				*current = std::move( model );
		}
	}
}

static std::map<std::string, ACPAgent> parseACPAgents( const nlohmann::json& j ) {
	std::map<std::string, ACPAgent> agents;
	for ( const auto& item : j.items() ) {
		std::string agentName = item.key();
		const auto& agentJson = item.value();

		ACPAgent agent;
		agent.name = agentName;
		agent.enabled = agentJson.value( "enabled", true );
		agent.command = agentJson.value( "command", "" );

		if ( agentJson.contains( "args" ) && agentJson["args"].is_array() ) {
			for ( const auto& arg : agentJson["args"] ) {
				agent.args.push_back( arg.get<std::string>() );
			}
		}

		if ( agentJson.contains( "environment" ) && agentJson["environment"].is_object() ) {
			for ( const auto& envItem : agentJson["environment"].items() ) {
				agent.environment[envItem.key()] = envItem.value().get<std::string>();
			}
		}

		agents[agentName] = agent;
	}
	return agents;
}

Plugin* AIAssistantPlugin::New( PluginManager* pluginManager ) {
	return eeNew( AIAssistantPlugin, ( pluginManager, false ) );
}

Plugin* AIAssistantPlugin::NewSync( PluginManager* pluginManager ) {
	return eeNew( AIAssistantPlugin, ( pluginManager, true ) );
}

AIAssistantPlugin::AIAssistantPlugin( PluginManager* pluginManager, bool sync ) :
	PluginBase( pluginManager ) {
	if ( sync ) {
		load( pluginManager );
	} else {
		mThreadPool->run( [this, pluginManager] { load( pluginManager ); } );
	}

	if ( getUISceneNode() ) {
		getPluginContext()->getSplitter()->forEachWidgetClass(
			"llm_chatui", [this]( UIWidget* widget ) {
				LLMChatUI* chat = static_cast<LLMChatUI*>( widget );
				chat->setManager( getManager() );
			} );
	}
}

AIAssistantPlugin::~AIAssistantPlugin() {
	{
		std::lock_guard<std::mutex> lock( mModelCatalogMutex );
		mModelCatalogCancelled->store( true );
		if ( mModelCatalog )
			mModelCatalog->cancel();
		mModelCatalog.reset();
	}
	if ( SceneManager::existsSingleton() && !SceneManager::instance()->isShuttingDown() ) {
		getPluginContext()->getSplitter()->forEachWidgetClass(
			"llm_chatui", []( UIWidget* widget ) {
				LLMChatUI* chat = static_cast<LLMChatUI*>( widget );
				chat->setManager( nullptr );
			} );
	}

	waitUntilLoaded();
	mShuttingDown = true;
	unsubscribeFileSystemListener();

	if ( mAIChatButton ) {
		if ( mAIChatButtonPosCbId )
			mAIChatButton->getParent()->removeEventListener( mAIChatButtonPosCbId );
		mAIChatButton->close();
	}
	getPluginContext()->getConfig().removeTabWidgetType( "llm_chatui" );
}

std::string AIAssistantPlugin::getPluginStatePath() const {
	return getManager()->getPluginsPath() + "state" + FileSystem::getOSSlash() + "aiassistant" +
		   FileSystem::getOSSlash();
}

std::string AIAssistantPlugin::getConversationsPath() const {
	return getPluginStatePath() + "chats" + FileSystem::getOSSlash();
}

void AIAssistantPlugin::load( PluginManager* pluginManager ) {
	Clock clock;
	AtomicBoolScopedOp loading( mLoading, true );
	pluginManager->subscribeMessages( this,
									  [this]( const auto& notification ) -> PluginRequestHandle {
										  return processMessage( notification );
									  } );

	const std::string bundledPath( pluginManager->getResourcesPath() + "plugins/aiassistant.json" );
	const std::string userPath( pluginManager->getPluginsPath() + "aiassistant.json" );
	if ( FileSystem::fileExists( userPath ) ||
		 FileSystem::fileWrite(
			 userPath, "{\n\"config\":{},\n  \"keybindings\":{},\n\"providers\":{}\n}\n" ) )
		mConfigPath = userPath;
	if ( !FileSystem::fileExists( bundledPath ) && mConfigPath.empty() )
		return;

	if ( FileSystem::fileExists( bundledPath ) ) {
		try {
			loadAIAssistantConfig( bundledPath, false );
		} catch ( const json::exception& e ) {
			Log::error( "Parsing AI assistant config \"%s\" failed:\n%s", bundledPath.c_str(),
						e.what() );
		}
	}

	LLMModelCatalog::Settings catalogSettings;
	catalogSettings.cachePath = getPluginStatePath() + "models.json";
	std::string userData;
	if ( !mConfigPath.empty() && FileSystem::fileGet( mConfigPath, userData ) ) {
		const auto userJson = json::parse( userData, nullptr, false, true );
		if ( userJson.is_object() && userJson.contains( "config" ) &&
			 userJson["config"].is_object() ) {
			const auto& userConfig = userJson["config"];
			if ( userConfig.contains( "model_catalog_enabled" ) &&
				 userConfig["model_catalog_enabled"].is_boolean() )
				catalogSettings.enabled = userConfig["model_catalog_enabled"].get<bool>();
			if ( userConfig.contains( "model_catalog_url" ) &&
				 userConfig["model_catalog_url"].is_string() )
				catalogSettings.url = userConfig["model_catalog_url"].get<std::string>();
			if ( userConfig.contains( "model_catalog_refresh_hours" ) &&
				 userConfig["model_catalog_refresh_hours"].is_number_unsigned() )
				catalogSettings.refreshIntervalHours =
					userConfig["model_catalog_refresh_hours"].get<std::uint32_t>();
		}
	}
	if ( !mConfigPath.empty() ) {
		try {
			loadAIAssistantConfig( mConfigPath, true );
		} catch ( const json::exception& e ) {
			Log::error( "Parsing AI assistant config \"%s\" failed:\n%s", mConfigPath.c_str(),
						e.what() );
		}
	}

	subscribeFileSystemListener();
	mReady = !mProviders.empty();

	TabWidgetCbs config;
	config.onLoad = [this]( const nlohmann::json& j ) {
		LLMChatUI* chatUI = LLMChatUI::New( mManager );

		if ( j.contains( "uuid" ) && j.contains( "summary" ) ) {
			auto uuid = j.value( "uuid", "" );
			auto filePath = chatUI->getNewFilePath( uuid, j.value( "summary", "" ),
													j.value( "locked", false ) );
			if ( filePath.empty() || !FileSystem::fileExists( filePath ) ) {
				auto conversationsPath = getConversationsPath();
				FileSystem::dirAddSlashAtEnd( conversationsPath );
				auto conversations = FileSystem::filesGetInPath( conversationsPath );

				auto foundIt = std::find_if( conversations.begin(), conversations.end(),
											 [&uuid]( const std::string& path ) {
												 return String::startsWith( path, uuid );
											 } );

				if ( foundIt != conversations.end() )
					filePath = conversationsPath + *foundIt;
			}

			std::string inputText;
			if ( !filePath.empty() ) {
				std::string data;
				FileSystem::fileGet( filePath, data );
				if ( !data.empty() ) {
					auto j = nlohmann::json::parse( data, nullptr, false );
					if ( !j.empty() ) {
						inputText = chatUI->unserialize( j );
					}
				}
			}

			chatUI->on( Event::OnDataChanged, [chatUI, inputText = std::move( inputText )]( auto ) {
				chatUI->updateTabTitle();
				if ( chatUI->getChatInput() )
					chatUI->getChatInput()->getDocument().textInput( inputText );
			} );
		}

		return TabWidgetData{ chatUI, getPluginContext()->findIcon( "chat-sparkle" ),
							  i18n( "ai_assistant", "AI Assistant" ) };
	};
	config.onSave = []( UIWidget* widget ) {
		LLMChatUI* chatUI = static_cast<LLMChatUI*>( widget );
		nlohmann::json j;
		if ( chatUI->hasChat() ) {
			j["uuid"] = chatUI->getUUID().toString();
			j["summary"] = chatUI->getSummary();
			j["locked"] = chatUI->isLocked();
		}
		return j;
	};

	getPluginContext()->getConfig().addTabWidgetType( "llm_chatui", config );
	{
		std::lock_guard<std::mutex> lock( mModelCatalogMutex );
		mModelCatalogSettings = catalogSettings;
	}

	if ( mReady ) {
		fireReadyCbs();
		setReady( clock.getElapsedTime() );
		// Re-enabled plugins already have a UI context. Initial startup safely defers this
		// request until PluginMessageType::UIReady.
		refreshModelCatalogAsync();
	}
}

void AIAssistantPlugin::refreshModelCatalogAsync() {
	LLMModelCatalog::Settings settings;
	Node* mainThreadNode;
	std::shared_ptr<LLMModelCatalog> catalog;
	std::shared_ptr<std::atomic_bool> cancelled;
	{
		std::lock_guard<std::mutex> lock( mModelCatalogMutex );
		if ( mModelCatalogRefreshStarted || !mModelCatalogSettings ||
			 !mModelCatalogSettings->enabled || mModelCatalogSettings->url.empty() )
			return;
		mainThreadNode =
			mManager->getSplitter() ? mManager->getSplitter()->getBaseLayout() : nullptr;
		if ( !mainThreadNode || !mainThreadNode->getSceneNode() )
			return;
		settings = *mModelCatalogSettings;
		mModelCatalog = std::make_shared<LLMModelCatalog>( settings );
		catalog = mModelCatalog;
		cancelled = mModelCatalogCancelled;
		mModelCatalogRefreshStarted = true;
	}
	auto* manager = mManager;
	auto applyProviders = [manager, mainThreadNode, cancelled]( LLMProviders providers ) mutable {
		if ( cancelled->load() )
			return;
		mainThreadNode->runOnMainThread(
			[providers = std::move( providers ), manager, cancelled]() mutable {
				if ( cancelled->load() || manager->isClosing() )
					return;
				auto* plugin = manager->get( AIAssistantPlugin::Definition().id );
				if ( plugin ) {
					static_cast<AIAssistantPlugin*>( plugin )->applyModelCatalog(
						std::move( providers ) );
				} else {
					Log::warning(
						"Could not apply refreshed LLM model catalog: plugin is unavailable" );
				}
			} );
	};
	LLMProviders providers = mProviders;
	mThreadPool->run( [catalog = std::move( catalog ), providers = std::move( providers ),
					   applyProviders = std::move( applyProviders ), cancelled]() mutable {
		if ( cancelled->load() )
			return;
		if ( catalog->loadCached( providers ) && !cancelled->load() )
			applyProviders( LLMProviders( providers ) );
		if ( cancelled->load() )
			return;
		catalog->refreshAsync( std::move( providers ), std::move( applyProviders ) );
	} );
}

void AIAssistantPlugin::applyModelCatalog( LLMProviders providers ) {
	std::string userData;
	if ( !mConfigPath.empty() && FileSystem::fileGet( mConfigPath, userData ) ) {
		const auto userJson = json::parse( userData, nullptr, false, true );
		try {
			if ( userJson.is_object() && userJson.contains( "providers" ) &&
				 userJson["providers"].is_object() )
				mergeLLMProviders( providers, parseLLMProviders( userJson["providers"] ), true );
		} catch ( const json::exception& error ) {
			Log::warning( "Could not reapply AI Assistant user providers after catalog refresh: %s",
						  error.what() );
		}
	}
	mProviders = std::move( providers );
	getPluginContext()->getSplitter()->forEachWidgetClass(
		"llm_chatui", [this]( UIWidget* widget ) {
			static_cast<LLMChatUI*>( widget )->setProviders( LLMProviders( mProviders ), true );
		} );
}

void AIAssistantPlugin::displayBrokenUserConfigFileWarning() {
	if ( nullptr == getUISceneNode() )
		return;

	NotificationCenter::instance()->addNotification(
		String::format( i18n( "error_aiassistant_config_parsing",
							  "AI Assistant Plugin - Error parsing AI Assistant config:\n%s" )
							.toUtf8(),
						mConfigFileError ),
		Seconds( 5 ) );
}

void AIAssistantPlugin::loadAIAssistantConfig( const std::string& path, bool updateConfigFile ) {
	std::string data;
	if ( !FileSystem::fileGet( path, data ) )
		return;
	if ( updateConfigFile )
		mBrokenUserConfigFile = false;
	json j;
	try {
		j = json::parse( data, nullptr, true, true );
	} catch ( const json::exception& e ) {
		Log::error(
			"AIAssistantPlugin::loadAIAssistantConfig - Error parsing AI assistant config from "
			"path %s, error: %s, config file content:\n%s",
			path.c_str(), e.what(), data.c_str() );
		if ( !updateConfigFile )
			return;
		else {
			// updateConfigFile = true is always the user config file
			// file recreation logic has been disabled
			mBrokenUserConfigFile = true;
			mConfigFileError = e.what();
			displayBrokenUserConfigFileWarning();
			return;
		}
		// Recreate it
		j = json::parse( "{\n\"config\":{},\n  \"keybindings\":{},\n\"providers\":[]\n}\n", nullptr,
						 true, true );
	}

	if ( updateConfigFile ) {
		mConfigHash = String::hash( data );
	}

	if ( j.contains( "config" ) ) {
		auto& config = j["config"];
		if ( updateConfigFile ) {
			if ( !config.contains( "model_catalog_enabled" ) )
				config["model_catalog_enabled"] = true;
			if ( !config.contains( "model_catalog_url" ) )
				config["model_catalog_url"] = "https://models.dev/api.json";
			if ( !config.contains( "model_catalog_refresh_hours" ) )
				config["model_catalog_refresh_hours"] = 24;
		}

		if ( config.contains( "display_reasoning" ) && config["display_reasoning"].is_boolean() )
			mDisplayReasoning = config.value( "display_reasoning", false );
		else if ( updateConfigFile )
			config["display_reasoning"] = mDisplayReasoning;

		if ( config.contains( "openai_api_key" ) )
			mApiKeys["openai"] = config.value( "openai_api_key", "" );
		else if ( updateConfigFile )
			config["openai_api_key"] = mApiKeys["openai"];

		if ( config.contains( "anthropic_api_key" ) )
			mApiKeys["anthropic"] = config.value( "anthropic_api_key", "" );
		else if ( updateConfigFile )
			config["anthropic_api_key"] = mApiKeys["anthropic"];

		if ( config.contains( "google_ai_api_key" ) )
			mApiKeys["google"] = config.value( "google_ai_api_key", "" );
		else if ( updateConfigFile )
			config["google_ai_api_key"] = mApiKeys["google"];

		if ( config.contains( "deepseek_api_key" ) )
			mApiKeys["deepseek"] = config.value( "deepseek_api_key", "" );
		else if ( updateConfigFile )
			config["deepseek_api_key"] = mApiKeys["deepseek"];

		if ( config.contains( "mistral_api_key" ) )
			mApiKeys["mistral"] = config.value( "mistral_api_key", "" );
		else if ( updateConfigFile )
			config["mistral_api_key"] = mApiKeys["mistral"];

		if ( config.contains( "xai_api_key" ) )
			mApiKeys["xai"] = config.value( "xai_api_key", "" );
		else if ( updateConfigFile )
			config["xai_api_key"] = mApiKeys["xai"];

		if ( config.contains( "github_api_key" ) )
			mApiKeys["github"] = config.value( "github_api_key", "" );
		else if ( updateConfigFile )
			config["github_api_key"] = mApiKeys["github"];

		if ( config.contains( "perplexity_api_key" ) )
			mApiKeys["perplexity"] = config.value( "perplexity_api_key", "" );
		else if ( updateConfigFile )
			config["perplexity_api_key"] = mApiKeys["perplexity"];

		if ( config.contains( "openrouter_api_key" ) )
			mApiKeys["openrouter"] = config.value( "openrouter_api_key", "" );
		else if ( updateConfigFile )
			config["openrouter_api_key"] = mApiKeys["openrouter"];

		if ( config.contains( "moonshot_api_key" ) )
			mApiKeys["moonshotai"] = config.value( "moonshot_api_key", "" );
		else if ( updateConfigFile )
			config["moonshot_api_key"] = mApiKeys["moonshotai"];

		if ( config.contains( "nvidia_api_key" ) )
			mApiKeys["nvidia"] = config.value( "nvidia_api_key", "" );
		else if ( updateConfigFile )
			config["nvidia_api_key"] = mApiKeys["nvidia"];

		if ( config.contains( "together_api_key" ) )
			mApiKeys["togetherai"] = config.value( "together_api_key", "" );
		else if ( updateConfigFile )
			config["together_api_key"] = mApiKeys["togetherai"];

		if ( config.contains( "mimo_api_key" ) )
			mApiKeys["xiaomi"] = config.value( "mimo_api_key", "" );
		else if ( updateConfigFile )
			config["mimo_api_key"] = mApiKeys["xiaomi"];

		if ( config.contains( "api_keys" ) && config["api_keys"].is_object() ) {
			for ( const auto& [provider, apiKey] : config["api_keys"].items() )
				if ( apiKey.is_string() )
					mApiKeys[provider] = apiKey.get<std::string>();
		} else if ( updateConfigFile ) {
			config["api_keys"] = json::object();
		}
	}

	if ( mKeyBindings.empty() ) {
		mKeyBindings["new-ai-assistant"] = "mod+shift+m";
		mKeyBindings["ai-prompt"] = "mod+return";
		mKeyBindings["ai-chat-history"] = "mod+h";
		mKeyBindings["ai-clone-chat"] = "mod+shift+c";
		mKeyBindings["ai-settings"] = "mod+shift+s";
		mKeyBindings["ai-toggle-private-chat"] = "mod+shift+p";
		mKeyBindings["ai-save-chat"] = "mod+s";
		mKeyBindings["ai-rename-chat"] = "f2";
		mKeyBindings["ai-show-menu"] = "mod+m";
		mKeyBindings["ai-chat-toggle-role"] = "mod+shift+r";
		mKeyBindings["ai-refresh-local-models"] = "mod+shift+l";
		mKeyBindings["ai-attach-file"] = "mod+shift+a";
		mKeyBindings["ai-link-file"] = "mod+shift+z";
		mKeyBindings["ai-select-model"] = "mod+shift+x";
		mKeyBindings["ai-toggle-agent-mode"] = "mod+shift+d";
		mKeyBindings["ai-agent-config"] = "shift+alt+c";
		mKeyBindings["ai-add-chat"] = "mod+shift+return";
	}

	auto& kb = j["keybindings"];
	for ( const auto& key : AIAssistantCommandList ) {
		if ( kb.contains( key ) ) {
			if ( !kb[key].empty() )
				mKeyBindings[key] = kb[key];
		} else if ( updateConfigFile )
			kb[key] = mKeyBindings[key];
	}

	if ( updateConfigFile ) {
		std::string newData( j.dump( 2 ) );
		if ( newData != data ) {
			FileSystem::fileWrite( path, newData );
			mConfigHash = String::hash( newData );
		}
	}

	if ( j.contains( "agents" ) ) {
		auto agents = parseACPAgents( j["agents"] );
		for ( const auto& [key, value] : agents ) {
			mAgents[key] = value;
		}
	}

	if ( !j.contains( "providers" ) )
		return;

	auto providers = parseLLMProviders( j["providers"] );
	if ( mProviders.empty() ) {
		mProviders = std::move( providers );
	} else {
		// User models override catalog metadata; bundled configs only fill gaps.
		mergeLLMProviders( mProviders, std::move( providers ), updateConfigFile );
	}

	if ( getUISceneNode() )
		initUI();
}

LLMChatUI* AIAssistantPlugin::newAIAssistant() {
	auto splitter = getPluginContext()->getSplitter();
	auto chatUI = LLMChatUI::New( mManager );
	auto tabName( i18n( "ai_assistant", "AI Assistant" ) );
	UITabWidget* tabWidget = splitter->getTabWidgets()[splitter->getTabWidgets().size() - 1];
	if ( !splitter->hasSplit() )
		tabWidget = splitter->splitTabWidget( SplitDirection::Right, tabWidget );
	auto [tab, _] = splitter->createWidgetInTabWidget( tabWidget, chatUI, tabName );
	auto icon = getPluginContext()->findIcon( "chat-sparkle" );
	if ( icon )
		tab->setIcon( icon );
	return chatUI;
}

void AIAssistantPlugin::onRegisterDocument( TextDocument* doc ) {
	doc->setCommand( "new-ai-assistant", [this] { newAIAssistant()->setFocus(); } );
}

void AIAssistantPlugin::onRegisterEditor( UICodeEditor* editor ) {
	editor->addUnlockedCommands( AIAssistantUnlockedCommandList );

	for ( auto& kb : mKeyBindings ) {
		if ( !kb.second.empty() && std::find( AIAssistantUnlockedCommandList.begin(),
											  AIAssistantUnlockedCommandList.end(),
											  kb.first ) != AIAssistantUnlockedCommandList.end() ) {
			editor->getKeyBindings().addKeybindString( kb.second, kb.first );
		}
	}
}

void AIAssistantPlugin::onUnregisterEditor( UICodeEditor* editor ) {
	editor->removeUnlockedCommands( AIAssistantUnlockedCommandList );
}

PluginRequestHandle AIAssistantPlugin::processMessage( const PluginMessage& msg ) {
	switch ( msg.type ) {
		case ecode::PluginMessageType::UIReady: {
			refreshModelCatalogAsync();

			for ( const auto& kb : mKeyBindings ) {
				if ( !String::startsWith( kb.first, "ai-" ) ) {
					getPluginContext()->getMainLayout()->getKeyBindings().addKeybindString(
						kb.second, kb.first );
				}
			}

			if ( !mUIInit )
				initUI();

			if ( mBrokenUserConfigFile )
				displayBrokenUserConfigFileWarning();

			break;
		}
		default:
			break;
	}
	return PluginRequestHandle::empty();
}

void AIAssistantPlugin::initUI() {
	mUIInit = true;

	getPluginContext()->getMainLayout()->setCommand( "new-ai-assistant",
													 [this] { newAIAssistant()->setFocus(); } );

	if ( !mStatusBar )
		getUISceneNode()->bind( "status_bar", mStatusBar );
	if ( !mStatusBar )
		return;

	if ( !mAIChatButton ) {
		mAIChatButton = UIPushButton::New();
		mAIChatButton->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::MatchParent );
		mAIChatButton->setParent( mStatusBar );
		mAIChatButton->setId( "ai_assistant_but" );
		mAIChatButton->setClass( "status_but" );
		mAIChatButton->setIcon( iconDrawable( "chat-sparkle", 14 ) );
		mAIChatButton->setTooltipText( i18n( "ai_assistant", "AI Assistant" ) );

		mAIChatButton->on( Event::MouseClick,
						   [this]( const Event* ) { newAIAssistant()->setFocus(); } );

		mAIChatButtonPosCbId =
			mAIChatButton->getParent()->on( Event::OnChildCountChanged, []( const Event* event ) {
				auto statusButton = event->getNode()->find( "ai_assistant_but" );
				if ( statusButton && event->getNode()->getLastChild() != statusButton )
					statusButton->toFront();
			} );
	}
}

std::optional<std::string> AIAssistantPlugin::getApiKeyFromProvider( const std::string& provider,
																	 AIAssistantPlugin* instance ) {
	static const char* OPEN_API_KEY = "";

	const char* ret = nullptr;
	if ( provider == "openai" ) {
		ret = getenv( "OPENAI_API_KEY" );
	} else if ( provider == "anthropic" ) {
		ret = getenv( "ANTHROPIC_API_KEY" );
	} else if ( provider == "google" ) {
		const char* apiKey = getenv( "GOOGLE_AI_API_KEY" );
		if ( apiKey != nullptr )
			ret = apiKey;
		else
			ret = getenv( "GEMINI_API_KEY" );
	} else if ( provider == "deepseek" ) {
		ret = getenv( "DEEPSEEK_API_KEY" );
	} else if ( provider == "mistral" ) {
		ret = getenv( "MISTRAL_API_KEY" );
	} else if ( provider == "xai" ) {
		const char* apiKey = getenv( "XAI_API_KEY" );
		if ( apiKey != nullptr )
			ret = apiKey;
		else
			ret = getenv( "GROK_API_KEY" );
	} else if ( provider == "github" ) {
		ret = getenv( "GITHUB_API_KEY" );
	} else if ( provider == "perplexity" ) {
		ret = getenv( "PERPLEXITY_API_KEY" );
	} else if ( provider == "openrouter" ) {
		ret = getenv( "OPENROUTER_API_KEY" );
	} else if ( provider == "moonshotai" ) {
		ret = getenv( "MOONSHOT_API_KEY" );
	} else if ( provider == "nvidia" ) {
		ret = getenv( "NVIDIA_API_KEY" );
	} else if ( provider == "togetherai" ) {
		ret = getenv( "TOGETHER_API_KEY" );
	} else if ( provider == "xiaomi" ) {
		ret = getenv( "MIMO_API_KEY" );
	} else {
		const auto& providerModelIt = instance->mProviders.find( provider );
		if ( providerModelIt != instance->mProviders.end() ) {
			for ( const auto& env : providerModelIt->second.apiKeyEnvVars ) {
				if ( !String::icontains( env, "key" ) && !String::icontains( env, "token" ) )
					continue;
				ret = getenv( env.c_str() );
				if ( ret )
					break;
			}
			if ( !ret && providerModelIt->second.apiKeyEnvVars.size() == 1 )
				ret = getenv( providerModelIt->second.apiKeyEnvVars.front().c_str() );
			if ( !ret && providerModelIt->second.openApi )
				ret = OPEN_API_KEY;
		}
	}

	if ( ret )
		return std::string{ ret };

	if ( instance ) {
		auto providerIt = instance->mApiKeys.find( provider );
		if ( providerIt != instance->mApiKeys.end() )
			return providerIt->second;
	}

	return {};
}

void AIAssistantPlugin::onSaveState( IniFile* state ) {
	std::vector<LLMChatUI*> chats;
	LLMChatUI* mainChat{ nullptr };

	getPluginContext()->getSplitter()->forEachWidgetClass(
		"llm_chatui", [&chats, &mainChat]( UIWidget* widget ) {
			LLMChatUI* chat = static_cast<LLMChatUI*>( widget );
			chats.emplace_back( chat );
			if ( widget->isVisible() && mainChat == nullptr )
				mainChat = chat;
		} );

	if ( mainChat == nullptr && !chats.empty() )
		mainChat = chats[chats.size() - 1];

	AIAssistantConfig config;

	if ( mainChat != nullptr ) {
		config.partition = mainChat->getSplitter()->getSplitPartition();
		config.modelProvider = mainChat->getCurModel().provider;
		config.modelName = mainChat->getCurModel().name;
		config.agentName = mainChat->getCurAgent();
	} else {
		config = mConfig;
	}

	if ( mConfig.modelName.empty() || mConfig.modelProvider.empty() ||
		 mConfig.partition.getValue() == 0 )
		return;

	const std::string keyname = "aiassistant";
	state->setValue( keyname, "split_partition", config.partition.toString() );
	state->setValue( keyname, "default_provider", config.modelProvider );
	state->setValue( keyname, "default_model", config.modelName );
	state->setValue( keyname, "default_agent", config.agentName );
}

} // namespace ecode
