#include "chatui.hpp"

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
	lh: 16dp;
	padding-top: 0dp;
	padding-bottom: 0dp;
	border: 0;
	background-color: var(--tab-back);
}
.llm_chatui .llm_topbar > * {
	layout-gravity: center;
}
.llm_chatui .settings_but {
	tint: var(--floating-icon);
}
.llm_chatui .llm_topbar PushButton {
	margin-bottom: 1dp;
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
.llm_chatui > .llm_topbar {
	border-bottom: 1dp var(--tab-line);
	margin-bottom: 4dp;
	padding-left: 4dp;
	padding-right: 4dp;
}
PushButton.llm_button.primary {
	text-as-fallback: false;
}
</style>
<vbox class="llm_chatui"  lw="mp" lh="mp">
	<hbox class="llm_topbar" lw="mp" lh="wc">
		<TextView text="@string(llm_model, LLM Model:)" margin-right="4dp" />
		<DropDownList class="model_ui" lw="350dp" selected-index="0"></DropDownList>
		<PushButton id="refresh_model_ui" text="@string(refresh_model_ui, Refresh)" icon="icon(refresh, 14dp)" />
		<Widget lw="0" lw8="1" />
		<PushButton class="settings_but" text="@string(settings, Settings)" icon="icon(settings, 14dp)" tooltip="@string(settings, Settings)" />
	</hbox>
	<Splitter lw="mp" lh="0" lw8="1" orientation="vertical" splitter-partition="80%" padding="4dp">
		<ScrollView lw="mp" class="llm_chat_scrollview">
			<vbox lw="mp" class="llm_chats"></vbox>
		</ScrollView>
		<RelativeLayout lw="mp" class="llm_controls">
			<CodeEditor class="llm_chat_input" lw="mp" lh="mp" />
			<PushButton id="llm_user" class="llm_button" text="@string(user, User)" min-width="60dp" layout_gravity="bottom|left" layout_margin="8dp" />
			<PushButton class="llm_button" text="@string(attach, Attach)" icon="icon(attach, 14dp)" min-width="32dp" layout_gravity="bottom|left" layout-to-right-of="llm_user" />
			<PushButton id="llm_run" class="llm_button primary" text="@string(run, Run)" icon="icon(play-filled, 14dp)" layout_gravity="bottom|right" layout-margin="8dp" />
			<PushButton id="llm_add_chat" class="llm_button" text="@string(add, Add)" icon="icon(add, 15dp)" min-width="32dp" layout-to-left-of="llm_run" />
			<PushButton id="llm_stop" class="llm_button primary" text="@string(stop, Stop)" icon="icon(stop, 12dp)" layout_gravity="bottom|right" layout-margin="8dp" visible="false" enabled="false" />
		</RelativeLayout>
	</Splitter>
</vbox>
)xml";

static const char* DEFAULT_CHAT_GLOBE = R"xml(
<vbox class="llm_conversation" lw="mp" lh="wc">
	<hbox class="llm_conversation_opt">
		<DropDownList class="role_ui" lw="150dp" selected-index="1">
			<item>@string(User, user)</item>
			<item>@string(Assistant, assistant)</item>
			<item>@string(system, System)</item>
		</DropDownList>
		<PushButton class="move_up" text="@string(move_up, Move Up)" icon="icon(arrow-up-s, 12dp)" tooltip="@string(move_up, Move Up)" />
		<PushButton class="move_down" text="@string(move_down, Move Down)" icon="icon(arrow-down-s, 12dp)"  tooltip="@string(move_down, Move Down)" />
		<PushButton class="erase_but" text="@string(remove_chat, Remove Chat)" icon="icon(chrome-close, 10dp)" tooltip="@string(remove_chat, Remove Chat)" />
	</hbox>
	<CodeEditor class="data_ui" lw="mp" lh="32dp" />
</vbox>
)xml";

ChatUI::ChatUI( UISceneNode* ui, LLMProviders providers ) {
	setProviders( std::move( providers ) );

	mChatUI = ui->loadLayoutFromString( DEFAULT_LAYOUT );
	mChatsList = mChatUI->findByClass( "llm_chats" );
	mModelDDL = mChatUI->findByClass<UIDropDownList>( "model_ui" );

	mChatUI->find( "refresh_model_ui" )->onClick( [this]( auto ) { fillApiModels( mModelDDL ); } );

	fillModelDropDownList( mModelDDL );

	mChatScrollView = mChatUI->findByClass( "llm_chat_scrollview" )->asType<UIScrollView>();
	mChatScrollView->getVerticalScrollBar()->setValue( 1 );

	mChatInput = mChatUI->findByClass<UICodeEditor>( "llm_chat_input" );
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
		auto chats = mChatUI->findAllByClass( "llm_conversation" );

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
				showMsg( mChatUI->getUISceneNode()->i18n(
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

	mChatUI->find( "llm_add_chat" )->onClick( [this]( auto ) {
		mChatInput->getDocument().execute( "add_chat" );
	} );

	const auto& markdown = SyntaxDefinitionManager::instance()->getByLSPName( "markdown" );
	mChatInput->setShowFoldingRegion( true );
	mChatInput->getDocument().getFoldRangeService().setEnabled( true );
	mChatInput->setFoldDrawable( findIcon( "chevron-down", PixelDensity::dpToPxI( 12 ) ) );
	mChatInput->setFoldedDrawable( findIcon( "chevron-right", PixelDensity::dpToPxI( 12 ) ) );

	mChatInput->setSyntaxDefinition( markdown );

	mChatRun = mChatUI->find<UIPushButton>( "llm_run" );
	mChatRun->onClick( [this]( auto ) { mChatInput->getDocument().execute( "prompt" ); } );

	mChatStop = mChatUI->find<UIPushButton>( "llm_stop" );
	mChatStop->onClick( [this]( auto ) { mChatInput->getDocument().execute( "prompt-stop" ); } );

	mChatUserRole = mChatUI->find<UIPushButton>( "llm_user" );
	mChatUserRole->onClick( [this]( auto ) {
		if ( mChatUserRole->getText() == mChatUserRole->i18n( "user", "User" ) ) {
			mChatUserRole->setText( mChatUserRole->i18n( "assistant", "Assistant" ) );
		} else if ( mChatUserRole->getText() == mChatUserRole->i18n( "assistant", "Assistant" ) ) {
			mChatUserRole->setText( mChatUserRole->i18n( "system", "System" ) );
		} else if ( mChatUserRole->getText() == mChatUserRole->i18n( "system", "System" ) ) {
			mChatUserRole->setText( mChatUserRole->i18n( "user", "User" ) );
		}
	} );

	mChatUI->on( Event::OnClose, [this]( auto ) { eeDelete( this ); } );
}

void ChatUI::fillApiModels( UIDropDownList* modelDDL ) {
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

void ChatUI::fillModelDropDownList( UIDropDownList* modelDDL ) {
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

void ChatUI::resizeToFit( UICodeEditor* editor ) {
	Float visibleLineCount = editor->getDocumentView().getVisibleLinesCount();
	Float lineHeight = editor->getLineHeight();
	Float height = lineHeight * visibleLineCount + editor->getPixelsPadding().Top +
				   editor->getPixelsPadding().Bottom;
	editor->setPixelsSize( editor->getPixelsSize().getWidth(), height );
}

nlohmann::json ChatUI::chatToJson( const std::string& /*provider*/ ) {
	auto j = nlohmann::json::array();
	auto chats = mChatUI->findAllByClass( "llm_conversation" );
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

nlohmann::json ChatUI::serialize( const std::string& /*provider*/ ) {
	nlohmann::json j = { { "model", mCurModel.name },
						 { "stream", true },
						 { "messages", chatToJson( mCurModel.provider ) } };
	if ( mCurModel.maxOutputTokens )
		j["max_tokens"] = *mCurModel.maxOutputTokens;
	return j;
}

void unserialize( const nlohmann::json& /*payload*/ ) {}

const char* ChatUI::getApiKeyFromProvider( const std::string& provider ) {
	static const char* OPEN_API_KEY = "";
	if ( provider == "openai" )
		return getenv( "OPENAI_API_KEY" );
	if ( provider == "anthropic" )
		return getenv( "ANTHROPIC_API_KEY" );
	if ( provider == "google" ) {
		const char* apiKey = getenv( "GOOGLE_AI_API_KEY" );
		if ( apiKey != nullptr )
			return apiKey;
		return getenv( "GEMINI_API_KEY" );
	}
	if ( provider == "deepseek" )
		return getenv( "DEEPSEEK_API_KEY" );
	if ( provider == "mistral" )
		return getenv( "MISTRAL_API_KEY" );
	if ( provider == "lmstudio" || provider == "ollama" )
		return OPEN_API_KEY;
	if ( provider == "xai" ) {
		const char* apiKey = getenv( "XAI_API_KEY" );
		if ( apiKey != nullptr )
			return apiKey;
		return getenv( "GROK_API_KEY" );
	}
	return nullptr;
}

std::string ChatUI::prepareApiUrl( const std::string& apiKey ) {
	const auto& provider = mProviders[mCurModel.provider];
	std::string url = provider.apiUrl;
	String::replaceAll( url, "${model}", mCurModel.name );
	String::replaceAll( url, "${api_key}", apiKey );
	return url;
}

void ChatUI::doRequest() {
	if ( mRequest )
		return;

	const char* apiKey = getApiKeyFromProvider( mCurModel.provider );
	if ( apiKey == nullptr ) {
		showMsg( mChatUI->getUISceneNode()->i18n(
			"configure_api_key", "You must first configure your provider api key." ) );
		return;
	}
	std::string apiKeyStr{ apiKey };

	mChatRun->setVisible( false )->setEnabled( false );
	mChatStop->setVisible( true )->setEnabled( true );

	UIWidget* chat = addChatUI( LLMChat::Role::Assistant );
	toggleEnableChats( false );

	auto* editor = chat->findByClass<UICodeEditor>( "data_ui" );
	mRequest = std::make_unique<LLMChatCompletionRequest>( prepareApiUrl( apiKeyStr ), apiKeyStr,
														   serialize( mCurModel.provider ).dump(),
														   mCurModel.provider );
	mRequest->streamedResponseCb = [this, editor]( const std::string& chunk ) {
		auto conversation = chunk;
		editor->runOnMainThread( [this, conversation = std::move( conversation ), editor] {
			editor->getDocument().textInput( String::fromUtf8( conversation ) );
			editor->setCursorVisible( false );
			resizeToFit( editor );
		} );
	};
	mRequest->doneCb = [this, editor]( const LLMChatCompletionRequest&, Http::Response& response ) {
		auto status = response.getStatus();
		auto statusDesc = response.getStatusDescription();

		mChatUI->runOnMainThread( [this, editor, status, statusDesc] {
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
		} );
	};
	mRequest->requestAsync();
}

void ChatUI::toggleEnableChat( UIWidget* chat, bool enabled ) {
	chat->findByClass( "role_ui" )->setEnabled( enabled );
	UICodeEditor* editor = chat->findByClass( "data_ui" )->asType<UICodeEditor>();
	editor->setEnabled( enabled );
	editor->setLocked( !enabled );
	chat->findByClass( "erase_but" )->setEnabled( enabled );
	chat->findByClass( "move_up" )->setEnabled( enabled );
	chat->findByClass( "move_down" )->setEnabled( enabled );
}

void ChatUI::toggleEnableChats( bool enabled ) {
	auto chats = mChatsList->findAllByClass( "llm_conversation" );
	for ( auto chat : chats )
		toggleEnableChat( chat, enabled );
}

Drawable* ChatUI::findIcon( const std::string& name, const size_t iconSize ) {
	if ( name.empty() )
		return nullptr;
	UIIcon* icon = mChatUI->getUISceneNode()->findIcon( name );
	if ( icon )
		return icon->getSize( iconSize );
	return nullptr;
}

UIWidget* ChatUI::addChatUI( LLMChat::Role role ) {
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

	editor->on( Event::OnSizeChange, [editor, this]( auto ) { resizeToFit( editor ); } );
	editor->on( Event::OnVisibleLinesCountChange,
				[editor, this]( auto ) { resizeToFit( editor ); } );
	chat->findByClass( "erase_but" )->onClick( [chat]( auto ) { chat->close(); } );
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

void ChatUI::addChat( LLMChat::Role role, std::string conversation ) {
	UIWidget* chat = addChatUI( role );
	auto* editor = chat->findByClass<UICodeEditor>( "data_ui" );
	editor->getDocument().textInput( String::fromUtf8( conversation ) );
	editor->setCursorVisible( false );
	resizeToFit( editor );
}

void ChatUI::removeLastChat() {
	auto chats = mChatsList->findAllByClass( "llm_conversation" );
	if ( !chats.empty() ) {
		auto* chat = chats[chats.size() - 1];
		auto* editor = chat->findByClass<UICodeEditor>( "data_ui" );
		if ( editor->getDocument().isEmpty() )
			chat->close();
	}
}

void ChatUI::setProviders( LLMProviders&& providers ) {
	mProviders = std::move( providers );
}

UIWidget* ChatUI::getChatUI() {
	return mChatUI;
}

void ChatUI::showMsg( String msg ) {
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

} // namespace ecode
