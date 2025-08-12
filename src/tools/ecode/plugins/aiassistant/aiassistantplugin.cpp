#include "aiassistantplugin.hpp"
#include "chatui.hpp"
#include "protocol.hpp"

#include "../../appconfig.hpp"
#include "../../widgetcommandexecuter.hpp"

#include <eepp/system/filesystem.hpp>
#include <eepp/system/scopedop.hpp>

using json = nlohmann::json;

namespace ecode {

static std::initializer_list<std::string> AIAssistantUnlockedCommandList = {

	"new-ai-assistant"

};

static std::initializer_list<std::string> AIAssistantCommandList = {

	"ai-prompt",
	"ai-add-chat",
	"ai-chat-history",
	"ai-attach-file",
	"ai-clone-chat",
	"ai-settings",
	"ai-toggle-private-chat",
	"ai-save-chat",
	"ai-rename-chat",
	"ai-show-menu",
	"ai-chat-toggle-role",
	"ai-refresh-local-models",
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

				if ( modelJson.contains( "cache_configuration" ) &&
					 !modelJson["cache_configuration"].is_null() ) {
					const auto& cacheJson = modelJson["cache_configuration"];
					LLMCacheConfiguration cache;
					cache.maxCacheAnchors = cacheJson["max_cache_anchors"].get<int>();
					cache.minTotalToken = cacheJson["min_total_token"].get<int>();
					cache.shouldSpeculate = cacheJson["should_speculate"].get<bool>();
					model.cacheConfiguration = cache;
				}

				provider.models.push_back( model );
			}
		}

		providers[providerName] = provider;
	}

	return providers;
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
	if ( SceneManager::existsSingleton() && !SceneManager::instance()->isShuttingDown() ) {
		getPluginContext()->getSplitter()->forEachWidgetClass(
			"llm_chatui", []( UIWidget* widget ) {
				LLMChatUI* chat = static_cast<LLMChatUI*>( widget );
				chat->setManager( nullptr );
			} );
	}

	waitUntilLoaded();
	mShuttingDown = true;
	if ( mStatusButton )
		mStatusButton->close();

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

	std::vector<std::string> paths;
	std::string path( pluginManager->getResourcesPath() + "plugins/aiassistant.json" );
	if ( FileSystem::fileExists( path ) )
		paths.emplace_back( path );
	path = pluginManager->getPluginsPath() + "aiassistant.json";
	if ( FileSystem::fileExists( path ) ||
		 FileSystem::fileWrite(
			 path, "{\n\"config\":{},\n  \"keybindings\":{},\n\"providers\":[]\n}\n" ) ) {
		mConfigPath = path;
		paths.emplace_back( path );
	}
	if ( paths.empty() )
		return;
	for ( const auto& tpath : paths ) {
		try {
			loadAIAssistantConfig( tpath, mConfigPath == tpath );
		} catch ( const json::exception& e ) {
			Log::error( "Parsing linter \"%s\" failed:\n%s", tpath.c_str(), e.what() );
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

		return TabWidgetData{ chatUI, getPluginContext()->findIcon( "code-ai" ),
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

	if ( mReady ) {
		fireReadyCbs();
		setReady( clock.getElapsedTime() );
	}
}

void AIAssistantPlugin::loadAIAssistantConfig( const std::string& path, bool updateConfigFile ) {
	std::string data;
	if ( !FileSystem::fileGet( path, data ) )
		return;
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
		// Recreate it
		j = json::parse( "{\n\"config\":{},\n  \"keybindings\":{},\n\"providers\":[]\n}\n", nullptr,
						 true, true );
	}

	if ( updateConfigFile ) {
		mConfigHash = String::hash( data );
	}

	if ( j.contains( "config" ) ) {
		auto& config = j["config"];

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
	}

	if ( mKeyBindings.empty() ) {
		mKeyBindings["new-ai-assistant"] = "mod+shift+m";
		mKeyBindings["ai-prompt"] = "mod+return";
		mKeyBindings["ai-add-chat"] = "mod+shift+return";
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

	if ( !j.contains( "providers" ) )
		return;

	auto providers = parseLLMProviders( j["providers"] );
	if ( mProviders.empty() ) {
		mProviders = std::move( providers );
	} else {
		for ( const auto& [key, value] : providers ) {
			auto providerIt = mProviders.find( key );
			if ( providerIt != mProviders.end() ) {
				auto& provider = providerIt->second;
				if ( !value.apiUrl.empty() )
					provider.apiUrl = value.apiUrl;
				if ( !value.name.empty() )
					provider.name = value.name;
				if ( value.displayName )
					provider.displayName = value.displayName;
				if ( value.fetchModelsUrl )
					provider.fetchModelsUrl = value.fetchModelsUrl;
				// Add model if not exists
				for ( auto& model : value.models ) {
					if ( std::find_if( provider.models.begin(), provider.models.end(),
									   [&model]( const LLMModel& cmodel ) {
										   return cmodel.provider == model.provider &&
												  cmodel.name == model.name;
									   } ) == provider.models.end() ) {
						provider.models.emplace_back( std::move( model ) );
					}
				}
			} else {
				mProviders.insert( { key, std::move( value ) } );
			}
		}
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
	auto icon = getPluginContext()->findIcon( "code-ai" );
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
			for ( const auto& kb : mKeyBindings ) {
				if ( !String::startsWith( kb.first, "ai-" ) ) {
					getPluginContext()->getMainLayout()->getKeyBindings().addKeybindString(
						kb.second, kb.first );
				}
			}

			if ( !mUIInit )
				initUI();

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

	if ( !mStatusButton ) {
		mStatusButton = UIPushButton::New();
		mStatusButton->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::MatchParent );
		mStatusButton->setParent( mStatusBar );
		mStatusButton->setId( "ai_assistant_but" );
		mStatusButton->setClass( "status_but" );
		mStatusButton->setIcon( iconDrawable( "code-ai", 14 ) );
		mStatusButton->setTooltipText( i18n( "ai_assistant", "AI Assistant" ) );
		mStatusButton->on( Event::MouseClick,
						   [this]( const Event* ) { newAIAssistant()->setFocus(); } );
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
	} else if ( provider == "lmstudio" || provider == "ollama" ) {
		ret = OPEN_API_KEY;
	} else if ( provider == "xai" ) {
		const char* apiKey = getenv( "XAI_API_KEY" );
		if ( apiKey != nullptr )
			ret = apiKey;
		else
			ret = getenv( "GROK_API_KEY" );
	} else if ( provider == "github" ) {
		ret = getenv( "GITHUB_API_KEY" );
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
}

} // namespace ecode
