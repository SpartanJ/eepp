#include "aiassistantplugin.hpp"
#include "chatui.hpp"

#include <eepp/system/filesystem.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uidropdownlist.hpp>
#include <eepp/ui/uiicon.hpp>
#include <eepp/ui/uimessagebox.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollview.hpp>
#include <eepp/window/clipboard.hpp>
#include <eepp/window/window.hpp>

#include <nlohmann/json.hpp>

using namespace EE::System;
using namespace EE::Window;

namespace ecode {

const char* LLMChat::roleToString( Role role ) {
	switch ( role ) {
		case Role::User:
			return "user";
		case Role::Assistant:
			return "assistant";
		case Role::System:
			return "system";
		case Role::Tool:
			return "tool";
	}
	return "";
}

LLMChat::Role LLMChat::stringToRole( UIPushButton* userBut ) {
	if ( userBut->getText() == userBut->i18n( "user", "User" ) ) {
		return Role::User;
	} else if ( userBut->getText() == userBut->i18n( "assistant", "Assistant" ) ) {
		return Role::Assistant;
	} else if ( userBut->getText() == userBut->i18n( "system", "System" ) ) {
		return Role::System;
	}
	return Role::User;
}

static const char* DEFAULT_LAYOUT = R"xml(
<style>
.llm_chatui DropDownList {
	border: 0;
	background-color: var(--tab-back);
}
.llm_chatui #settings_but {
	tint: var(--floating-icon);
}
.llm_conversation {
	margin-bottom: 8dp;
}
.llm_conversation > .llm_conversation_opt {
	layout-gravity: left;
}
.llm_conversation.user > .llm_conversation_opt {
	layout-gravity: right;
}
.llm_conversation {
	margin-left: 0dp;
	margin-right: 32dp;
}
.llm_conversation.user {
	margin-left: 32dp;
	margin-right: 0dp;
}
.llm_conversation CodeEditor,
.llm_chat_input {
	padding: 8dp;
	disable-editor-flags: editorfeatures|defaultstyle;
	border-radius: 0dp 8dp 8dp 8dp;
	line-wrap-mode: word;
	line-wrap-type: viewport;
}
.llm_conversation.user CodeEditor,
.llm_chat_input {
	border-radius: 8dp 0dp 8dp 8dp;
}
CodeEditor.llm_chat_input {
	font-family: monospace;
	border-radius: 8dp;
	padding-bottom: 38dp;
}
.llm_chatui PushButton {
	border-color: transparent;
	text-as-fallback: true;
}
.llm_chatui PushButton:hover {
	border-color: var(--primary);
}
.llm_conversation_opt PushButton {
	lh: 16dp;
	text-as-fallback: true;
}
.llm_button {
	text-as-fallback: true;
}
PushButton.llm_button.primary {
	text-as-fallback: false;
}
DropDownList.role_ui {
	gravity: left|center_vertical;
}
</style>
<Splitter lw="mp" lh="mp" orientation="vertical" splitter-partition="75%" padding="4dp">
	<RelativeLayout lw="mp">
		<ScrollView lw="mp" lh="mp" class="llm_chat_scrollview">
			<vbox lw="mp" lh="wc" class="llm_chats"></vbox>
		</ScrollView>
		<vbox id="chat_presentation" lw="wc" lh="wc" layout-gravity="center" gravity="center">
			<Image lw="wc" lh="wc" icon="icon(robot-2, 96dp)" gravity="center" layout-gravity="center" />
			<TextView text="@string(ai_llm_presentation, What can I help with?)" font-size="24dp" />
		</vbox>
	</RelativeLayout>
	<RelativeLayout lw="mp" class="llm_controls">
		<CodeEditor class="llm_chat_input" lw="mp" lh="mp" />
		<hbox lw="mp" lh="wc" layout_gravity="bottom|left" layout_margin="8dp" clip="false">
			<PushButton id="llm_user" class="llm_button" text="@string(user, User)" min-width="60dp" layout-margin-right="8dp" />
			<PushButton class="llm_button" text="@string(attach, Attach)" icon="icon(attach, 14dp)" min-width="32dp" />
			<hbox lw="0" lw8="1" lh="mp" layout_gravity="center" padding-left="8dp" padding-right="8dp">
				<PushButton id="settings_but" text="@string(settings, Settings)" icon="icon(settings, 14dp)" tooltip="@string(settings, Settings)" margin-right="8dp" />
				<DropDownList class="model_ui" lw="0" lw8="1" selected-index="0"></DropDownList>
				<PushButton id="refresh_model_ui" text="@string(refresh_model_ui, Refresh)" icon="icon(refresh, 14dp)" />
			</hbox>
			<PushButton id="llm_add_chat" class="llm_button" text="@string(add, Add)" icon="icon(add, 15dp)" min-width="32dp" layout-margin-right="8dp" />
			<PushButton id="llm_run" class="llm_button primary" text="@string(run, Run)" icon="icon(play-filled, 14dp)" />
			<PushButton id="llm_stop" class="llm_button primary" text="@string(stop, Stop)" icon="icon(stop, 12dp)" visible="false" enabled="false" />
		</hbox>
	</RelativeLayout>
</Splitter>
)xml";

static const char* DEFAULT_CHAT_GLOBE = R"xml(
<vbox class="llm_conversation" lw="mp" lh="wc">
	<hbox class="llm_conversation_opt">
		<DropDownList class="role_ui" lw="150dp" lh="16dp" selected-index="1">
			<item>@string(user, User)</item>
			<item>@string(assistant, Assistant)</item>
			<item>@string(system, System)</item>
		</DropDownList>
		<PushButton class="move_up" text="@string(move_up, Move Up)" icon="icon(arrow-up-s, 12dp)" tooltip="@string(move_up, Move Up)" />
		<PushButton class="move_down" text="@string(move_down, Move Down)" icon="icon(arrow-down-s, 12dp)"  tooltip="@string(move_down, Move Down)" />
		<PushButton class="erase_but" text="@string(remove_chat, Remove Chat)" icon="icon(chrome-close, 10dp)" tooltip="@string(remove_chat, Remove Chat)" />
	</hbox>
	<CodeEditor class="data_ui" lw="mp" lh="32dp" />
</vbox>
)xml";

LLMChatUI::LLMChatUI( PluginManager* manager ) : UILinearLayout(), mManager( manager ) {
	setClass( "llm_chatui" );
	setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent );

	getUISceneNode()->loadLayoutFromString( DEFAULT_LAYOUT, this );

	mChatsList = findByClass( "llm_chats" );
	mModelDDL = findByClass<UIDropDownList>( "model_ui" );

	find( "refresh_model_ui" )->onClick( [this]( auto ) { fillApiModels( mModelDDL ); } );

	find( "settings_but" )->onClick( [this]( auto ) {
		if ( getPlugin() )
			getPlugin()->getPluginContext()->focusOrLoadFile( getPlugin()->getFileConfigPath() );
	} );

	mChatScrollView = findByClass( "llm_chat_scrollview" )->asType<UIScrollView>();
	mChatScrollView->getVerticalScrollBar()->setValue( 1 );

	mChatInput = findByClass<UICodeEditor>( "llm_chat_input" );

	on( Event::OnFocus, [this]( auto ) { mChatInput->setFocus(); } );

	if ( getPlugin() ) {
		mChatInput->setColorScheme(
			getPlugin()->getPluginContext()->getSplitter()->getCurrentColorScheme() );
	}
	mChatInput->getKeyBindings().addKeybindString( "mod+return", "prompt" );
	mChatInput->getKeyBindings().addKeybindString( "mod+keypad enter", "prompt" );

	mChatInput->getKeyBindings().addKeybindString( "mod+shift+return", "add_chat" );
	mChatInput->getKeyBindings().addKeybindString( "mod+shift+keypad enter", "add_chat" );

	mChatInput->getDocument().setCommand( "add_chat", [this] {
		if ( mChatInput->getDocument().isEmpty() )
			return;

		addChat( LLMChat::stringToRole( mChatUserRole ),
				 mChatInput->getDocument().getText().toUtf8() );
		mChatInput->getDocument().selectAll();
		mChatInput->getDocument().textInput( String{} );
		mChatInput->setFocus();
	} );

	mChatInput->getDocument().setCommand( "prompt", [this] {
		auto chats = findAllByClass( "llm_conversation" );

		if ( chats.empty() && mChatInput->getDocument().isEmpty() )
			return;

		auto inputUserRole = LLMChat::stringToRole( mChatUserRole );
		if ( ( !chats.empty() &&
			   ( mChatInput->getDocument().isEmpty() || inputUserRole != LLMChat::Role::User ) ) ||
			 ( chats.empty() && inputUserRole != LLMChat::Role::User ) ) {
			if ( chats[chats.size() - 1]
					 ->findByClass( "role_ui" )
					 ->asType<UIDropDownList>()
					 ->getListBox()
					 ->getItemSelectedIndex() != 0 ) {
				showMsg( getUISceneNode()->i18n(
					"llm_last_message_must_be_from_user",
					"The last chat message must be from a \"User\" role" ) );
			}
		}

		mChatInput->getDocument().execute( "add_chat" );
		doRequest();
	} );

	mChatInput->getDocument().setCommand( "prompt-stop", [this] {
		if ( mRequest )
			mRequest->cancel();
	} );

	find( "llm_add_chat" )->onClick( [this]( auto ) {
		mChatInput->getDocument().execute( "add_chat" );
	} );

	const auto& markdown = SyntaxDefinitionManager::instance()->getByLSPName( "markdown" );
	mChatInput->setShowFoldingRegion( true );
	mChatInput->getDocument().getFoldRangeService().setEnabled( true );
	mChatInput->setFoldDrawable( findIcon( "chevron-down", PixelDensity::dpToPxI( 12 ) ) );
	mChatInput->setFoldedDrawable( findIcon( "chevron-right", PixelDensity::dpToPxI( 12 ) ) );

	mChatInput->setSyntaxDefinition( markdown );

	mChatRun = find<UIPushButton>( "llm_run" );
	mChatRun->onClick( [this]( auto ) { mChatInput->getDocument().execute( "prompt" ); } );

	mChatStop = find<UIPushButton>( "llm_stop" );
	mChatStop->onClick( [this]( auto ) { mChatInput->getDocument().execute( "prompt-stop" ); } );

	mChatUserRole = find<UIPushButton>( "llm_user" );
	mChatUserRole->onClick( [this]( auto ) {
		if ( mChatUserRole->getText() == mChatUserRole->i18n( "user", "User" ) ) {
			mChatUserRole->setText( mChatUserRole->i18n( "assistant", "Assistant" ) );
		} else if ( mChatUserRole->getText() == mChatUserRole->i18n( "assistant", "Assistant" ) ) {
			mChatUserRole->setText( mChatUserRole->i18n( "system", "System" ) );
		} else if ( mChatUserRole->getText() == mChatUserRole->i18n( "system", "System" ) ) {
			mChatUserRole->setText( mChatUserRole->i18n( "user", "User" ) );
		}
	} );

	if ( getPlugin() == nullptr )
		return;

	auto providers = getPlugin()->getProviders();
	setProviders( std::move( providers ) );

	fillModelDropDownList( mModelDDL );
}

void LLMChatUI::fillApiModels( UIDropDownList* modelDDL ) {
	for ( auto& [name, data] : mProviders ) {
		if ( !data.enabled || !data.fetchModelsUrl )
			continue;

		auto res = Http::get( *data.fetchModelsUrl, Milliseconds( 100 ) );
		if ( res.getStatus() != Http::Response::Status::Ok )
			continue;

		nlohmann::json j = nlohmann::json::parse( res.getBody(), nullptr, false, true );

		if ( !( j.contains( "data" ) && j["data"].is_array() ) &&
			 !( j.contains( "models" ) && j["models"].is_array() ) )
			continue;

		data.models.erase(
			std::remove_if( data.models.begin(), data.models.end(),
							[]( const LLMModel& model ) { return model.isEphemeral; } ),
			data.models.end() );

		const auto& jdata = j.contains( "data" ) ? j["data"] : j["models"];

		for ( const auto& el : jdata ) {
			LLMModel model;
			model.provider = name;
			model.name = el.contains( "model" ) ? el.value( "model", "" ) : el.value( "id", "" );

			if ( el.contains( "name" ) )
				model.displayName = el.value( "name", "" );

			model.isEphemeral = true;

			if ( model.name.empty() )
				continue;

			if ( el.contains( "max_context_length" ) )
				model.maxOutputTokens = el.value( "max_context_length", 0 );

			data.models.emplace_back( model );
		}

		std::string pname = name;
		modelDDL->runOnMainThread( [pname = std::move( pname ), modelDDL, this] {
			String providerName( pname );
			std::vector<String> removeValues;
			size_t count = modelDDL->getListBox()->getCount();
			for ( size_t i = 0; i < count; i++ ) {
				const String& txt = modelDDL->getListBox()->getItemText( i );
				if ( txt.contains( providerName ) )
					removeValues.emplace_back( txt );
			}

			for ( const auto& val : removeValues )
				modelDDL->getListBox()->removeListBoxItem( val );

			const auto& models = mProviders[pname].models;
			std::vector<String> newModels;
			for ( const auto& model : models ) {
				if ( !model.isEphemeral )
					continue;
				newModels.emplace_back( String::format( "%s (%s)", model.name, pname ) );
				mModelsMap[newModels[newModels.size() - 1].getHash()] = model;
			}

			modelDDL->getListBox()->addListBoxItems( newModels );
		} );
	}
}

void LLMChatUI::fillModelDropDownList( UIDropDownList* modelDDL ) {
	std::vector<String> models;
	std::size_t selectedIndex = 0;
	for ( const auto& [name, data] : mProviders ) {
		if ( !data.enabled )
			continue;

		for ( const auto& model : data.models ) {
			String modelName( String::format(
				"%s (%s)", model.displayName ? *model.displayName : model.name,
				data.displayName ? *data.displayName : String::capitalize( data.name ) ) );
			mModelsMap[modelName.getHash()] = model;
			if ( model.provider == "openai" && model.name == "gpt-4o" ) {
				mCurModel = model;
				selectedIndex = models.size();
			}
			models.push_back( std::move( modelName ) );
		}
	}
	modelDDL->getListBox()->addListBoxItems( std::move( models ) );
	modelDDL->getListBox()->setSelected( selectedIndex );
	modelDDL->on( Event::OnValueChange, [this, modelDDL]( auto ) {
		auto selectedModel =
			mModelsMap.find( modelDDL->getListBox()->getItemSelectedText().getHash() );
		if ( selectedModel != mModelsMap.end() ) {
			mCurModel = selectedModel->second;
		}
	} );

	modelDDL->getUISceneNode()->getThreadPool()->run(
		[this, modelDDL] { fillApiModels( modelDDL ); } );
}

void LLMChatUI::resizeToFit( UICodeEditor* editor ) {
	Float visibleLineCount = editor->getDocumentView().getVisibleLinesCount();
	Float lineHeight = editor->getLineHeight();
	Float height = lineHeight * visibleLineCount + editor->getPixelsPadding().Top +
				   editor->getPixelsPadding().Bottom;
	editor->setPixelsSize( editor->getPixelsSize().getWidth(), height );
}

nlohmann::json LLMChatUI::chatToJson() {
	auto j = nlohmann::json::array();
	auto chats = findAllByClass( "llm_conversation" );
	for ( const auto& chat : chats ) {
		UIDropDownList* roleDDL = chat->findByClass<UIDropDownList>( "role_ui" );
		UICodeEditor* codeEditor = chat->findByClass<UICodeEditor>( "data_ui" );
		std::string role = LLMChat::roleToString(
			static_cast<LLMChat::Role>( roleDDL->getListBox()->getItemSelectedIndex() ) );
		auto text = codeEditor->getDocument().getText().toUtf8();

		if ( text.empty() )
			continue;

		j.push_back( { { "role", role }, { "content", std::move( text ) } } );
	}
	return j;
}

Uint32 LLMChatUI::onMessage( const NodeMessage* msg ) {
	if ( msg->getMsg() == NodeMessage::Focus ) {
		getPlugin()->getPluginContext()->getSplitter()->setCurrentWidget( this );
	}
	return 0;
}

nlohmann::json LLMChatUI::serializeChat( const LLMModel& model ) {
	nlohmann::json j = {
		{ "model", model.name }, { "stream", true }, { "messages", chatToJson() } };
	if ( model.maxOutputTokens )
		j["max_tokens"] = *model.maxOutputTokens;
	return j;
}

nlohmann::json LLMChatUI::serialize() {
	nlohmann::json j;
	j["uuid"] = mUUID.toString();
	j["chat"] = serializeChat( mCurModel );
	return j;
}

void LLMChatUI::unserialize( const nlohmann::json& /*payload*/ ) {}

void LLMChatUI::saveChat() {
	auto plugin = getPlugin();
	if ( plugin == nullptr )
		return;

	std::string pluginsStatePath( plugin->getManager()->getPluginsPath() + "plugins_state" +
								  FileSystem::getOSSlash() + "aiassistant" +
								  FileSystem::getOSSlash() );
	if ( !FileSystem::fileExists( pluginsStatePath ) )
		FileSystem::makeDir( pluginsStatePath, true );
	std::string path( pluginsStatePath + mSummary + ".json" );
	FileSystem::fileWrite( path, serialize().dump( 2 ) );
}

std::string LLMChatUI::prepareApiUrl( const std::string& apiKey ) {
	const auto& provider = mProviders[mCurModel.provider];
	std::string url = provider.apiUrl;
	String::replaceAll( url, "${model}", mCurModel.name );
	String::replaceAll( url, "${api_key}", apiKey );
	return url;
}

void LLMChatUI::doRequest() {
	if ( mRequest )
		return;

	auto apiKey = AIAssistantPlugin::getApiKeyFromProvider( mCurModel.provider, getPlugin() );
	if ( !apiKey ) {
		showMsg( getUISceneNode()->i18n( "configure_api_key",
										 "You must first configure your provider api key." ) );
		return;
	}
	std::string apiKeyStr{ *apiKey };

	mChatRun->setVisible( false )->setEnabled( false );
	mChatStop->setVisible( true )->setEnabled( true );

	UIWidget* chat = addChatUI( LLMChat::Role::Assistant );
	toggleEnableChats( false );

	auto model = mCurModel;
	auto* editor = chat->findByClass<UICodeEditor>( "data_ui" );
	std::string apiUrl( prepareApiUrl( apiKeyStr ) );
	mRequest = std::make_unique<LLMChatCompletionRequest>(
		apiUrl, apiKeyStr, serializeChat( model ).dump(), model.provider );
	mRequest->streamedResponseCb = [this, editor]( const std::string& chunk ) {
		auto conversation = chunk;
		editor->runOnMainThread( [this, conversation = std::move( conversation ), editor] {
			editor->getDocument().textInput( String::fromUtf8( conversation ) );
			editor->setCursorVisible( false );
			resizeToFit( editor );
		} );
	};
	mRequest->doneCb = [this, editor, apiUrl = std::move( apiUrl ),
						apiKeyStr = std::move( apiKeyStr ), model = std::move( model )](
						   const LLMChatCompletionRequest&, Http::Response& response ) {
		auto status = response.getStatus();
		auto statusDesc = response.getStatusDescription();

		runOnMainThread( [this, editor, status, statusDesc, apiUrl = std::move( apiUrl ),
						  apiKeyStr = std::move( apiKeyStr ), model = std::move( model )] {
			if ( status != Http::Response::Ok ) {
				auto resp = nlohmann::json::parse( mRequest->getStream(), nullptr, false );
				if ( resp.contains( "error" ) && resp["error"].contains( "message" ) ) {
					showMsg( resp["error"].value( "message", "" ) );
				} else if ( resp.contains( "error" ) && resp["error"].is_string() ) {
					showMsg( URI::decode( resp.value( "error", "" ) ) );
				} else {
					showMsg( statusDesc );
				}

				removeLastChat();
			}
			mRequest.reset();
			toggleEnableChats( true );

			mChatStop->setVisible( false )->setEnabled( false );
			mChatRun->setVisible( true )->setEnabled( true );

			if ( editor->hasFocus() )
				mChatInput->setFocus();

			if ( !mSummaryRequest && mSummary.empty() && status == Http::Response::Ok ) {
				static const std::string SummaryPrompt =
					"Generate a concise 3-7 word title for this conversation, omitting "
					"punctuation. Go "
					"straight to the title, without any preamble and prefix like `Here's a concise "
					"suggestion:...` or `Title:`. Ignore this message for the summary generation.";

				auto jchat = serializeChat( getCheapestModelFromCurrentProvider() );

				jchat["messages"].push_back(
					{ { "role", LLMChat::roleToString( LLMChat::Role::User ) },
					  { "content", SummaryPrompt } } );

				auto chatstr = jchat.dump();

				mSummaryRequest = std::make_unique<LLMChatCompletionRequest>(
					apiUrl, apiKeyStr, chatstr, model.provider );

				mSummaryRequest->doneCb = [this]( const LLMChatCompletionRequest& req,
												  Http::Response& response ) {
					auto status = response.getStatus();
					if ( status == Http::Response::Ok ) {
						mSummary = req.getResponse();
						saveChat();
					}
					runOnMainThread( [this] { mSummaryRequest.reset(); } );
				};
				mSummaryRequest->requestAsync();
			} else {
				saveChat();
			}
		} );
	};
	mRequest->requestAsync();
}

void LLMChatUI::toggleEnableChat( UIWidget* chat, bool enabled ) {
	chat->findByClass( "role_ui" )->setEnabled( enabled );
	UICodeEditor* editor = chat->findByClass( "data_ui" )->asType<UICodeEditor>();
	editor->setEnabled( enabled );
	editor->setLocked( !enabled );
	chat->findByClass( "erase_but" )->setEnabled( enabled );
	chat->findByClass( "move_up" )->setEnabled( enabled );
	chat->findByClass( "move_down" )->setEnabled( enabled );
}

void LLMChatUI::toggleEnableChats( bool enabled ) {
	auto chats = mChatsList->findAllByClass( "llm_conversation" );
	for ( auto chat : chats )
		toggleEnableChat( chat, enabled );
}

Drawable* LLMChatUI::findIcon( const std::string& name, const size_t iconSize ) {
	if ( name.empty() )
		return nullptr;
	UIIcon* icon = getUISceneNode()->findIcon( name );
	if ( icon )
		return icon->getSize( iconSize );
	return nullptr;
}

UIWidget* LLMChatUI::addChatUI( LLMChat::Role role ) {
	find( "chat_presentation" )->setVisible( false );

	UIWidget* chat =
		mChatsList->getUISceneNode()->loadLayoutFromString( DEFAULT_CHAT_GLOBE, mChatsList );
	auto* roleDDL = chat->findByClass( "role_ui" )->asType<UIDropDownList>();
	auto* roleListBox = chat->findByClass( "role_ui" )->asType<UIDropDownList>()->getListBox();
	switch ( role ) {
		case LLMChat::Role::System:
			roleListBox->setSelected( 2 );
			break;
		case LLMChat::Role::User:
			chat->findByClass( "llm_conversation" )->addClass( "user" );
			roleListBox->setSelected( 0 );
			break;
		case LLMChat::Role::Assistant:
			roleListBox->setSelected( 1 );
			break;
		case LLMChat::Role::Tool:
			break;
	}
	roleListBox->on( Event::OnItemSelected, [roleDDL, chat]( auto ) {
		if ( roleDDL->getListBox()->getItemSelectedIndex() == 0 ) {
			chat->addClass( "user" );
		} else {
			chat->removeClass( "user" );
		}
	} );
	auto* editor = chat->findByClass<UICodeEditor>( "data_ui" );
	const auto& markdown = SyntaxDefinitionManager::instance()->getByLSPName( "markdown" );
	editor->setDisableCursorBlinkingAfterAMinuteOfInactivity( false );
	editor->setCursorBlinkTime( Time::Zero );
	editor->setSyntaxDefinition( markdown );
	editor->setShowFoldingRegion( true );
	editor->getDocument().getFoldRangeService().setEnabled( true );
	editor->setFoldDrawable( findIcon( "chevron-down", PixelDensity::dpToPxI( 12 ) ) );
	editor->setFoldedDrawable( findIcon( "chevron-right", PixelDensity::dpToPxI( 12 ) ) );

	if ( getPlugin() ) {
		editor->setColorScheme(
			getPlugin()->getPluginContext()->getSplitter()->getCurrentColorScheme() );
	}

	editor->on( Event::OnSizeChange, [editor, this]( auto ) { resizeToFit( editor ); } );
	editor->on( Event::OnVisibleLinesCountChange,
				[editor, this]( auto ) { resizeToFit( editor ); } );
	chat->findByClass( "erase_but" )->onClick( [chat, this]( auto ) {
		chat->close();
		auto chats = findAllByClass( "llm_conversation" );
		if ( chats.empty() )
			find( "chat_presentation" )->setVisible( true );
	} );
	chat->findByClass( "move_up" )->onClick( [chat]( auto ) {
		if ( chat->getNodeIndex() > 0 )
			chat->toPosition( chat->getNodeIndex() - 1 );
	} );
	chat->findByClass( "move_down" )->onClick( [chat]( auto ) {
		if ( chat->getNodeIndex() < chat->getParent()->getChildCount() - 1 )
			chat->toPosition( chat->getNodeIndex() + 1 );
	} );
	resizeToFit( editor );
	return chat;
}

void LLMChatUI::addChat( LLMChat::Role role, std::string conversation ) {
	UIWidget* chat = addChatUI( role );
	auto* editor = chat->findByClass<UICodeEditor>( "data_ui" );
	editor->getDocument().textInput( String::fromUtf8( conversation ) );
	editor->setCursorVisible( false );
	resizeToFit( editor );
}

void LLMChatUI::removeLastChat() {
	auto chats = mChatsList->findAllByClass( "llm_conversation" );
	if ( !chats.empty() ) {
		auto* chat = chats[chats.size() - 1];
		auto* editor = chat->findByClass<UICodeEditor>( "data_ui" );
		if ( editor->getDocument().isEmpty() )
			chat->close();
	}
}

void LLMChatUI::setProviders( LLMProviders&& providers ) {
	mProviders = std::move( providers );
}

void LLMChatUI::showMsg( String msg ) {
	auto msgBox = UIMessageBox::New( UIMessageBox::OK, msg );
	msgBox->getTextBox()->setTextSelection( true );
	msgBox->getTextBox()->onClick(
		[]( const MouseEvent* event ) {
			auto tv = event->getNode()->asType<UITextView>();
			tv->getUISceneNode()->getWindow()->getClipboard()->setText( tv->getText() );
		},
		EE_BUTTON_RIGHT );
	msgBox->showWhenReady();
}

AIAssistantPlugin* LLMChatUI::getPlugin() {
	auto plugin = mManager->get( "aiassistant" );
	if ( plugin )
		return reinterpret_cast<AIAssistantPlugin*>( plugin );
	return nullptr;
}

const LLMModel& LLMChatUI::getCheapestModelFromCurrentProvider() const {
	auto providerIt = mProviders.find( mCurModel.provider );
	if ( providerIt != mProviders.end() ) {
		for ( const auto& model : providerIt->second.models )
			if ( model.cheapest )
				return model;
	}
	return mCurModel;
}

} // namespace ecode
