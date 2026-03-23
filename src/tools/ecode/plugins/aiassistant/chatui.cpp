#include "chatui.hpp"
#include "../../appconfig.hpp"
#include "../../notificationcenter.hpp"
#include "../../projectdirectorytree.hpp"
#include "../../widgetcommandexecuter.hpp"
#include "aiassistantplugin.hpp"
#include "chathistory.hpp"

#include <eepp/system/filesystem.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>
#include <eepp/ui/models/itemlistmodel.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uidropdownlist.hpp>
#include <eepp/ui/uidropdownmodellist.hpp>
#include <eepp/ui/uiicon.hpp>
#include <eepp/ui/uiloader.hpp>
#include <eepp/ui/uimarkdownview.hpp>
#include <eepp/ui/uimessagebox.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollview.hpp>
#include <eepp/ui/uitableview.hpp>
#include <eepp/ui/uitextview.hpp>
#include <eepp/ui/uitooltip.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <eepp/window/clipboard.hpp>
#include <eepp/window/window.hpp>
#include <eterm/ui/uiterminal.hpp>

#include <chrono>
#include <nlohmann/json.hpp>

using namespace EE::System;
using namespace EE::Window;

namespace ecode {

class LLMModelsModel : public Model {
  public:
	enum Columns { Name, Provider, Hash };

	LLMModelsModel( const std::vector<LLMModel>& models, UISceneNode* uiSceneNode = nullptr ) :
		mModels( models ), mUISceneNode( uiSceneNode ) {
		mCurModels.reserve( mModels.size() );
		for ( const auto& model : mModels ) {
			mCurModels.emplace_back( &model );
		}
	}

	virtual size_t rowCount( const ModelIndex& = ModelIndex() ) const override {
		return mCurModels.size();
	}

	virtual size_t columnCount( const ModelIndex& = ModelIndex() ) const override { return 3; }

	virtual std::string columnName( const size_t& column ) const override {
		switch ( column ) {
			case Columns::Name:
				return mUISceneNode ? mUISceneNode->i18n( "name", "Name" ) : "Name";
			case Columns::Provider:
				return mUISceneNode ? mUISceneNode->i18n( "provider", "Provider" ) : "Provider";
			case Columns::Hash:
				return mUISceneNode ? mUISceneNode->i18n( "hash", "Hash" ) : "Hash";
		}
		return "";
	}

	virtual Variant data( const ModelIndex& index,
						  ModelRole role = ModelRole::Display ) const override {
		if ( role != ModelRole::Display )
			return {};

		if ( index.row() < 0 || static_cast<size_t>( index.row() ) >= mCurModels.size() )
			return {};

		const auto& model = *mCurModels[index.row()];

		switch ( index.column() ) {
			case Columns::Name: {
				if ( model.displayName.has_value() && !model.displayName->empty() ) {
					return Variant( model.displayName->c_str() );
				}
				return Variant( model.name.c_str() );
			}
			case Columns::Provider: {
				return Variant( model.provider.c_str() );
			}
			case Columns::Hash: {
				return Variant( static_cast<Uint64>( model.hash ) );
			}
		}

		return {};
	}

	ModelIndex getFromHash( Uint64 hash ) {
		auto it = std::find_if( mCurModels.begin(), mCurModels.end(),
								[hash]( const LLMModel* model ) { return hash == model->hash; } );
		return it != mCurModels.end() ? index( std::distance( mCurModels.begin(), it ) )
									  : index( 0 );
	}

	void setFilter( const std::string& filter ) {
		if ( mCurFilter == filter )
			return;
		mCurFilter = filter;
		mCurModels.clear();

		for ( const auto& model : mModels ) {
			if ( filter.empty() ) {
				mCurModels.emplace_back( &model );
				continue;
			}

			bool matchesName = String::icontains( model.name, filter );
			bool matchesDisplayName =
				model.displayName.has_value() && String::icontains( *model.displayName, filter );
			bool matchesProvider = String::icontains( model.provider, filter );

			if ( matchesName || matchesDisplayName || matchesProvider ) {
				mCurModels.emplace_back( &model );
			}
		}

		invalidate();
	}

	const std::vector<const LLMModel*>& getCurModels() const { return mCurModels; }

	void refresh() { setFilter( mCurFilter ); }

  protected:
	const std::vector<LLMModel>& mModels;
	std::vector<const LLMModel*> mCurModels;
	std::string mCurFilter;
	UISceneNode* mUISceneNode;
};

class ACPAgentsModel : public Model {
  public:
	enum Columns { Name, Command };

	ACPAgentsModel( const std::map<std::string, ACPAgent>& agents,
					UISceneNode* uiSceneNode = nullptr ) :
		mAgents( agents ), mUISceneNode( uiSceneNode ) {
		mCurAgents.reserve( mAgents.size() );
		for ( const auto& [name, agent] : mAgents ) {
			if ( agent.enabled )
				mCurAgents.emplace_back( name );
		}
	}

	virtual size_t rowCount( const ModelIndex& = ModelIndex() ) const override {
		return mCurAgents.size();
	}

	virtual size_t columnCount( const ModelIndex& = ModelIndex() ) const override { return 2; }

	virtual std::string columnName( const size_t& column ) const override {
		switch ( column ) {
			case Columns::Name:
				return mUISceneNode ? mUISceneNode->i18n( "name", "Name" ) : "Name";
			case Columns::Command:
				return mUISceneNode ? mUISceneNode->i18n( "command", "Command" ) : "Command";
		}
		return "";
	}

	virtual Variant data( const ModelIndex& index,
						  ModelRole role = ModelRole::Display ) const override {
		if ( role != ModelRole::Display )
			return {};

		if ( index.row() < 0 || static_cast<size_t>( index.row() ) >= mCurAgents.size() )
			return {};

		const auto& agentName = mCurAgents[index.row()];
		auto it = mAgents.find( agentName );
		if ( it == mAgents.end() )
			return {};

		switch ( index.column() ) {
			case Columns::Name: {
				return Variant( it->second.name.c_str() );
			}
			case Columns::Command: {
				return Variant( it->second.command.c_str() );
			}
		}

		return {};
	}

	ModelIndex getFromName( const std::string& name ) {
		auto it =
			std::find_if( mCurAgents.begin(), mCurAgents.end(),
						  [&name]( const std::string& agentName ) { return name == agentName; } );
		return it != mCurAgents.end() ? index( std::distance( mCurAgents.begin(), it ) )
									  : index( 0 );
	}

	void setFilter( const std::string& filter ) {
		if ( mCurFilter == filter )
			return;
		mCurFilter = filter;
		mCurAgents.clear();

		for ( const auto& [name, agent] : mAgents ) {
			if ( !agent.enabled )
				continue;

			if ( filter.empty() ) {
				mCurAgents.emplace_back( name );
				continue;
			}

			bool matchesName = String::icontains( agent.name, filter );
			bool matchesCommand = String::icontains( agent.command, filter );

			if ( matchesName || matchesCommand ) {
				mCurAgents.emplace_back( name );
			}
		}

		invalidate();
	}

	const std::vector<std::string>& getCurAgents() const { return mCurAgents; }

	void refresh() { setFilter( mCurFilter ); }

  protected:
	const std::map<std::string, ACPAgent>& mAgents;
	std::vector<std::string> mCurAgents;
	std::string mCurFilter;
	UISceneNode* mUISceneNode;
};

class AgentSessionHistoryModel : public Model {
  public:
	enum Columns { Title, UpdatedAt, SessionId };

	AgentSessionHistoryModel( const std::vector<acp::SessionInfo>& sessions,
							  UISceneNode* uiSceneNode = nullptr ) :
		mSessions( sessions ), mUISceneNode( uiSceneNode ) {
		mCurSessions.reserve( mSessions.size() );
		for ( const auto& session : mSessions ) {
			mCurSessions.emplace_back( &session );
		}
	}

	virtual size_t rowCount( const ModelIndex& = ModelIndex() ) const override {
		return mCurSessions.size();
	}

	virtual size_t columnCount( const ModelIndex& = ModelIndex() ) const override { return 3; }

	virtual std::string columnName( const size_t& column ) const override {
		switch ( column ) {
			case Columns::Title:
				return mUISceneNode ? mUISceneNode->i18n( "title", "Title" ) : "Title";
			case Columns::UpdatedAt:
				return mUISceneNode ? mUISceneNode->i18n( "updated_at", "Updated At" )
									: "Updated At";
			case Columns::SessionId:
				return mUISceneNode ? mUISceneNode->i18n( "session_id", "Session ID" )
									: "Session ID";
		}
		return "";
	}

	virtual Variant data( const ModelIndex& index,
						  ModelRole role = ModelRole::Display ) const override {
		if ( role != ModelRole::Display && role != ModelRole::Custom )
			return {};

		if ( index.row() < 0 || static_cast<size_t>( index.row() ) >= mCurSessions.size() )
			return {};

		const auto& session = *mCurSessions[index.row()];

		if ( role == ModelRole::Custom ) {
			return Variant( session.sessionId.c_str() );
		}

		switch ( index.column() ) {
			case Columns::Title: {
				return Variant( session.title.empty()
									? ( mUISceneNode ? mUISceneNode->i18n( "untitled_session",
																		   "Untitled Session" )
													 : "Untitled Session" )
									: session.title.c_str() );
			}
			case Columns::UpdatedAt: {
				return Variant( session.updatedAt.c_str() );
			}
			case Columns::SessionId: {
				return Variant( session.sessionId.c_str() );
			}
		}

		return {};
	}

	void setFilter( const std::string& filter ) {
		if ( mCurFilter == filter )
			return;
		mCurFilter = filter;
		mCurSessions.clear();

		for ( const auto& session : mSessions ) {
			if ( filter.empty() ) {
				mCurSessions.emplace_back( &session );
				continue;
			}

			bool matchesTitle = String::icontains( session.title, filter );
			bool matchesId = String::icontains( session.sessionId, filter );

			if ( matchesTitle || matchesId ) {
				mCurSessions.emplace_back( &session );
			}
		}

		invalidate();
	}

  protected:
	std::vector<acp::SessionInfo> mSessions;
	std::vector<const acp::SessionInfo*> mCurSessions;
	std::string mCurFilter;
	UISceneNode* mUISceneNode;
};

static const char* DEFAULT_PROVIDER = "google";
static const char* DEFAULT_MODEL = "gemini-2.5-flash";

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

LLMChat::Role LLMChat::stringToRole( const std::string& roleStr ) {
	if ( roleStr == "user" ) {
		return Role::User;
	} else if ( roleStr == "assistant" ) {
		return Role::Assistant;
	} else if ( roleStr == "system" ) {
		return Role::System;
	}
	return Role::User;
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
<![CDATA[
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
.llm_thought,
.llm_plan {
	background-color: var(--list-back);
	padding: 4dp 4dp 4dp 8dp;
	border-left: 2dp solid var(--tab-line);
}
CodeEditor.llm_chat_input {
	font-family: monospace;
	border-radius: 8dp;
	padding-bottom: 38dp;
}
.llm_chatui PushButton,
.llm_chatui SelectButton {
	border-color: transparent;
	text-as-fallback: true;
}
.llm_chatui PushButton:hover,
.llm_chatui SelectButton:hover,
.llm_chatui .permission_options PushButton:focus,
.llm_chatui .permission_options SelectButton:focus {
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
.llm_chatui .image {
	tint: var(--font);
}
.llm_chat_attach,
.llm_chat_select_model,
.llm_chat_select_agent {
	padding: 8dp 8dp 38dp 8dp;
}
.llm_chat_locate_input,
.llm_chat_select_model_input,
.llm_chat_select_agent_input {
	margin-bottom: 2dp;
	padding: 0 0 0 4dp;
}
.llm_chat_attach_locate,
.llm_chat_model_locate,
.llm_chat_agent_locate {
	border-radius: 8dp;
	margin-bottom: 4dp;
}
.llm_chat_input {
	background-image: none;
}
.llm_chat_input.incognito {
	background-image: icon(spy-line, 16dp);
	background-position: top 8dp right 8dp;
}
.llm_chatui DropDownList,
.model_ui,
.agent_ui {
	border-color: transparent;
	background-color: var(--tab-back);
}
.llm_chatui DropDownList:hover,
.model_ui:hover,
.agent_ui:hover {
	border-color: var(--primary);
}
.model_ui,
.agent_ui {
	padding-right: 16dp;
	text-align: left;
	text-overflow: ellipsis;
	expand-text: true;
	foreground-image: url("data:image/svg,<svg viewBox='0 0 24 24' fill='white'><path d='M12 15.6315L20.9679 10.8838L20.0321 9.11619L12 13.3685L3.96788 9.11619L3.0321 10.8838L12 15.6315Z'></path></svg>");
	foreground-position-x: right 6dp;
	foreground-position-y: center 1dp;
	foreground-size: 12dp 16dp;
	foreground-tint: var(--icon);
}
.tool_permission {
	background-color: var(--list-back);
	border-radius: 8dp;
	padding: 4dp;
}
.tool_permission .llm_conversation_opt {
	background-color: var(--back);
	border-radius: 4dp;
}
.tool_permission .permission_options {
	margin-top: 8dp;
}
.tool_permission .permission_options > PushButton {
	margin-right: 8dp;
}
.agent_config_label {
	font-weight: bold;
	margin-bottom: 4dp;
}
.agent_config_dropdown {
	margin-bottom: 8dp;
}
.llm_chats Terminal {
	margin-top: 8dp;
}
]]>
</style>
<Splitter lw="mp" lh="mp" orientation="vertical" splitter-partition="75%" padding="4dp">
	<RelativeLayout lw="mp">
		<ScrollView lw="mp" lh="mp" class="llm_chat_scrollview">
			<vbox lw="mp" lh="wc" class="llm_chats"></vbox>
		</ScrollView>
		<vbox id="chat_presentation" lw="wc" lh="wc" layout-gravity="center" gravity="center">
			<Image class="image" icon="icon(robot-2, 72dp)" gravity="center" layout-gravity="center" margin-bottom="16dp" />
			<TextView text="@string(ai_llm_presentation, What can I help with?)" font-size="24dp" />
		</vbox>
	</RelativeLayout>
	<RelativeLayout lw="mp" class="llm_controls" clip="true">
		<CodeEditor class="llm_chat_input" lw="mp" lh="mp" />
		<vboxce class="llm_chat_attach" lw="mp" lh="mp" visible="false">
			<TableView lw="mp" lh="0dp" lw8="1" class="llm_chat_attach_locate" />
			<TextInput class="llm_chat_locate_input" lw="mp" lh="18dp" hint='@string(type_to_locate, "Type to Locate")' />
		</vboxce>
		<vboxce class="llm_chat_select_model" lw="mp" lh="mp" visible="false">
			<TableView lw="mp" lh="0dp" lw8="1" class="llm_chat_model_locate" />
			<TextInput class="llm_chat_select_model_input" lw="mp" lh="18dp" hint='@string(select_a_model_ellipsis, "Select a model...")' />
		</vboxce>
		<vboxce class="llm_chat_select_agent" lw="mp" lh="mp" visible="false">
			<TableView lw="mp" lh="0dp" lw8="1" class="llm_chat_agent_locate" />
			<TextInput class="llm_chat_select_agent_input" lw="mp" lh="18dp" hint='@string(select_an_agent_ellipsis, "Select an agent...")' />
		</vboxce>
		<hbox lw="mp" lh="wc" layout_gravity="bottom|left" layout_margin="8dp" clip="false">
			<PushButton id="llm_user" class="llm_button" text="@string(user, User)" tooltip="@string(change_role, Change Role)" min-width="60dp" margin-right="4dp" />
			<PushButton id="llm_attach" class="llm_button" text="@string(add_context, Add Context)" tooltip="@string(add_context, Add Context)" icon="icon(attach, 14dp)" min-width="32dp" margin-right="4dp" />
			<PushButton id="llm_chat_history" class="llm_button" text="@string(chat_history, Chat History)" tooltip="@string(chat_history, Chat History)" icon="icon(chat-history, 14dp)" min-width="32dp" margin-right="4dp" />
			<PushButton id="llm_more" class="llm_button" tooltip="@string(more_options, More Options)" icon="icon(more-fill, 14dp)" min-width="32dp" />
			<PushButton class="model_ui" lw="0" lw8="1" lh="mp" margin-left="4dp" margin-right="4dp" tooltip="@string(select_model, Select Model)" />
			<PushButton class="agent_ui" lw="0" lw8="1" lh="mp" margin-left="4dp" margin-right="4dp" tooltip="@string(select_agent, Select Agent)" visible="false" />
			<SelectButton id="llm_agent_mode" class="llm_button" tooltip="@string(toggle_agent_mode, Toggle Agent Mode)" icon="icon(robot-2, 14dp)" min-width="32dp" margin-right="4dp" select-on-click="true" />
			<PushButton class="agent_config_ui" tooltip="@string(agent_config, Agent Config)" icon="icon(agent, 14dp)" min-width="32dp" margin-right="4dp" visible="false" />
			<PushButton id="llm_settings_but" class="llm_button" text="@string(settings, Settings)" tooltip="@string(settings, Settings)" icon="icon(settings, 14dp)" min-width="32dp" margin-right="4dp" />
			<PushButton id="llm_add_chat" class="llm_button" text="@string(add, Add)" tooltip="@string(add_message, Add Message)" icon="icon(add, 15dp)" min-width="32dp" margin-right="4dp" />
			<PushButton id="llm_run" class="llm_button primary" text="@string(run, Run)" tooltip="@string(add_message_and_run_prompt, Add Message and Run Prompt)" icon="icon(play-filled, 14dp)" />
			<PushButton id="llm_stop" class="llm_button primary" text="@string(stop, Stop)" icon="icon(stop, 12dp)" min-width="32dp" visible="false" enabled="false" />
		</hbox>
	</RelativeLayout>
</Splitter>
)xml";

static const char* DEFAULT_PLAN_GLOBE = R"xml(
	<MarkdownView class="llm_conversation llm_plan" lw="mp" lh="wc" />
)xml";

static const char* DEFAULT_TOOL_CALL_GLOBE = R"xml(
	<MarkdownView class="llm_conversation llm_tool_call" lw="mp" lh="wc" />
)xml";

static const char* DEFAULT_THINKING_GLOBE = R"xml(
	<MarkdownView class="llm_conversation llm_thought" lw="mp" lh="wc" />
)xml";

static const char* DEFAULT_CHAT_GLOBE = R"xml(
<vbox class="llm_conversation" lw="mp" lh="wc">
	<hbox class="llm_conversation_opt">
		<DropDownList class="role_ui" lw="150dp" lh="16dp" selected-index="1">
			<item>@string(user, User)</item>
			<item>@string(assistant, Assistant)</item>
			<item>@string(system, System)</item>
		</DropDownList>
		<PushButton class="copy_contents" text="@string(copy_contents, Copy Contents)" icon="icon(copy, 12dp)" tooltip="@string(copy_contents, Copy Contents)" />
		<PushButton class="move_up" text="@string(move_up, Move Up)" icon="icon(arrow-up-s, 12dp)" tooltip="@string(move_up, Move Up)" />
		<PushButton class="move_down" text="@string(move_down, Move Down)" icon="icon(arrow-down-s, 12dp)"  tooltip="@string(move_down, Move Down)" />
		<PushButton class="erase_but" text="@string(remove_chat, Remove Chat)" icon="icon(chrome-close, 10dp)" tooltip="@string(remove_chat, Remove Chat)" />
	</hbox>
	<CodeEditor class="data_ui" lw="mp" lh="32dp"><Image class="image thinking" icon="icon(loader-2, 24dp)" visible="false" /></CodeEditor>
</vbox>
)xml";

static const char* DEFAULT_PERMISSION_GLOBE = R"xml(
<vbox class="llm_conversation tool_permission" lw="mp" lh="wc">
	<hbox class="llm_conversation_opt" lw="mp" lh="wc" padding="4dp">
		<TextView text="@string(tool_call_permission_request, Tool Call Permission Request)" font-style="bold" margin-left="4dp" />
	</hbox>
	<vbox class="data_ui" lw="mp" lh="wc" padding="8dp">
		<MarkdownView class="permission_desc" lw="mp" lh="wc" />
		<StackLayout class="permission_options" lw="mp" lh="wc" />
	</vbox>
</vbox>
)xml";

LLMChatUI::LLMChatUI( PluginManager* manager ) :
	UILinearLayout(), WidgetCommandExecuter( getInput() ), mManager( manager ) {
	setClass( "llm_chatui" );
	setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent );

	mChatSplitter = getUISceneNode()
						->loadLayoutFromString( DEFAULT_LAYOUT, this,
												String::hash( "ai_assistant_plugin_chat_ui" ) )
						->asType<UISplitter>();

	mChatsList = findByClass( "llm_chats" );
	mModelBtn = findByClass<UIPushButton>( "model_ui" );
	mModelBtn->onClick( [this]( auto ) { execute( "ai-select-model" ); } );
	mAgentBtn = findByClass<UIPushButton>( "agent_ui" );
	mAgentBtn->onClick( [this]( auto ) { execute( "ai-select-agent" ); } );
	mAgentConfigBtn = findByClass<UIPushButton>( "agent_config_ui" );
	mAgentConfigBtn->onClick( [this]( auto ) { showAgentConfigWindow(); } );

	mChatAgentMode = find<UISelectButton>( "llm_agent_mode" );
	mChatAgentMode->on( Event::OnValueChange, [this]( auto ) {
		mIsAgentMode = mChatAgentMode->isSelected();
		updateTabTitle();
		updateAgentModeUI();
	} );

	mChatMore = find<UIPushButton>( "llm_more" );
	mChatMore->onClick( [this]( auto ) { execute( "ai-show-menu" ); } );

	mChatSettings = find<UIPushButton>( "llm_settings_but" );
	mChatSettings->onClick( [this]( auto ) { execute( "ai-settings" ); } );

	mChatScrollView = findByClass( "llm_chat_scrollview" )->asType<UIScrollView>();
	mChatScrollView->getVerticalScrollBar()->setValue( 1 );

	mChatInput = findByClass<UICodeEditor>( "llm_chat_input" );
	mChatInput->setId( String::format( "chat_input_%p", mChatInput ) );

	on( Event::OnFocus, [this]( auto ) { mChatInput->setFocus(); } );

	if ( getPlugin() ) {
		mChatInput->setColorScheme(
			getPlugin()->getPluginContext()->getSplitter()->getCurrentColorScheme() );
	}

	mChatAdd = find<UIPushButton>( "llm_add_chat" );
	mChatAdd->onClick( [this]( auto ) {
		execute( "ai-add-chat" );
		if ( mChatAdd->getTooltip() && mChatAdd->getTooltip()->isVisible() )
			mChatAdd->getTooltip()->hide();
	} );

	const auto& markdown = SyntaxDefinitionManager::instance()->getByLSPName( "markdown" );
	mChatInput->setShowFoldingRegion( true );
	mChatInput->getDocument().getFoldRangeService().setEnabled( true );
	mChatInput->setFoldDrawable( findIcon( "chevron-down", PixelDensity::dpToPxI( 12 ) ) );
	mChatInput->setFoldedDrawable( findIcon( "chevron-right", PixelDensity::dpToPxI( 12 ) ) );
	mChatInput->setAllowSelectingTextFromGutter( false );
	mChatInput->setFindReplaceEnabled( true );

	mChatInput->setSyntaxDefinition( markdown );

	mChatInput->on( Event::OnTextChanged, [this]( const Event* ) {
		if ( !mIsAgentMode || mAvailableCommands.empty() )
			return;
		auto& doc = mChatInput->getDocument();
		auto cursor = doc.getSelection().start();
		auto lineText = doc.getLineTextUtf8( cursor.line() );
		if ( ( cursor.column() == 0 || cursor.column() == 1 ) && !lineText.empty() &&
			 lineText == "/\n" ) {
			showSlashCommands();
		}
	} );

	mChatRun = find<UIPushButton>( "llm_run" );
	mChatRun->onClick( [this]( auto ) {
		execute( "ai-prompt" );
		if ( mChatRun->getTooltip() && mChatRun->getTooltip()->isVisible() )
			mChatRun->getTooltip()->hide();
	} );

	mChatStop = find<UIPushButton>( "llm_stop" );
	mChatStop->onClick( [this]( auto ) {
		execute( "ai-prompt-stop" );
		if ( mChatStop->getTooltip() && mChatStop->getTooltip()->isVisible() )
			mChatStop->getTooltip()->hide();
	} );

	mChatUserRole = find<UIPushButton>( "llm_user" );
	mChatUserRole->onClick( [this]( auto ) { execute( "ai-chat-toggle-role" ); } );

	auto setCmd = [this]( const std::string& name, const CommandCallback& cb ) {
		setCommand( name, cb );
		mChatInput->getDocument().setCommand( name, cb );
	};

	setCmd( "ai-chat-toggle-role", [this] {
		if ( mChatUserRole->getText() == mChatUserRole->i18n( "user", "User" ) ) {
			mChatUserRole->setText( mChatUserRole->i18n( "assistant", "Assistant" ) );
		} else if ( mChatUserRole->getText() == mChatUserRole->i18n( "assistant", "Assistant" ) ) {
			mChatUserRole->setText( mChatUserRole->i18n( "system", "System" ) );
		} else if ( mChatUserRole->getText() == mChatUserRole->i18n( "system", "System" ) ) {
			mChatUserRole->setText( mChatUserRole->i18n( "user", "User" ) );
		}
	} );

	setCmd( "ai-settings", [this] {
		if ( getPlugin() )
			getPlugin()->getPluginContext()->focusOrLoadFile( getPlugin()->getFileConfigPath() );
	} );

	setCmd( "ai-add-chat", [this] {
		if ( mChatInput->getDocument().isEmpty() )
			return;

		addChat( LLMChat::stringToRole( mChatUserRole ),
				 mChatInput->getDocument().getText().toUtf8() );
		mChatInput->getDocument().selectAll();
		mChatInput->getDocument().textInput( String{} );
		mChatInput->setFocus();
		mChatScrollView->getVerticalScrollBar()->setValue( 1 );
	} );

	setCmd( "ai-prompt", [this] {
		// "ai-prompt-stop"
		if ( mRequest || ( mAgentSession && mAgentSession->isPrompting() ) ) {
			if ( mRequest && !mRequest->isCancelled() ) {
				mRequest->cancel();
				return;
			} else if ( mAgentSession && mAgentSession->isPrompting() ) {
				mAgentSession->cancel();
				return;
			} else {
				mRequest.reset();
			}
		}

		auto chats = findAllByClass( "llm_conversation" );

		if ( chats.empty() && mChatInput->getDocument().isEmpty() )
			return;

		auto inputUserRole = LLMChat::stringToRole( mChatUserRole );
		if ( !mIsAgentMode && ( ( !chats.empty() && ( mChatInput->getDocument().isEmpty() ||
													  inputUserRole != LLMChat::Role::User ) ) ||
								( chats.empty() && inputUserRole != LLMChat::Role::User ) ) ) {
			auto rolePicker = chats[chats.size() - 1]->findByClass( "role_ui" );
			if ( rolePicker &&
				 rolePicker->asType<UIDropDownList>()->getListBox()->getItemSelectedIndex() != 0 ) {
				showMsg( getUISceneNode()->i18n(
					"llm_last_message_must_be_from_user",
					"The last chat message must be from a \"User\" role" ) );
				return;
			}
		}

		execute( "ai-add-chat" );

		if ( mIsAgentMode ) {
			doAgentRequest();
		} else {
			doRequest();
		}
	} );

	setCmd( "ai-prompt-stop", [this] {
		if ( mRequest )
			mRequest->cancel();
	} );

	setCmd( "ai-clone-chat", [this] {
		if ( getPlugin() == nullptr )
			return;
		auto chats = findAllByClass( "llm_conversation" );
		if ( chats.empty() ) {
			getPlugin()->getPluginContext()->getNotificationCenter()->addNotification(
				i18n( "nothing_to_clone", "Nothing to Clone" ) );
			return;
		}
		auto* chatUI = getPlugin()->newAIAssistant();
		auto input = chatUI->unserialize( serialize() );
		chatUI->mUUID = UUID();
		chatUI->mSummary += i18n( "chat_cloned", " (cloned)" );
		if ( !input.empty() )
			chatUI->mChatInput->getDocument().textInput( input );
		chatUI->updateTabTitle();
		chatUI->setFocus();
	} );

	setCmd( "ai-save-chat", [this] { saveChat(); } );

	setCmd( "ai-chat-history", [this] { showChatHistory(); } );

	setCmd( "ai-attach-file", [this] {
		mLinkMode = false;
		showAttachFile();
		mLocateInput->getDocument().selectAll();
	} );

	setCmd( "ai-link-file", [this] {
		mLinkMode = true;
		showAttachFile();
		mLocateInput->getDocument().selectAll();
	} );

	setCmd( "ai-select-model", [this] {
		if ( mIsAgentMode ) {
			execute( "ai-select-agent" );
			return;
		}

		if ( !mLocateModelBarLayout->isVisible() ) {
			if ( mLocateModelTable->getModel() ) {
				mLocateModelInput->setText( "" );
				static_cast<LLMModelsModel*>( mLocateModelTable->getModel() )->setFilter( "" );
			}

			showSelectModel();

			mLocateModelTable->runOnMainThread( [this] {
				auto model = static_cast<LLMModelsModel*>( mLocateModelTable->getModel() );
				if ( model )
					mLocateModelTable->setSelection( model->getFromHash( mCurModel.hash ) );
			} );
		} else
			hideSelectModel();
	} );

	setCmd( "ai-select-agent", [this] {
		if ( !mLocateAgentBarLayout->isVisible() ) {
			if ( mLocateAgentTable->getModel() ) {
				mLocateAgentInput->setText( "" );
				static_cast<ACPAgentsModel*>( mLocateAgentTable->getModel() )->setFilter( "" );
			}

			showSelectAgent();

			mLocateAgentTable->runOnMainThread( [this] {
				auto model = static_cast<ACPAgentsModel*>( mLocateAgentTable->getModel() );
				if ( model ) {
					mLocateAgentTable->setSelection( model->getFromName( mCurAgent ) );
					mAgentBtn->setText( mCurAgent );
				}
			} );
		} else
			hideSelectAgent();
	} );

	setCmd( "ai-toggle-private-chat", [this] {
		mChatIsPrivate = !mChatIsPrivate;

		if ( mChatIsPrivate )
			mChatInput->addClass( "incognito" );
		else
			mChatInput->removeClass( "incognito" );
	} );

	setCmd( "ai-toggle-lock-chat", [this] { renameChat( mSummary, true ); } );

	setCmd( "ai-rename-chat", [this] {
		UIMessageBox* msgBox = UIMessageBox::New(
			UIMessageBox::INPUT, i18n( "rename_conversation", "Rename Conversation" ) );
		msgBox->on( Event::OnConfirm, [this, msgBox]( const Event* ) {
			std::string newName( msgBox->getTextInput()->getText().toUtf8() );
			if ( newName.empty() || mSummary == newName )
				return;
			msgBox->closeWindow();
			renameChat( newName );
		} );
		msgBox->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
		msgBox->center();
		auto summary = mSummary;
		String::replaceAll( summary, "\n", " " );
		String::trimInPlace( summary );
		msgBox->getTextInput()->setText( summary );
		msgBox->showWhenReady();
	} );

	setCmd( "ai-regenerate-title", [this] { regenerateChatName(); } );

	setCmd( "new-ai-assistant", [this] {
		if ( getPlugin() == nullptr )
			return;
		getPlugin()->newAIAssistant()->setFocus();
	} );

	setCmd( "ai-show-add-context-menu", [this] {
		if ( getPlugin() == nullptr )
			return;
		UIPopUpMenu* menu = UIPopUpMenu::New();

		menu->add( i18n( "attach_file", "Attach File" ),
				   getUISceneNode()->findIconDrawable( "attach", PixelDensity::dpToPxI( 12 ) ),
				   getKeyBindings().getCommandKeybindString( "ai-attach-file" ) )
			->setId( "ai-attach-file" );

		menu->add( i18n( "link_file", "Link File" ),
				   getUISceneNode()->findIconDrawable( "link", PixelDensity::dpToPxI( 12 ) ),
				   getKeyBindings().getCommandKeybindString( "ai-link-file" ) )
			->setTooltipText( i18n( "link_file_tooltip",
									"Unlike Attach File (which captures a snapshot),\n"
									"Link File always refers to the current version of the file,\n"
									"even when you re-run the same prompt later.\n"
									"Acts like a symbolic link." ) )
			->setId( "ai-link-file" );

		menu->on( Event::OnItemClicked, [this]( const Event* event ) {
			UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
			std::string id( item->getId() );
			execute( id );
		} );

		menu->runOnMainThread( [this, menu] {
			auto pos( mChatAttach->getScreenPos() );
			UIMenu::findBestMenuPos( pos, menu, nullptr, nullptr, mChatAttach );
			menu->showAtScreenPosition( pos );
		} );
	} );

	setCmd( "ai-show-menu", [this] {
		if ( getPlugin() == nullptr )
			return;
		UIPopUpMenu* menu = UIPopUpMenu::New();

		menu->add(
				i18n( "new_conversation", "New Conversation" ),
				getUISceneNode()->findIconDrawable( "chat-sparkle", PixelDensity::dpToPxI( 12 ) ),
				getPlugin()
					->getPluginContext()
					->getMainLayout()
					->getKeyBindings()
					.getCommandKeybindString( "new-ai-assistant" ) )
			->setId( "new-ai-assistant" );

		menu->add(
				i18n( "save_conversation", "Save Conversation" ),
				getUISceneNode()->findIconDrawable( "document-save", PixelDensity::dpToPxI( 12 ) ),
				getKeyBindings().getCommandKeybindString( "ai-save-chat" ) )
			->setId( "ai-save-chat" );

		menu->add( i18n( "rename_conversation", "Rename Conversation" ),
				   getUISceneNode()->findIconDrawable( "file-edit", PixelDensity::dpToPxI( 12 ) ),
				   getKeyBindings().getCommandKeybindString( "ai-rename-chat" ) )
			->setId( "ai-rename-chat" );

		menu->add( i18n( "clone_current_conversation", "Clone Current Conversation" ),
				   getUISceneNode()->findIconDrawable( "file-copy", PixelDensity::dpToPxI( 12 ) ),
				   getKeyBindings().getCommandKeybindString( "ai-clone-chat" ) )
			->setId( "ai-clone-chat" );

		menu->addSeparator();

		menu->addCheckBox( i18n( "lock_chat_memory", "Lock Chat Memory" ), mChatLocked,
						   getKeyBindings().getCommandKeybindString( "ai-toggle-lock-chat" ) )
			->setTooltipText(
				i18n( "lock_chat_memory_tooltip",
					  "Lock a chat memory to avoid being deleted during memory clean-ups.\nChat "
					  "memory will only be able to be deleted manually in the chat history." ) )
			->setId( "ai-toggle-lock-chat" );

		menu->addCheckBox( i18n( "toggle_private_chat", "Toggle Private Chat" ), mChatIsPrivate,
						   getKeyBindings().getCommandKeybindString( "ai-toggle-private-chat" ) )
			->setTooltipText( i18n( "toggle_private_chat_tooltip",
									"Private chats won't be saved into the chat history." ) )
			->setId( "ai-toggle-private-chat" );

		menu->addSeparator();

		if ( chatExistsInDisk() ) {
			menu->add( i18n( "regenerate_conversation_title", "Regenerate Conversation Title" ),
					   getUISceneNode()->findIconDrawable( "refresh", PixelDensity::dpToPxI( 12 ) ),
					   getKeyBindings().getCommandKeybindString( "ai-regenerate-title" ) )
				->setId( "ai-regenerate-title" );
		}

		menu->add( i18n( "refresh_model_ui", "Refresh Local Models" ),
				   getUISceneNode()->findIconDrawable( "refresh", PixelDensity::dpToPxI( 12 ) ),
				   getKeyBindings().getCommandKeybindString( "ai-refresh-local-models" ) )
			->setId( "ai-refresh-local-models" );

		menu->on( Event::OnItemClicked, [this]( const Event* event ) {
			UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
			std::string id( item->getId() );
			execute( id );
		} );

		menu->runOnMainThread( [this, menu] {
			auto pos( mChatMore->getScreenPos() );
			UIMenu::findBestMenuPos( pos, menu, nullptr, nullptr, mChatMore );
			menu->showAtScreenPosition( pos );
		} );
	} );

	setCmd( "ai-refresh-local-models", [this] { fillApiModels(); } );

	mChatHistory = find<UIPushButton>( "llm_chat_history" );
	mChatHistory->onClick( [this]( auto ) { showChatHistory(); } );

	mChatAttach = find<UIPushButton>( "llm_attach" );
	mChatAttach->onClick( [this]( auto ) { execute( "ai-show-add-context-menu" ); } );

	if ( getPlugin() == nullptr )
		return;

	initAttachFile();
	initSelectModel();
	initSelectAgent();

	auto providers = getPlugin()->getProviders();
	setProviders( std::move( providers ) );
	mCurModel = getDefaultModel();

	mAgents = getPlugin()->getAgents();

	AppConfig& config = getPlugin()->getPluginContext()->getConfig();
	auto partition = config.iniState.getValue( "aiassistant", "split_partition", "" );

	if ( !partition.empty() )
		mChatSplitter->setSplitPartition( StyleSheetLength::fromString( partition ) );

	auto modelProvider = config.iniState.getValue( "aiassistant", "default_provider", "" );
	auto modelName = config.iniState.getValue( "aiassistant", "default_model", "" );
	auto agentName = config.iniState.getValue( "aiassistant", "default_agent", "" );

	if ( !modelProvider.empty() && !modelName.empty() ) {
		auto modelOpt = getModel( modelProvider, modelName );
		if ( modelOpt ) {
			mCurModel = *modelOpt;
		}
	}

	if ( !agentName.empty() && mAgents.find( agentName ) != mAgents.end() ) {
		selectAgent( agentName );
	} else if ( !mAgents.empty() ) {
		selectAgent( mAgents.begin()->first );
	}

	fillModelDropDownList();

	const auto appendShortcutToTooltip = [this]( UIPushButton* but, const std::string& cmd ) {
		auto kb = getKeyBindings().getCommandKeybindString( cmd );
		if ( kb.empty() )
			return;
		but->setTooltipText( but->getTooltipText() + " (" + kb + ")" );
	};

	bindCmds( mChatInput, true );

	appendShortcutToTooltip( mChatHistory, "ai-chat-history" );
	appendShortcutToTooltip( mChatAttach, "ai-show-add-context-menu" );
	appendShortcutToTooltip( mChatRun, "ai-prompt" );
	appendShortcutToTooltip( mChatStop, "ai-prompt" );
	appendShortcutToTooltip( mChatAdd, "ai-add-chat" );
	appendShortcutToTooltip( mChatSettings, "ai-settings" );
	appendShortcutToTooltip( mChatMore, "ai-show-menu" );
	appendShortcutToTooltip( mChatUserRole, "ai-chat-toggle-role" );
	appendShortcutToTooltip( mModelBtn, "ai-select-model" );

	addKb( mChatInput, "mod+keypad enter", "ai-prompt", true, false );
	addKb( mChatInput, "mod+shift+keypad enter", "ai-add-chat", true, false );
}

LLMChatUI::~LLMChatUI() {
	if ( mRequest ) {
		mRequest->cancelCb = nullptr;
		mRequest->doneCb = nullptr;
		mRequest->streamedResponseCb = nullptr;
	}
	if ( mSummaryRequest ) {
		mSummaryRequest->cancelCb = nullptr;
		mSummaryRequest->doneCb = nullptr;
		mSummaryRequest->streamedResponseCb = nullptr;
	}
	if ( getPlugin() ) {
		AIAssistantPlugin::AIAssistantConfig config;
		config.partition = getSplitter()->getSplitPartition();
		config.modelProvider = mCurModel.provider;
		config.modelName = mCurModel.name;
		config.agentName = mCurAgent;
		getPlugin()->setConfig( std::move( config ) );
	}
}

void LLMChatUI::addKb( UICodeEditor* editor, std::string kb, const std::string& cmd,
					   bool bindToChatUI, bool searchDefined ) {
	if ( searchDefined && getPlugin() ) {
		const auto& find = getPlugin()->getKeybindings().find( cmd );
		if ( find != getPlugin()->getKeybindings().end() && !find->second.empty() )
			kb = find->second;
	}
	if ( bindToChatUI ) {
		getKeyBindings().addKeybindString( kb, cmd );
		editor->addKeyBindingString( kb, cmd, true );
	} else {
		// Chat UI editors besides the main chat input need to actually remove the keybindings
		// to let the parent take over the command
		editor->removeUnlockedCommand( cmd );
		editor->getKeyBindings().removeKeybind( kb );
	}
}

void LLMChatUI::bindCmds( UICodeEditor* editor, bool bindToChatUI ) {
	addKb( editor, "mod+return", "ai-prompt", bindToChatUI );
	addKb( editor, "mod+h", "ai-chat-history", bindToChatUI );
	addKb( editor, "mod+shift+c", "ai-clone-chat", bindToChatUI );
	addKb( editor, "mod+shift+s", "ai-settings", bindToChatUI );
	addKb( editor, "mod+shift+p", "ai-toggle-private-chat", bindToChatUI );
	addKb( editor, "mod+s", "ai-save-chat", bindToChatUI );
	addKb( editor, "f2", "ai-rename-chat", bindToChatUI );
	addKb( editor, "mod+m", "ai-show-menu", bindToChatUI );
	addKb( editor, "mod+shift+r", "ai-chat-toggle-role", bindToChatUI );
	addKb( editor, "mod+shift+l", "ai-refresh-local-models", bindToChatUI );
	addKb( editor, "mod+shift+a", "ai-attach-file", bindToChatUI );
	addKb( editor, "mod+shift+z", "ai-link-file", bindToChatUI );
	addKb( editor, "mod+shift+x", "ai-select-model", bindToChatUI );

	if ( bindToChatUI )
		addKb( editor, "mod+shift+return", "ai-add-chat", bindToChatUI );
}

std::optional<LLMModel> LLMChatUI::getModel( const std::string& provider,
											 const std::string& modelName ) {
	auto providerIt = mProviders.find( provider );
	if ( providerIt != mProviders.end() ) {
		const auto& models = providerIt->second.models;
		auto modelIt =
			std::find_if( models.begin(), models.end(), [&modelName]( const LLMModel& model ) {
				return model.name == modelName;
			} );
		if ( modelIt != models.end() )
			return *modelIt;
	}
	return {};
}

std::optional<LLMModel> LLMChatUI::getModel( Uint64 hash ) {
	auto modelIt = std::find_if( mModels.begin(), mModels.end(),
								 [hash]( const LLMModel& model ) { return hash == model.hash; } );
	if ( modelIt != mModels.end() )
		return *modelIt;
	return {};
}

void LLMChatUI::showChatHistory() {
	auto plugin = getPlugin();
	if ( plugin == nullptr )
		return;

	hideAttachFile();
	hideSelectModel();
	hideSelectAgent();

	static const char* CHAT_HISTORY_LAYOUT = R"xml(
		<window window-flags="default|shadow|modal|ephemeral|maximize"
				window-title="@string(ai_conversations_history, AI Conversations History)">
			<Loader id="loader" radius="48" outline-thickness="6dp" indeterminate="true" />
			<vbox lw="mp" lh="mp">
				<hbox id="top_bar" lw="mp" lh="wc">
					<TextInput lw="0dp" lw8="1" lh="wc" id="search_input" hint="@string(search_chat_ellipsis, Search Chat...)" visible="false" />
					<PushButton lw="wc" lh="wc" id="clear_history" text="@string(ai_clear_history, Clear History...)" visible="false" margin-left="4dp" />
				</hbox>
				<hbox id="delete_history_cont" lw="mp" lh="wc" visible="false">
					<TextView text="@string(delete_history_older_than_ellipsis, Delete Conversations Older Than...)" layout-gravity="center_vertical" />
					<TextInput id="days_num" lw="0dp" lw8="1" lh="wc" hint="@string(number_of_days, Number of Days)" margin-left="4dp" margin-right="4dp" layout-gravity="center_vertical" numeric="true" />
					<PushButton id="delete_but" text="@string(delete, Delete)" layout-gravity="center_vertical" />
				</hbox>
				<TableView id="table" lw="mp" lh="0" lw8="1" visible="false" />
			</vbox>
		</window>
	)xml";

	UIWindow* win =
		getUISceneNode()->loadLayoutFromString( CHAT_HISTORY_LAYOUT )->asType<UIWindow>();
	if ( mIsAgentMode ) {
		win->setTitle( i18n( "ai_agent_sessions_history", "AI Agent Sessions" ) );
	}
	UITextInput* input = win->find( "search_input" )->asType<UITextInput>();
	UILoader* loader = win->find( "loader" )->asType<UILoader>();
	UITableView* tv = win->find( "table" )->asType<UITableView>();
	UIPushButton* clearHistory = win->find( "clear_history" )->asType<UIPushButton>();
	UILinearLayout* deleteCont = win->find( "delete_history_cont" )->asType<UILinearLayout>();
	UITextInput* daysNum = win->find( "days_num" )->asType<UITextInput>();
	UIPushButton* deleteChatsBut = win->find( "delete_but" )->asType<UIPushButton>();

	if ( mIsAgentMode ) {
		clearHistory->setVisible( false );
		clearHistory->setEnabled( false );
	}

	win->setMinWindowSize( getUISceneNode()->getSize().getWidth() * 0.5f,
						   getUISceneNode()->getSize().getHeight() * 0.7f );
	win->setKeyBindingCommand( "closeWindow", [win, this] {
		win->closeWindow();
		setFocus();
	} );
	win->getKeyBindings().addKeybind( { KEY_ESCAPE }, "closeWindow" );
	win->center();
	win->getModalWidget()->addClass( "shadowbg" );
	win->setId( UUID().toString() );
	win->on( Event::OnWindowReady, [win]( auto ) { win->setFocus(); } );

	loader->center();

	tv->setAutoColumnsWidth( true );
	tv->setFitAllColumnsToWidget( true );
	if ( !mIsAgentMode ) {
		tv->setSetupCellCb( [this, tv, win, input]( UITableCell* cell ) {
			if ( cell->getCurIndex().column() == ChatHistoryModel::Delete ) {
				cell->onClick( [this, tv, cell, win, input]( const MouseEvent* ) {
					ChatHistoryModel* model = static_cast<ChatHistoryModel*>( tv->getModel() );
					auto summary = model
									   ->data( model->index( cell->getCurIndex().row(),
															 ChatHistoryModel::Summary ) )
									   .toString();
					auto msgBox = UIMessageBox::New(
						UIMessageBox::OK_CANCEL,
						String::format( i18n( "confirm_del_llm_chat",
											  "Are you sure you want to remove \"%s?\"" )
											.toUtf8(),
										summary ) );
					msgBox->setParent( win );
					msgBox->center();
					msgBox->showWhenReady();
					msgBox->on( Event::OnConfirm, [model, cell, input]( auto ) {
						model->remove( cell->getCurIndex() );
						input->setFocus();
					} );
				} );
			}
		} );
	}

	input->on( Event::KeyDown, [tv, input]( const Event* event ) {
		tv->forceKeyDown( *event->asKeyEvent() );
		input->setFocus();
	} );

	input->on( Event::OnTextChanged, [this, tv, input]( const Event* ) {
		if ( mIsAgentMode ) {
			AgentSessionHistoryModel* model =
				static_cast<AgentSessionHistoryModel*>( tv->getModel() );
			model->setFilter( input->getText().toUtf8() );
			if ( tv->getSelection().isEmpty() && model->rowCount() > 0 )
				tv->setSelection( model->index( 0, 0 ) );
		} else {
			ChatHistoryModel* model = static_cast<ChatHistoryModel*>( tv->getModel() );
			model->setFilter( input->getText().toUtf8() );
			if ( tv->getSelection().isEmpty() && !model->getCurHistory().empty() )
				tv->setSelection( model->index( 0, 0 ) );
		}
	} );

	const auto openCurrentSelectedModelItem = [this, tv] {
		ModelIndex index;
		if ( !tv->getSelection().isEmpty() )
			index = tv->getSelection().first();
		else if ( tv->getModel() && tv->getModel()->rowCount() > 0 )
			index = tv->getModel()->index( 0, 0 );
		if ( getPlugin() == nullptr || !index.isValid() )
			return;

		if ( mIsAgentMode ) {
			auto* model = static_cast<const AgentSessionHistoryModel*>( tv->getModel() );
			auto sessionId =
				model
					->data( model->index( index.row(), AgentSessionHistoryModel::SessionId ),
							ModelRole::Custom )
					.toString();
			if ( !sessionId.empty() ) {
				auto* chatUI = getPlugin()->newAIAssistant();
				nlohmann::json fakePayload = { { "agent_mode", true },
											   { "agent_name", mCurAgent },
											   { "session_id", sessionId } };
				chatUI->unserialize( fakePayload );
				chatUI->setFocus();
			}
		} else {
			auto* model = static_cast<const ChatHistoryModel*>( tv->getModel() );
			auto* chatUI = getPlugin()->newAIAssistant();
			auto path = model->data( model->index( index.row(), ChatHistoryModel::Path ) );
			std::string data;
			FileSystem::fileGet( path.toString(), data );
			nlohmann::json j = nlohmann::json::parse( data, nullptr, false );
			auto input = chatUI->unserialize( j );
			if ( !input.empty() )
				chatUI->mChatInput->getDocument().textInput( input );
			chatUI->setFocus();
		}
	};

	input->on( Event::OnPressEnter, [win, openCurrentSelectedModelItem]( const Event* ) {
		win->executeKeyBindingCommand( "closeWindow" );
		openCurrentSelectedModelItem();
	} );

	tv->on( Event::OnModelEvent, [win, openCurrentSelectedModelItem]( const Event* event ) {
		const ModelEvent* modelEvent = static_cast<const ModelEvent*>( event );
		if ( modelEvent->getModelEventType() == ModelEventType::Open ) {
			win->executeKeyBindingCommand( "closeWindow" );
			openCurrentSelectedModelItem();
		}
	} );

	clearHistory->onClick(
		[deleteCont]( auto ) { deleteCont->setVisible( !deleteCont->isVisible() ); } );

	std::string winId = win->getId();
	UISceneNode* uiSceneNode = getUISceneNode();

	if ( mIsAgentMode ) {
		auto listSessionsCb = [winId, uiSceneNode, tv, loader,
							   input]( const std::vector<acp::SessionInfo>& sessions,
									   const std::optional<acp::ResponseError>& err ) {
			uiSceneNode->runOnMainThread( [winId, uiSceneNode, tv, loader, input, sessions, err] {
				auto win = uiSceneNode->find( winId );
				if ( win == nullptr )
					return;
				if ( err ) {
					loader->setVisible( false );
					win->asType<UIWindow>()->closeWindow();
					return;
				}
				auto model = std::make_shared<AgentSessionHistoryModel>( sessions, uiSceneNode );
				loader->setVisible( false );
				input->setVisible( true );
				tv->setVisible( true );
				tv->setModel( model );
				input->setFocus();
			} );
		};

		if ( !mAgentSession ) {
			setupAgentSession();

			mAgentSession->start( [this, winId, uiSceneNode, loader, listSessionsCb]( bool ready ) {
				if ( !ready ) {
					uiSceneNode->runOnMainThread( [loader, winId, uiSceneNode] {
						if ( uiSceneNode->find( winId ) == nullptr )
							return;
						loader->setVisible( false );
					} );
					return;
				}
				mAgentSession->listSessions( listSessionsCb );
			} );
		} else {
			mAgentSession->listSessions( listSessionsCb );
		}
	} else {
		getUISceneNode()->getThreadPool()->run(
			[plugin, loader, input, tv, winId = std::move( winId ), uiSceneNode, clearHistory] {
				std::string conversationsPath = plugin->getConversationsPath();
				auto model = std::make_shared<ChatHistoryModel>(
					ChatHistory::getHistory( conversationsPath ), uiSceneNode );
				if ( uiSceneNode->find( winId ) == nullptr ) // Window closed?
					return;
				tv->runOnMainThread( [tv, loader, input, model, clearHistory] {
					loader->setVisible( false );
					input->setVisible( true );
					tv->setVisible( true );
					clearHistory->setVisible( true );
					tv->setModel( model );
					tv->setColumnsVisible( { ChatHistoryModel::Summary, ChatHistoryModel::DateTime,
											 ChatHistoryModel::Delete } );
					input->setFocus();
				} );
			} );
	}

	deleteChatsBut->onClick( [this, daysNum]( auto ) {
		int days = 0;
		if ( daysNum->getText().empty() || !String::fromString( days, daysNum->getText() ) ||
			 days < 0 )
			return;
		auto msgBox = UIMessageBox::New(
			UIMessageBox::OK_CANCEL,
			String::format( i18n( "ai_confirm_delete_chats",
								  "Are you sure you want to delete the conversations older than %d "
								  "days?\nThis operation cannot be reverted!" )
								.toUtf8(),
							days ) );

		msgBox->on( Event::OnConfirm, [this, days]( auto ) {
			deleteOldConversations( days );
			showChatHistory();
		} );

		msgBox->center();
		msgBox->showWhenReady();
	} );
}

void LLMChatUI::fillApiModels() {
	mPendingModelsToLoad = 0;
	mNewModels.clear();

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
			model.name = el.contains( "model" ) ? el.value( "model", "" )
												: ( el.contains( "id" ) ? el.value( "id", "" )
																		: el.value( "key", "" ) );

			if ( el.contains( "name" ) )
				model.displayName = el.value( "name", "" );

			if ( !model.displayName && el.contains( "display_name" ) )
				model.displayName = el.value( "display_name", "" );

			model.isEphemeral = true;
			model.hash = hashCombine( std::hash<std::string>()( model.name ),
									  std::hash<std::string>()( model.provider ) );

			if ( model.name.empty() )
				continue;

			if ( el.contains( "max_context_length" ) )
				model.maxOutputTokens = el.value( "max_context_length", 0 );

			data.models.emplace_back( model );
			mNewModels.push_back( model );
		}

		mPendingModelsToLoad++;

		runOnMainThread( [this] {
			mPendingModelsToLoad--;
			if ( mPendingModelsToLoad == 0 ) {
				mModels.erase(
					std::remove_if( mModels.begin(), mModels.end(),
									[]( const LLMModel& model ) { return model.isEphemeral; } ),
					mModels.end() );

				mModels.insert( mModels.end(), mNewModels.begin(), mNewModels.end() );

				if ( mLocateModelTable && mLocateModelTable->getModel() )
					loadSelectModel();
				onInit();
			}
		} );
	}

	if ( mPendingModelsToLoad == 0 )
		onInit();
}

String LLMChatUI::getModelDisplayName( const LLMModel& model ) const {
	auto providerIt = mProviders.find( model.provider );
	if ( providerIt == mProviders.end() )
		return "";
	const auto& data = providerIt->second;
	return String::format( "%s (%s)", model.displayName ? *model.displayName : model.name,
						   data.displayName ? *data.displayName : String::capitalize( data.name ) );
}

bool LLMChatUI::selectModel( std::optional<LLMModel> model ) {
	if ( model ) {
		mModelBtn->setText( getModelDisplayName( *model ) );
		mCurModel = *model;
		return true;
	}
	return false;
}

bool LLMChatUI::selectAgent( const std::string& agent ) {
	if ( !agent.empty() && mAgents.find( agent ) != mAgents.end() ) {
		mAgentBtn->setText( agent );
		mCurAgent = agent;
		updateTabTitle();
		return true;
	}
	return false;
}
void LLMChatUI::fillModelDropDownList() {
	mModels.clear();
	std::size_t reserve = 0;
	for ( const auto& [_, data] : mProviders )
		reserve += data.models.size();
	mModels.reserve( reserve + 8 /* extra space for local models */ );
	for ( const auto& [name, data] : mProviders ) {
		if ( !data.enabled )
			continue;
		for ( const auto& model : data.models )
			mModels.push_back( model );
	}
	getUISceneNode()->getThreadPool()->run( [this] { fillApiModels(); } );
}

// Agent Picker

void LLMChatUI::updateLocateAgentBarColumns() {
	Float width = eeceil( mLocateAgentTable->getPixelsSize().getWidth() );
	width -= mLocateAgentTable->getVerticalScrollBar()->getPixelsSize().getWidth();
	mLocateAgentTable->setColumnsVisible( { 0 } );
	mLocateAgentTable->setColumnWidth( 0, eeceil( width ) );
}

void LLMChatUI::loadSelectAgent() {
	auto ctx = getPlugin()->getPluginContext();
	mLocateAgentTable->setModel(
		std::make_shared<ACPAgentsModel>( mAgents, ctx->getUISceneNode() ) );

	static_cast<ACPAgentsModel*>( mLocateAgentTable->getModel() )
		->setFilter( mLocateAgentInput->getText().toUtf8() );
}

void LLMChatUI::showSelectAgent() {
	if ( getPlugin() == nullptr )
		return;
	hideAttachFile();
	hideSelectModel();

	if ( nullptr == mLocateAgentTable->getModel() )
		loadSelectAgent();

	static_cast<ACPAgentsModel*>( mLocateAgentTable->getModel() )
		->setFilter( mLocateAgentInput->getText().toUtf8() );
	mLocateAgentBarLayout->setVisible( true );
	mLocateAgentInput->setFocus();
	updateLocateAgentBarColumns();
}

void LLMChatUI::hideSelectAgent() {
	mLocateAgentBarLayout->setVisible( false );
}

void LLMChatUI::showAgentConfigWindow() {
	if ( !mAgentSession )
		setupAgentSession();

	static const char* AGENT_CONFIG_LAYOUT = R"xml(
		<window window-flags="default|shadow|modal|ephemeral"
				window-title="@string(ai_agent_config, Agent Configuration)">
			<Loader id="loader" radius="48" outline-thickness="6dp" indeterminate="true" />
			<vbox id="config_container" lw="mp" lh="wc" padding="8dp" visible="false"></vbox>
		</window>
	)xml";

	UIWindow* win =
		getUISceneNode()->loadLayoutFromString( AGENT_CONFIG_LAYOUT )->asType<UIWindow>();
	UILoader* loader = win->find( "loader" )->asType<UILoader>();
	UILinearLayout* container = win->find( "config_container" )->asType<UILinearLayout>();

	win->setMinWindowSize( getUISceneNode()->getSize().getWidth() * 0.4f,
						   getUISceneNode()->getSize().getHeight() * 0.3f );
	win->setKeyBindingCommand( "closeWindow", [win, this] {
		win->closeWindow();
		setFocus();
	} );
	win->getKeyBindings().addKeybind( { KEY_ESCAPE }, "closeWindow" );
	win->center();
	win->getModalWidget()->addClass( "shadowbg" );
	win->on( Event::OnWindowReady, [win, loader]( auto ) {
		win->setFocus();
		loader->center();
	} );

	auto setupConfig = [this, loader, container, win]() {
		auto configOptions = mAgentSession->getConfigOptions();
		if ( !configOptions.is_array() || configOptions.empty() ) {
			NotificationCenter::instance()->addNotification(
				i18n( "no_agent_configs", "No Config Options Available" ) );
			win->closeWindow();
			return;
		}

		loader->setVisible( false );
		container->setVisible( true );

		for ( const auto& opt : configOptions ) {
			if ( !opt.is_object() || !opt.contains( "id" ) || !opt.contains( "name" ) ||
				 !opt.contains( "options" ) || !opt["options"].is_array() )
				continue;

			std::string optId = opt["id"].get<std::string>();
			std::string currentValId = opt.value( "currentValue", "" );
			std::string currentValName = "";

			UITextView* label = UITextView::New();
			label->setParent( container );
			label->setText( opt["name"].get<std::string>() );
			label->addClass( "agent_config_label" );

			UIDropDownModelList* dropdown = UIDropDownModelList::New();
			dropdown->setParent( container );
			dropdown->setLayoutWidthPolicy( SizePolicy::MatchParent );
			dropdown->addClass( "agent_config_dropdown" );

			std::vector<std::pair<std::string, std::string>> options;
			int selectedIndex = -1;
			int count = 0;
			for ( const auto& subopt : opt["options"] ) {
				if ( !subopt.is_object() || !subopt.contains( "id" ) || !subopt.contains( "name" ) )
					continue;
				std::string subId = subopt["id"].get<std::string>();
				std::string subName = subopt["name"].get<std::string>();
				options.push_back( { subName, subId } );
				if ( subId == currentValId ) {
					selectedIndex = count;
					currentValName = subName;
				}
				count++;
			}

			auto model = Models::ItemPairListOwnerModel<std::string, std::string>::create(
				std::move( options ) );
			dropdown->setModel( model );
			dropdown->getListView()->setColumnsVisible( { 0 } );
			dropdown->getListView()->setAutoExpandOnSingleColumn( true );
			if ( selectedIndex != -1 ) {
				dropdown->setText( currentValName );
			}

			dropdown->on( Event::OnItemSelected, [this, optId, model, dropdown]( const Event* ) {
				auto* listView = dropdown->getListView();
				if ( !listView || listView->getSelection().isEmpty() )
					return;
				ModelIndex index = listView->getSelection().first();
				std::string subId = model->data( model->index( index.row(), 1 ) ).toString();

				acp::SetConfigOptionRequest req;
				req.sessionId = mAgentSession->getSessionId();
				req.configId = optId;
				req.optionId = subId;
				mAgentSession->setConfigOption(
					req, [this, optId, subId]( const acp::SetConfigOptionResponse& res,
											   const std::optional<acp::ResponseError>& err ) {
						if ( !err ) {
							auto newOpts = res.configOptions;
							if ( newOpts.empty() ) {
								newOpts = acp::parseLegacyConfigOptions(
									{}, mAgentSession->getConfigOptions(), optId, subId );
							}
							mAgentSession->setConfigOptions( newOpts );
						}
					} );
			} );
		}
	};

	if ( mAgentSession && !mAgentSession->getClient()->isReady() ) {
		mAgentConfigBtn->setEnabled( false );
		mAgentSession->start( [this, setupConfig, win]( bool ready ) {
			runOnMainThread( [this, setupConfig, win, ready]() {
				mAgentConfigBtn->setEnabled( true );
				if ( ready && win->isVisible() ) {
					setupConfig();
				} else {
					if ( !ready ) {
						NotificationCenter::instance()->addNotification(
							i18n( "failed_to_start_agent", "Failed to start agent process." ) );
						mAgentSession.reset();
					}
					win->closeWindow();
				}
			} );
		} );
	} else if ( mAgentSession && mAgentSession->getClient()->isReady() ) {
		setupConfig();
	}
}

void LLMChatUI::initSelectAgent() {
	mLocateAgentBarLayout = findByClass<UIVLinearLayoutCommandExecuter>( "llm_chat_select_agent" );
	mLocateAgentInput = findByClass<UITextInput>( "llm_chat_select_agent_input" );
	mLocateAgentTable = findByClass<UITableView>( "llm_chat_agent_locate" );
	mLocateAgentTable->setHeadersVisible( false );

	mLocateAgentTable->on( Event::OnSizeChange,
						   [this]( const Event* ) { updateLocateAgentBarColumns(); } );

	mLocateAgentInput->on( Event::OnTextChanged, [this]( const Event* ) {
		showSelectAgent();
		updateLocateAgentBarColumns();
	} );
	mLocateAgentInput->on( Event::OnPressEnter, [this]( const Event* ) {
		KeyEvent keyEvent( mLocateAgentTable, Event::KeyDown, KEY_RETURN, SCANCODE_UNKNOWN, 0, 0 );
		mLocateAgentTable->forceKeyDown( keyEvent );
	} );
	mLocateAgentInput->on( Event::KeyDown, [this]( const Event* event ) {
		const KeyEvent* keyEvent = static_cast<const KeyEvent*>( event );
		mLocateAgentTable->forceKeyDown( *keyEvent );
	} );
	mLocateAgentBarLayout->setCommand( "close-locatebar", [this] {
		hideSelectAgent();
		if ( mChatInput )
			mChatInput->setFocus();
	} );
	mLocateAgentBarLayout->getKeyBindings().addKeybindsString( {
		{ "escape", "close-locatebar" },
	} );
	mLocateAgentTable->on( Event::KeyDown, [this]( const Event* event ) {
		const KeyEvent* keyEvent = static_cast<const KeyEvent*>( event );
		if ( keyEvent->getKeyCode() == KEY_ESCAPE )
			mLocateAgentBarLayout->execute( "close-locatebar" );
	} );
	mLocateAgentTable->on( Event::OnModelEvent, [this]( const Event* event ) {
		const ModelEvent* modelEvent = static_cast<const ModelEvent*>( event );
		if ( modelEvent->getModelEventType() == ModelEventType::Open ) {
			Variant vName( modelEvent->getModel()->data(
				modelEvent->getModel()->index( modelEvent->getModelIndex().row(),
											   ACPAgentsModel::Name ),
				ModelRole::Display ) );

			if ( vName.isValid() ) {
				std::string name( vName.toString() );
				if ( name != mCurAgent ) {
					selectAgent( name );
					if ( mAgentSession ) {
						mAgentSession->stop();
						mAgentSession.reset();
					}
					updateTabTitle();
				}
			}

			mLocateAgentBarLayout->execute( "close-locatebar" );
		}
	} );
}

UIWidget* LLMChatUI::getLastConversation( bool skipEmpty ) const {
	auto conversations = mChatsList->findAllByClass( "llm_conversation" );
	for ( int i = (int)conversations.size() - 1; i >= 0; --i ) {
		auto* chat = conversations[i];
		if ( !chat->hasClass( "llm_plan" ) && !chat->hasClass( "llm_tool_call" ) &&
			 !chat->hasClass( "llm_thought" ) ) {
			if ( skipEmpty ) {
				auto* editorNode = chat->findByClass( "data_ui" );
				if ( editorNode && editorNode->isType( UI_TYPE_CODEEDITOR ) ) {
					if ( editorNode->asType<UICodeEditor>()->getDocument().isEmpty() )
						continue;
				}
			}
			return chat;
		}
	}
	return nullptr;
}

void LLMChatUI::removeWaitingBubble() {
	auto waitingNodes = mChatsList->findAllByClass( "llm_waiting" );
	for ( auto* node : waitingNodes ) {
		node->close();
	}
}

void LLMChatUI::writeToLastChat( const std::string& text ) {
	runOnMainThread( [this, text] {
		removeWaitingBubble();

		UIWidget* chatAdded = nullptr;
		if ( mChatsList->getChildCount() > 0 ) {
			auto* lastChild = mChatsList->getLastChild()->asType<UIWidget>();
			if ( lastChild->hasClass( "llm_conversation" ) && !lastChild->hasClass( "llm_plan" ) &&
				 !lastChild->hasClass( "llm_tool_call" ) &&
				 !lastChild->hasClass( "llm_thought" ) ) {
				auto* roleDDL = lastChild->findByClass<UIDropDownList>( "role_ui" );
				if ( roleDDL && roleDDL->getListBox()->getItemSelectedIndex() == 1 ) {
					chatAdded = lastChild;
				}
			}
		}

		if ( nullptr == chatAdded ) {
			chatAdded = addChatUI( LLMChat::Role::Assistant );
		}

		auto* editor = chatAdded->findByClass<UICodeEditor>( "data_ui" );
		if ( !editor )
			return;

		auto* thinking = editor->findByClass<UIImage>( "thinking" );
		if ( thinking ) {
			auto thinkingID = String::hash( String::format( "thinking-%p", thinking ) );
			thinking->removeActionsByTag( thinkingID );
			thinking->setVisible( false );
		}

		editor->getDocument().textInput( String::fromUtf8( text ) );
		editor->setCursorVisible( false );
		resizeToFit( editor );
		mChatScrollView->getVerticalScrollBar()->setValue( 1.0f );
	} );
}

void LLMChatUI::updateAgentModeUI() {
	mModelBtn->setVisible( !mIsAgentMode );
	mAgentBtn->setVisible( mIsAgentMode );
	mAgentConfigBtn->setVisible( mIsAgentMode );
	mChatAdd->setVisible( !mIsAgentMode );
	mChatUserRole->setVisible( !mIsAgentMode );

	auto chats = mChatsList->findAllByClass( "llm_conversation" );
	for ( auto chat : chats ) {
		if ( auto roleUi = chat->findByClass( "role_ui" ) )
			roleUi->setVisible( !mIsAgentMode );
		if ( auto moveUp = chat->findByClass( "move_up" ) )
			moveUp->setVisible( !mIsAgentMode );
		if ( auto moveDown = chat->findByClass( "move_down" ) )
			moveDown->setVisible( !mIsAgentMode );
		if ( auto eraseBut = chat->findByClass( "erase_but" ) )
			eraseBut->setVisible( !mIsAgentMode );
	}
}

void LLMChatUI::setupAgentSession() {
	auto it = mAgents.find( mCurAgent );
	if ( it == mAgents.end() )
		return;

	acp::ACPClient::Config config;
	config.command = it->second.command;
	config.args = it->second.args;
	config.environment = it->second.environment;
	config.workingDirectory = getPlugin()->getPluginContext()->getCurrentProject();

	mAgentSession =
		std::make_unique<acp::AgentSession>( getUISceneNode()->getThreadPool(), config );

	mAgentSession->onSessionUpdate = [this]( const nlohmann::json& msg ) {
		auto sessionUpdate = msg.value( "sessionUpdate", "" );
		if ( sessionUpdate == "agent_message_chunk" ) {
			if ( msg.contains( "content" ) && msg["content"].contains( "text" ) ) {
				mThinkingBubble = nullptr; // Reset thinking bubble so next thought gets a new one
				auto chunk = msg["content"].value( "text", "" );
				if ( !chunk.empty() )
					writeToLastChat( chunk );
			}
		} else if ( sessionUpdate == "agent_thought_chunk" ) {
			if ( msg.contains( "content" ) && msg["content"].contains( "text" ) ) {
				auto chunk = msg["content"].value( "text", "" );
				if ( !chunk.empty() )
					runOnMainThread( [this, chunk] { updateThinkingBubble( chunk ); } );
			}
		} else if ( sessionUpdate == "tool_call" || sessionUpdate == "tool_call_update" ) {
			addToolCallUpdate( msg );
		} else if ( sessionUpdate == "plan" ) {
			addPlanUpdate( msg );
		} else if ( sessionUpdate == "available_commands_update" ) {
			if ( msg.contains( "availableCommands" ) && msg["availableCommands"].is_array() ) {
				runOnMainThread( [this, msg] {
					mAvailableCommands.clear();
					for ( const auto& cmd : msg["availableCommands"] ) {
						mAvailableCommands.push_back(
							{ cmd.value( "name", "" ), cmd.value( "description", "" ) } );
					}
				} );
			}
		}
	};

	mAgentSession->onRequestPermission = [this]( const auto& req, auto cb ) {
		runOnMainThread( [this, req, cb]() { addPermissionUI( req, cb ); } );
	};

	mAgentSession->onError = [this]( const acp::ResponseError& err ) {
		NotificationCenter::instance()->addNotification(
			i18n( "ai_assistant_agent_error", "Agent Error: " ) + err.message );
	};

	mAgentSession->onTerminalCreated = [this]( const acp::CreateTerminalRequest& req,

											   const std::string& termId ) {
		runOnMainThread( [this, req, termId] {
			find( "chat_presentation" )->setVisible( false );
			UIWidget* bubble = mChatsList->getUISceneNode()->loadLayoutFromString(
				R"xml(<vbox class="llm_conversation terminal_bubble" lw="mp" lh="300dp" padding="8dp" background-color="var(--tab-back)" />)xml",
				mChatsList );

			std::unordered_map<std::string, std::string> env;
			for ( const auto& e : req.env )
				env[e.name] = e.value;

			auto* uiTerm = eterm::UI::UITerminal::New(
				getPlugin()->getPluginContext()->getTerminalFont(),
				getPlugin()->getPluginContext()->termConfig().fontSize.asPixels(
					getUISceneNode()->getPixelsSize().getWidth(), getUISceneNode()->getPixelsSize(),
					getUISceneNode()->getDPI(),
					getUISceneNode()->getUIThemeManager()->getDefaultFontSize() ),
				Sizef( 0, 0 ), req.command, req.args, env,
				req.cwd ? *req.cwd : getPlugin()->getPluginContext()->getCurrentProject(), 10000,
				nullptr, false, false );
			uiTerm->setClass( "eterm" );
			uiTerm->setParent( bubble );
			uiTerm->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent );

			mTerminalBubbles[termId] = bubble;
			mAgentSession->setTerminalData( termId, uiTerm );
		} );
	};
}

void LLMChatUI::doAgentRequest() {
	mToolCallBubbles.clear();
	mTerminalBubbles.clear();
	if ( !mAgentSession ) {
		auto it = mAgents.find( mCurAgent );
		if ( it == mAgents.end() ) {
			showMsg( "Agent not configured." );
			return;
		}

		mChatRun->setVisible( false )->setEnabled( false );
		mChatStop->setVisible( true )->setEnabled( true );

		setupAgentSession();

		UIWidget* chat = addChatUI( LLMChat::Role::Assistant );
		chat->addClass( "llm_waiting" );
		toggleEnableChats( false );
		auto* editor = chat->findByClass<UICodeEditor>( "data_ui" );
		editor->setEnabled( false );
		auto* thinking = editor->findByClass<UIImage>( "thinking" );
		auto thinkingID = String::hash( String::format( "thinking-%p", thinking ) );
		thinking->setVisible( true );
		thinking->setPosition( { PixelDensity::dpToPx( 8 ), PixelDensity::dpToPx( 3 ) } );
		thinking->setInterval( [thinking] { thinking->rotate( 360 / 32 ); }, Seconds( 0.125 ),
							   thinkingID );

		mAgentSession->start( [this]( bool ready ) {
			runOnMainThread( [this, ready]() {
				if ( ready ) {
					sendAgentPrompt();
				} else {
					mChatStop->setVisible( false )->setEnabled( false );
					mChatRun->setVisible( true )->setEnabled( true );
					toggleEnableChats( true );
					showMsg( i18n( "failed_to_start_agent", "Failed to start agent process." ) );
					mAgentSession.reset();
				}
			} );
		} );
	} else {
		// Existing session, just send the prompt
		mChatRun->setVisible( false )->setEnabled( false );
		mChatStop->setVisible( true )->setEnabled( true );

		UIWidget* chat = addChatUI( LLMChat::Role::Assistant );
		chat->addClass( "llm_waiting" );
		toggleEnableChats( false );
		auto* editor = chat->findByClass<UICodeEditor>( "data_ui" );
		editor->setEnabled( false );
		auto* thinking = editor->findByClass<UIImage>( "thinking" );
		auto thinkingID = String::hash( String::format( "thinking-%p", thinking ) );
		thinking->setVisible( true );
		thinking->setPosition( { PixelDensity::dpToPx( 8 ), PixelDensity::dpToPx( 3 ) } );
		thinking->setInterval( [thinking] { thinking->rotate( 360 / 32 ); }, Seconds( 0.125 ),
							   thinkingID );

		sendAgentPrompt();
	}
}

void LLMChatUI::sendAgentPrompt() {
	acp::PromptRequest req;
	req.sessionId = mAgentSession->getSessionId();

	// Create prompt from the last user message
	auto lastChat = getLastConversation( true );
	if ( lastChat ) {
		auto* editorNode = lastChat->findByClass( "data_ui" );
		if ( !editorNode || !editorNode->isType( UI_TYPE_CODEEDITOR ) ) {
			req.prompt = { { { "type", "text" }, { "text", "" } } };
		} else {
			auto* editor = editorNode->asType<UICodeEditor>();
			std::string text = editor->getDocument().getText().toUtf8();

			bool isSlashCommand = false;
			bool showHelp = false;
			if ( String::startsWith( text, "/" ) ) {
				auto spacePos = text.find( ' ' );
				std::string cmdName = text.substr(
					1, spacePos == std::string::npos ? std::string::npos : spacePos - 1 );
				std::string arg = spacePos == std::string::npos ? "" : text.substr( spacePos + 1 );

				auto it = std::find_if(
					mAvailableCommands.begin(), mAvailableCommands.end(),
					[&cmdName]( const SlashCommand& c ) { return c.name == cmdName; } );

				if ( it != mAvailableCommands.end() ) {
					req.prompt = {
						{ { "type", "slash_command" }, { "name", cmdName }, { "argument", arg } } };
					isSlashCommand = true;
				} else {
					showHelp = true;
				}
			}

			if ( !isSlashCommand && !showHelp ) {
				req.prompt = promptToContentBlocks( text );
			}

			if ( showHelp && !mAvailableCommands.empty() ) {
				std::string help = i18n( "available_commands", "Available Commands:" ) + "\n";
				for ( const auto& cmd : mAvailableCommands ) {
					help += "- **/" + cmd.name + "**: " + cmd.description + "\n";
				}
				help += "\n" + i18n( "type_slash_command_to_use",
									 "Type / followed by the command name to use it." );
				writeToLastChat( help );

				runOnMainThread( [this] {
					mChatStop->setVisible( false )->setEnabled( false );
					mChatRun->setVisible( true )->setEnabled( true );
					toggleEnableChats( true );
					auto lastChat = getLastConversation();
					if ( lastChat ) {
						auto* editorNode = lastChat->findByClass( "data_ui" );
						if ( editorNode && editorNode->isType( UI_TYPE_CODEEDITOR ) ) {
							auto* editor = editorNode->asType<UICodeEditor>();
							auto* thinking = editor->findByClass<UIImage>( "thinking" );
							if ( thinking )
								thinking->setVisible( false );
							editor->setEnabled( true );
							if ( editor->hasFocus() )
								mChatInput->setFocus();
						}
					}
				} );
				return;
			}
		}
	} else {
		req.prompt = { { { "type", "text" }, { "text", "" } } };
	}

	mAgentSession->prompt( req, [this]( const acp::PromptResponse& res,
										const std::optional<acp::ResponseError>& err ) {
		if ( err ) {
			runOnMainThread( [this] {
				mChatStop->setVisible( false )->setEnabled( false );
				mChatRun->setVisible( true )->setEnabled( true );
				toggleEnableChats( true );
				auto lastChat = getLastConversation();
				if ( lastChat ) {
					auto* thinking = lastChat->findByClass<UIImage>( "thinking" );
					if ( thinking )
						thinking->setVisible( false );
				}
			} );
			return;
		}
		runOnMainThread( [this, res]() {
			auto chat = getLastConversation();
			if ( chat ) {
				auto* editor = chat->findByClass<UICodeEditor>( "data_ui" );
				if ( !editor )
					return;
				auto* thinking = editor->findByClass<UIImage>( "thinking" );
				if ( thinking ) {
					auto thinkingID = String::hash( String::format( "thinking-%p", thinking ) );
					thinking->removeActionsByTag( thinkingID );
					thinking->setVisible( false );
				}
				editor->setEnabled( true );
				if ( editor->hasFocus() )
					mChatInput->setFocus();
			}

			toggleEnableChats( true );
			mChatStop->setVisible( false )->setEnabled( false );
			mChatRun->setVisible( true )->setEnabled( true );

			if ( res.stopReason != "cancelled" ) {
				if ( !mChatIsPrivate && !mSummaryRequest && mSummary.empty() ) {
					generateChatName( false );
				} else {
					saveChat();
				}
			}
		} );
	} );
}

void LLMChatUI::showSlashCommands() {
	if ( !getPlugin() || mAvailableCommands.empty() )
		return;

	std::vector<std::string> commands;
	for ( const auto& cmd : mAvailableCommands ) {
		commands.push_back( cmd.name + " - " + cmd.description );
	}

	getPlugin()->createListView(
		mChatInput, ItemListOwnerModel<std::string>::create( std::move( commands ) ),
		[this]( const ModelEvent* event ) {
			if ( event->getModelEventType() == ModelEventType::Open ) {
				auto row = event->getModelIndex().row();
				if ( row >= 0 && row < (int)mAvailableCommands.size() ) {
					auto& doc = mChatInput->getDocument();
					auto cursor = doc.getSelection().start();
					doc.setSelection( { cursor.line(), 0 }, cursor );
					doc.textInput( "/" + mAvailableCommands[row].name + " " );
				}
				event->getNode()->close();
			}
		} );
}

void LLMChatUI::resizeToFit( UICodeEditor* editor ) {
	Float visibleLineCount = editor->getDocumentView().getVisibleLinesCount();
	Float lineHeight = editor->getLineHeight();
	Float height = lineHeight * visibleLineCount + editor->getPixelsPadding().Top +
				   editor->getPixelsPadding().Bottom;
	editor->setPixelsSize( editor->getPixelsSize().getWidth(), height );
}

void LLMChatUI::replaceFileLinksToContents( std::string& text ) {
	LuaPattern ptrn( "\n```file://([^`]*)```\n?" );
	PatternMatcher::Range matches[2];
	while ( ptrn.matches( text, matches ) ) {
		std::string path( text.substr( matches[1].start, matches[1].length() ) );
		if ( FileSystem::isRelativePath( path ) ) {
			std::string prjPath( getPlugin()->getPluginContext()->getCurrentProject() );
			path = prjPath + path;
		}
		std::string fileBuffer;
		TextDocument doc;
		if ( FileSystem::fileExists( path ) &&
			 doc.loadFromFile( path ) == TextDocument::LoadStatus::Loaded ) {
			fileBuffer += "\n`" + doc.getFilename() + "`:\n";
			fileBuffer += "```" + doc.getSyntaxDefinition().getLSPName();
			if ( doc.linesCount() >= 1 && !String::startsWith( doc.line( 0 ).getText(), "\n" ) ) {
				fileBuffer += "\n";
			}
			fileBuffer += doc.getText().toUtf8();
			if ( doc.linesCount() >= 1 &&
				 doc.line( doc.linesCount() - 1 ).getText() != String( "\n" ) ) {
				fileBuffer += "\n";
			}
			fileBuffer += "```\n";
		} else {
			fileBuffer = path + "has been deleted from the file system.";
		}
		text.replace( matches[0].start, matches[0].length(), fileBuffer );
	}
}

nlohmann::json LLMChatUI::promptToContentBlocks( std::string text ) {
	auto j = nlohmann::json::array();
	LuaPattern ptrn( "\n```file://([^`]*)```\n?" );
	PatternMatcher::Range matches[2];
	size_t lastPos = 0;
	while ( ptrn.matches( text, matches, lastPos ) ) {
		// Text before the file
		if ( (size_t)matches[0].start > lastPos ) {
			j.push_back( { { "type", "text" },
						   { "text", text.substr( lastPos, matches[0].start - lastPos ) } } );
		}

		std::string path( text.substr( matches[1].start, matches[1].length() ) );
		if ( FileSystem::isRelativePath( path ) ) {
			std::string prjPath( getPlugin()->getPluginContext()->getCurrentProject() );
			path = prjPath + path;
		}

		if ( FileSystem::fileExists( path ) ) {
			j.push_back( { { "type", "resource_link" },
						   { "name", FileSystem::fileNameFromPath( path ) },
						   { "uri", "file://" + path } } );
		}

		lastPos = matches[0].end;
	}

	if ( lastPos < text.size() ) {
		j.push_back( { { "type", "text" }, { "text", text.substr( lastPos ) } } );
	}

	if ( j.empty() && !text.empty() ) {
		j.push_back( { { "type", "text" }, { "text", text } } );
	}

	return j;
}

nlohmann::json LLMChatUI::chatToJson( bool forRequest ) {
	auto j = nlohmann::json::array();
	auto chats = findAllByClass( "llm_conversation" );
	for ( const auto& chat : chats ) {
		UIDropDownList* roleDDL = chat->findByClass<UIDropDownList>( "role_ui" );
		UIWidget* editorNode = chat->findByClass( "data_ui" );
		if ( !roleDDL || !editorNode || !editorNode->isType( UI_TYPE_CODEEDITOR ) )
			continue;
		UICodeEditor* codeEditor = editorNode->asType<UICodeEditor>();
		std::string role = LLMChat::roleToString(
			static_cast<LLMChat::Role>( roleDDL->getListBox()->getItemSelectedIndex() ) );
		auto text = codeEditor->getDocument().getText().toUtf8();

		if ( text.empty() )
			continue;

		if ( forRequest )
			replaceFileLinksToContents( text );

		j.push_back( { { "role", role }, { "content", std::move( text ) } } );
	}
	return j;
}

Uint32 LLMChatUI::onMessage( const NodeMessage* msg ) {
	AIAssistantPlugin* plugin = getPlugin();
	// plugin can be null since plugin could be re-loading or unloaded
	if ( plugin && msg->getMsg() == NodeMessage::Focus ) {
		getPlugin()->getPluginContext()->getSplitter()->setCurrentWidget( this );
	}
	return 0;
}

nlohmann::json LLMChatUI::serializeChat( const LLMModel& model, bool forRequest ) {
	nlohmann::json j = {
		{ "model", model.name }, { "stream", true }, { "messages", chatToJson( forRequest ) } };
	if ( model.maxOutputTokens )
		j["max_tokens"] = *model.maxOutputTokens;
	return j;
}

nlohmann::json LLMChatUI::serialize() {
	mTimestamp = std::chrono::system_clock::to_time_t( std::chrono::system_clock::now() );
	nlohmann::json j;
	j["uuid"] = mUUID.toString();
	if ( !mIsAgentMode ) {
		j["chat"] = serializeChat( mCurModel );
	}
	j["provider"] = mCurModel.provider;
	j["timestamp"] = mTimestamp;
	j["summary"] = mSummary;
	std::string inputText( mChatInput->getDocument().getText().toUtf8() );
	j["input"] = std::move( inputText );
	j["locked"] = mChatLocked;
	j["agent_mode"] = mIsAgentMode;
	j["agent_name"] = mCurAgent;
	if ( mAgentSession && !mAgentSession->getSessionId().empty() )
		j["session_id"] = mAgentSession->getSessionId();
	return j;
}

std::string LLMChatUI::unserialize( const nlohmann::json& payload ) {
	auto uuid = UUID::fromString( payload.value( "uuid", "" ) );
	if ( uuid )
		mUUID = *uuid;
	mTimestamp = payload.value( "timestamp", 0 );
	mSummary = payload.value( "summary", "" );
	mChatLocked = payload.value( "locked", false );
	mIsAgentMode = payload.value( "agent_mode", false );
	mCurAgent = payload.value( "agent_name", "" );

	selectAgent( mCurAgent );

	std::string provider = payload.value( "provider", "" );
	if ( payload.contains( "chat" ) && payload["chat"].is_object() ) {
		const auto& chat = payload["chat"];
		std::string model = chat.value( "model", "" );
		mCurModel = findModel( provider, model );
	}

	if ( mCurModel.name.empty() && !mIsAgentMode )
		return payload.value( "input", "" );

	if ( mIsAgentMode ) {
		mChatAgentMode->setSelected( true );
		updateAgentModeUI();

		std::string sessionId = payload.value( "session_id", "" );
		if ( !sessionId.empty() ) {
			auto it = mAgents.find( mCurAgent );
			if ( it != mAgents.end() ) {
				setupAgentSession();
				mAgentSession->startLoaded( sessionId, []( bool ) {} );
			}
		}
	} else {
		if ( !selectModel( mCurModel ) )
			fillModelDropDownList();
	}

	if ( !mIsAgentMode && payload.contains( "chat" ) && payload["chat"].is_object() ) {
		const auto& chat = payload["chat"];
		const auto& messages = chat["messages"];
		for ( const auto& chat : messages ) {
			addChat( LLMChat::stringToRole( chat.value( "role", "" ) ),
					 chat.value( "content", "" ) );
		}
	}

	updateTabTitle();

	return payload.value( "input", "" );
}

LLMModel LLMChatUI::findModel( const std::string& provider, const std::string& model ) {
	auto providerIt = mProviders.find( provider );
	if ( providerIt != mProviders.end() ) {
		auto modelIt =
			std::find_if( providerIt->second.models.begin(), providerIt->second.models.end(),
						  [&model]( const LLMModel& cmodel ) { return cmodel.name == model; } );
		if ( modelIt != providerIt->second.models.end() ) {
			return *modelIt;
		}
	}
	if ( provider == DEFAULT_PROVIDER && model == DEFAULT_MODEL )
		return mCurModel; // Do not stack-overflow if something is really wrong
	return getDefaultModel();
}

LLMModel LLMChatUI::getDefaultModel() {
	return findModel( DEFAULT_PROVIDER, DEFAULT_MODEL );
}

std::string LLMChatUI::getNewFilePath( const std::string& uuid, const std::string& summary,
									   bool isLocked ) const {
	auto plugin = getPlugin();
	if ( plugin == nullptr || mChatIsPrivate )
		return "";
	std::string conversationsPath = plugin->getConversationsPath();
	if ( !FileSystem::fileExists( conversationsPath ) )
		FileSystem::makeDir( conversationsPath, true );
	return conversationsPath + uuid + " - " + summary + ( isLocked ? ".locked" : "" ) + ".json";
}

std::string LLMChatUI::getFilePath() const {
	return getNewFilePath( mUUID.toString(), mSummary, mChatLocked );
}

void LLMChatUI::saveChat() {
	if ( mIsAgentMode )
		return;
	auto plugin = getPlugin();
	if ( plugin == nullptr || mChatIsPrivate )
		return;
	FileSystem::fileWrite( getFilePath(), serialize().dump( 2 ) );
}

std::string LLMChatUI::prepareApiUrl( const std::string& apiKey ) {
	const auto& provider = mProviders[mCurModel.provider];
	std::string url = provider.apiUrl;
	String::replaceAll( url, "${model}", mCurModel.name );
	String::replaceAll( url, "${api_key}", apiKey );
	return url;
}

static bool allNewLines( const std::string& s ) {
	return !s.empty() && std::all_of( s.begin(), s.end(), []( char c ) { return c == '\n'; } );
}

void LLMChatUI::generateChatName( bool isRenaming ) {
	auto apiKey = AIAssistantPlugin::getApiKeyFromProvider( mCurModel.provider, getPlugin() );
	if ( !apiKey ) {
		showMsg( getUISceneNode()->i18n( "configure_api_key",
										 "You must first configure your provider api key." ) );
		return;
	}
	std::string apiKeyStr{ *apiKey };
	std::string apiUrl( prepareApiUrl( apiKeyStr ) );
	auto model = mCurModel;

	static const std::string SummaryPrompt =
		"Generate a concise 3-7 word title for this conversation, omitting punctuation. Go "
		"straight to the title, without any preamble and prefix like `Here's a concise "
		"suggestion:...` or `Title:`. Ignore this message for the summary generation.";

	auto jchat = serializeChat( getCheapestModelFromCurrentProvider(), true );

	jchat["messages"].push_back( { { "role", LLMChat::roleToString( LLMChat::Role::User ) },
								   { "content", SummaryPrompt } } );

	auto chatstr = jchat.dump();

	mSummaryRequest =
		std::make_unique<LLMChatCompletionRequest>( apiUrl, apiKeyStr, chatstr, model.provider );

	mSummaryRequest->doneCb = [this, isRenaming]( const LLMChatCompletionRequest& req,
												  Http::Response& response ) {
		auto status = response.getStatus();
		runOnMainThread( [this, isRenaming, status, responseText = req.getResponse()] {
			String oldSummary = std::move( mSummary );

			if ( status == Http::Response::Ok ) {
				mSummary = String::trim( responseText );
				String::trimInPlace( mSummary, '\n' );
				String::trimInPlace( mSummary, ' ' );
				String::trimInPlace( mSummary, '"' );
			} else {
				// TODO: Implement generating a summary based on the user prompt (take the
				// first few words)
				mSummary = i18n( "untitled_conversation", "Untitled Conversation" );
			}

			if ( isRenaming ) {
				String newSummary = std::move( mSummary );
				mSummary = std::move( oldSummary );
				renameChat( newSummary );
			} else
				saveChat();

			updateTabTitle();
			mSummaryRequest.reset();
		} );
	};
	mSummaryRequest->requestAsync();
}

bool LLMChatUI::chatExistsInDisk() const {
	return FileSystem::fileExists( getFilePath() );
}

void LLMChatUI::regenerateChatName() {
	if ( chatExistsInDisk() )
		generateChatName( true );
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
	chat->addClass( "llm_waiting" );
	toggleEnableChats( false ); // editor is not disabled (to allow copy with locked document)

	auto model = mCurModel;
	auto* editor = chat->findByClass<UICodeEditor>( "data_ui" );

	if ( !editor )
		return;

	// new editor must be disabled because even on locked document it's possible to move the cursor
	editor->setEnabled( false );

	auto* thinking = editor->findByClass<UIImage>( "thinking" );
	auto thinkingID = String::hash( String::format( "thinking-%p", thinking ) );
	thinking->setVisible( true );
	thinking->setPosition( { PixelDensity::dpToPx( 8 ), PixelDensity::dpToPx( 3 ) } );
	thinking->setInterval( [thinking] { thinking->rotate( 360 / 32 ); }, Seconds( 0.125 ),
						   thinkingID );

	std::string apiUrl( prepareApiUrl( apiKeyStr ) );
	mRequest = std::make_unique<LLMChatCompletionRequest>(
		apiUrl, apiKeyStr, serializeChat( model, true ).dump(), model.provider );

	mThinkingBubble = nullptr;
	mRequest->streamedResponseCb = [this, editor, thinking, thinkingID]( const std::string& chunk,
																		 bool fromReasoning ) {
		if ( fromReasoning ) {
			runOnMainThread( [this, chunk] { updateThinkingBubble( chunk ); } );
			return;
		}

		if ( mRequest && mRequest->getResponse().empty() && allNewLines( chunk ) )
			return;

		auto conversation = chunk;
		editor->runOnMainThread(
			[this, conversation = std::move( conversation ), editor, thinking, thinkingID] {
				mThinkingBubble = nullptr;
				editor->getDocument().textInput( String::fromUtf8( conversation ) );
				editor->setCursorVisible( false );
				thinking->removeActionsByTag( thinkingID );
				thinking->setVisible( false );
				resizeToFit( editor );
			} );
	};

	mRequest->cancelCb = [this, thinking, thinkingID, editor]( const LLMChatCompletionRequest& ) {
		runOnMainThread( [this, thinking, thinkingID, editor] {
			thinking->removeActionsByTag( thinkingID );
			thinking->setVisible( false );
			mChatStop->setVisible( false )->setEnabled( false );
			mChatRun->setVisible( true )->setEnabled( true );
			toggleEnableChats( true );
			editor->setEnabled( true );
			if ( editor->hasFocus() )
				mChatInput->setFocus();
			removeLastChat();
		} );
	};

	mRequest->doneCb =
		[this, editor, apiUrl = std::move( apiUrl ), apiKeyStr = std::move( apiKeyStr ),
		 model = std::move( model )]( const LLMChatCompletionRequest&, Http::Response& response ) {
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
					} else if ( resp.is_array() && !resp.empty() ) {
						const auto& first = resp.at( 0 );
						if ( first.contains( "error" ) && first["error"].contains( "message" ) )
							showMsg( first["error"].value( "message", "" ) );
						else
							showMsg( statusDesc );
					} else {
						showMsg( statusDesc );
					}

					removeLastChat();
				}
				mRequest.reset();
				toggleEnableChats( true );
				editor->setEnabled( true );

				mChatStop->setVisible( false )->setEnabled( false );
				mChatRun->setVisible( true )->setEnabled( true );

				if ( editor->hasFocus() )
					mChatInput->setFocus();

				if ( !mChatIsPrivate && !mSummaryRequest && mSummary.empty() &&
					 status == Http::Response::Ok ) {
					generateChatName( false );
				} else {
					saveChat();
				}
			} );
		};
	mRequest->requestAsync();
}

void LLMChatUI::toggleEnableChat( UIWidget* chat, bool enabled ) {
	auto* roleDDL = chat->findByClass( "role_ui" );
	if ( roleDDL )
		roleDDL->setEnabled( enabled );
	auto* editorNode = chat->findByClass( "data_ui" );
	if ( editorNode && editorNode->isType( UI_TYPE_CODEEDITOR ) )
		editorNode->asType<UICodeEditor>()->setLocked( !enabled );
	auto* eraseBut = chat->findByClass( "erase_but" );
	if ( eraseBut )
		eraseBut->setEnabled( enabled );
	auto* moveUp = chat->findByClass( "move_up" );
	if ( moveUp )
		moveUp->setEnabled( enabled );
	auto* moveDown = chat->findByClass( "move_down" );
	if ( moveDown )
		moveDown->setEnabled( enabled );
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

void LLMChatUI::addPermissionUI( const acp::RequestPermissionRequest& req,
								 std::function<void( const acp::RequestPermissionResponse& )> cb ) {
	find( "chat_presentation" )->setVisible( false );

	UIWidget* chat =
		mChatsList->getUISceneNode()->loadLayoutFromString( DEFAULT_PERMISSION_GLOBE, mChatsList );

	UIMarkdownView* desc = chat->findByClass<UIMarkdownView>( "permission_desc" );
	std::string descStr =
		"#####	 " +
		i18n( "agent_wants_to_execute_tool_call", "The agent wants to execute a tool call:" ) +
		"\n\n";
	descStr += "**" + i18n( "title", "Title" ) + ":** " + req.toolCall.title + "\n\n";
	descStr += "**" + i18n( "kind", "Kind" ) + ":** `" + req.toolCall.kind + "`\n\n";
	if ( !req.toolCall.rawInput.is_null() ) {
		descStr +=
			"**" + i18n( "input", "Input" ) + ":**\n\n" + req.toolCall.rawInput.dump( 2 ) + "\n";
	}
	desc->loadFromString( descStr );
	UIWidget* optionsBox = chat->findByClass( "permission_options" );
	if ( !optionsBox )
		return;

	bool isFirst = true;
	UIPushButton* firstBut = nullptr;
	UIPushButton* lastBut = nullptr;
	for ( const auto& opt : req.options ) {
		UIPushButton* but = UIPushButton::New();
		but->setParent( optionsBox );
		but->setText( opt.name );

		auto cbCopy = cb;
		auto optId = opt.optionId;
		but->onClick( [chat, cbCopy, optId, this]( const Event* ) {
			chat->close(); // Close the permission request UI once selected
			acp::RequestPermissionResponse res;
			res.outcome = "selected";
			res.optionId = optId;
			cbCopy( res );

			if ( mChatInput )
				mChatInput->setFocus();
		} );

		if ( isFirst ) {
			isFirst = false;
			firstBut = but;
			but->runOnMainThread( [but] { but->setFocus(); } );
		}

		lastBut = but;
	}

	if ( lastBut && firstBut != lastBut ) {
		lastBut->on( Event::OnTabNavigate,
					 [firstBut]( const Event* event ) { firstBut->setFocus(); } );
	}

	mChatScrollView->getVerticalScrollBar()->setValue( 1.0f );
}

UIWidget* LLMChatUI::addMarkdownBubble( const std::string& layout, const std::string& markdown ) {
	find( "chat_presentation" )->setVisible( false );
	UIWidget* chat = mChatsList->getUISceneNode()->loadLayoutFromString( layout, mChatsList );
	if ( chat )
		chat->asType<UIMarkdownView>()->loadFromString( markdown );
	return chat;
}

void LLMChatUI::addPlanBubble( const std::string& markdown ) {
	runOnMainThread( [this, markdown] {
		removeWaitingBubble();

		UIWidget* bubble = nullptr;
		if ( mChatsList->getLastChild() ) {
			auto* lastChild = mChatsList->getLastChild()->asType<UIWidget>();
			if ( lastChild->hasClass( "llm_plan" ) )
				bubble = lastChild;
		}

		if ( !bubble ) {
			bubble = addMarkdownBubble( DEFAULT_PLAN_GLOBE, markdown );
		} else {
			bubble->asType<UIMarkdownView>()->loadFromString( markdown );
		}
		mChatScrollView->getVerticalScrollBar()->setValue( 1.0f );
	} );
}

void LLMChatUI::addPlanUpdate( const nlohmann::json& msg ) {
	runOnMainThread( [this, msg] {
		removeWaitingBubble();
		std::string planMarkdown = "📋 " + i18n( "plan_updated", "Plan Updated:" ) + "\n";
		if ( msg.contains( "plan" ) ) {
			const auto& plan = msg["plan"];
			const auto& entries =
				plan.contains( "entries" )
					? plan["entries"]
					: ( plan.contains( "steps" ) ? plan["steps"] : nlohmann::json::array() );
			if ( entries.is_array() ) {
				for ( const auto& step : entries ) {
					std::string status = step.value( "status", "" );
					std::string statusIcon = "⏳";
					if ( status == "completed" || status == "success" )
						statusIcon = "✅";
					else if ( status == "running" || status == "in_progress" )
						statusIcon = "🔄";
					else if ( status == "failed" || status == "error" )
						statusIcon = "❌";
					else if ( status == "cancelled" )
						statusIcon = "🚫";

					planMarkdown += "- " + statusIcon + " " + step.value( "title", "" ) + "\n";
				}
			}
		}
		addPlanBubble( planMarkdown );
	} );
}

void LLMChatUI::addToolCallBubble( const std::string& markdown ) {
	runOnMainThread( [this, markdown] {
		removeWaitingBubble();

		UIWidget* bubble = nullptr;
		if ( mChatsList->getLastChild() ) {
			auto* lastChild = mChatsList->getLastChild()->asType<UIWidget>();
			if ( lastChild->hasClass( "llm_tool_call" ) )
				bubble = lastChild;
		}

		if ( !bubble ) {
			mCurToolCall = markdown;
			bubble = addMarkdownBubble( DEFAULT_TOOL_CALL_GLOBE, mCurToolCall );
		} else {
			mCurToolCall += "\n" + markdown;
			bubble->asType<UIMarkdownView>()->loadFromString( mCurToolCall );
		}

		mChatScrollView->getVerticalScrollBar()->setValue( 1.0f );
	} );
}

void LLMChatUI::addToolCallUpdate( const nlohmann::json& msg ) {
	runOnMainThread( [this, msg] {
		removeWaitingBubble();
		std::string toolCallId = msg.value( "toolCallId", "" );
		std::string terminalId = msg.value( "terminalId", "" );
		std::string status = msg.value( "status", "" );
		std::string title = msg.value( "title", "" );

		UIWidget* bubble = nullptr;
		if ( !toolCallId.empty() ) {
			auto it = mToolCallBubbles.find( toolCallId );
			if ( it != mToolCallBubbles.end() )
				bubble = it->second;
		}

		if ( !bubble ) {
			if ( mChatsList->getLastChild() ) {
				auto* lastChild = mChatsList->getLastChild()->asType<UIWidget>();
				if ( lastChild->hasClass( "llm_tool_call" ) && toolCallId.empty() )
					bubble = lastChild;
			}
		}

		std::string statusIcon = "⏳";
		if ( status == "completed" || status == "success" )
			statusIcon = "✅";
		else if ( status == "running" || status == "in_progress" )
			statusIcon = "🔄";
		else if ( status == "failed" || status == "error" )
			statusIcon = "❌";
		else if ( status == "cancelled" )
			statusIcon = "🚫";

		std::string toolMarkdown = "> " + statusIcon + " 🛠️ " + i18n( "tool_call", "Tool Call: " ) +
								   ( title.empty() ? toolCallId : title ) + "\n\n";

		if ( msg.contains( "rawInput" ) ) {
			auto rawInput = msg["rawInput"];
			toolMarkdown +=
				"\n```json\n" +
				( rawInput.is_string() ? rawInput.get<std::string>() : rawInput.dump( 2 ) ) +
				"\n```\n";
		}
		if ( msg.contains( "rawOutput" ) ) {
			auto rawOutput = msg["rawOutput"];
			toolMarkdown +=
				"\n```json\n" +
				( rawOutput.is_string() ? rawOutput.get<std::string>() : rawOutput.dump( 2 ) ) +
				"\n```\n";
		}

		if ( !bubble ) {
			bubble = addMarkdownBubble( DEFAULT_TOOL_CALL_GLOBE, toolMarkdown );
			if ( !toolCallId.empty() )
				mToolCallBubbles[toolCallId] = bubble;
		} else {
			bubble->asType<UIMarkdownView>()->loadFromString( toolMarkdown );
		}

		if ( !terminalId.empty() ) {
			auto it = mTerminalBubbles.find( terminalId );
			if ( it != mTerminalBubbles.end() ) {
				UIWidget* terminalBubble = it->second;
				eterm::UI::UITerminal* uiTerm =
					terminalBubble->findByClass<eterm::UI::UITerminal>( "eterm" );
				if ( uiTerm ) {
					uiTerm->setParent( bubble );
					uiTerm->setLayoutHeightPolicy( SizePolicy::Fixed );
					uiTerm->setPixelsSize( uiTerm->getPixelsSize().getWidth(),
										   PixelDensity::dpToPx( 300 ) );
					terminalBubble->close();
					mTerminalBubbles.erase( it );
				}
			}
		}
		mChatScrollView->getVerticalScrollBar()->setValue( 1.0f );
	} );
}

void LLMChatUI::updateThinkingBubble( const std::string& chunk ) {
	runOnMainThread( [this, chunk] {
		removeWaitingBubble();

		UIWidget* bubble = nullptr;
		if ( mChatsList->getLastChild() ) {
			auto* lastChild = mChatsList->getLastChild()->asType<UIWidget>();
			if ( lastChild->hasClass( "llm_thought" ) ) {
				bubble = lastChild;
			}
		}

		if ( !bubble ) {
			mCurThinking = "";
			bubble = addMarkdownBubble( DEFAULT_THINKING_GLOBE, mCurThinking );
		}

		mCurThinking += chunk;
		bubble->asType<UIMarkdownView>()->loadFromString( mCurThinking );
		mChatScrollView->getVerticalScrollBar()->setValue( 1.0f );
	} );
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
	editor->on( Event::OnBeforeFoldUnfoldRange,
				[this]( auto ) { mChatScrollView->setAnchorScroll( true ); } );
	editor->on( Event::OnFoldUnfoldRange,
				[this]( auto ) { mChatScrollView->setAnchorScroll( false ); } );
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
	chat->findByClass( "copy_contents" )->onClick( [this, editor]( auto ) {
		auto text = editor->getDocument().getText().toUtf8();
		if ( text.empty() )
			return;
		replaceFileLinksToContents( text );
		getUISceneNode()->getWindow()->getClipboard()->setText( text );
		getPlugin()->getPluginContext()->getNotificationCenter()->addNotification(
			i18n( "chat_copied", "Chat Copied" ) );
	} );
	resizeToFit( editor );
	if ( mIsAgentMode ) {
		chat->findByClass( "role_ui" )->setVisible( false );
		chat->findByClass( "erase_but" )->setVisible( false );
		chat->findByClass( "move_up" )->setVisible( false );
		chat->findByClass( "move_down" )->setVisible( false );
	}

	return chat;
}

void LLMChatUI::addChat( LLMChat::Role role, std::string conversation ) {
	UIWidget* chat = addChatUI( role );
	auto* editor = chat->findByClass<UICodeEditor>( "data_ui" );
	editor->getDocument().textInput( String::fromUtf8( conversation ) );
	editor->setCursorVisible( false );
	editor->setAllowSelectingTextFromGutter( false );
	bindCmds( editor, false );
	resizeToFit( editor );
}

void LLMChatUI::removeLastChat() {
	auto* chat = getLastConversation();
	if ( chat ) {
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
	msgBox->getTextBox()->setLayoutWidthPolicy( SizePolicy::Fixed );
	msgBox->getTextBox()->setPixelsSize(
		{ PixelDensity::dpToPx( 600 ), msgBox->getTextBox()->getPixelsSize().getHeight() } );
	msgBox->getTextBox()->setWordWrap( true );
	msgBox->getTextBox()->setTextSelectionEnabled( true );
	msgBox->getTextBox()->onClick(
		[]( const MouseEvent* event ) {
			auto tv = event->getNode()->asType<UITextView>();
			tv->getUISceneNode()->getWindow()->getClipboard()->setText( tv->getText() );
		},
		EE_BUTTON_RIGHT );
	msgBox->showWhenReady();
}

AIAssistantPlugin* LLMChatUI::getPlugin() const {
	if ( mManager != nullptr ) {
		auto plugin = mManager->get( "aiassistant" );
		if ( plugin )
			return reinterpret_cast<AIAssistantPlugin*>( plugin );
	}
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

void LLMChatUI::onInit() {
	if ( !mModelBtn )
		return;
	if ( getModelDisplayName( mCurModel ) != mModelBtn->getText() )
		selectModel( mCurModel );
}

void LLMChatUI::updateTabTitle() {
	if ( getData() == 0 )
		return;
	UITab* tab = reinterpret_cast<UITab*>( getData() );
	auto title = i18n( "ai_assistant", "AI Assistant" );
	if ( mIsAgentMode && !mCurAgent.empty() ) {
		title = i18n( "agent", "Agent" ) + ": " + mCurAgent;
	}
	if ( !mSummary.empty() )
		title += " - " + mSummary;
	tab->setText( title );
	tab->setTooltipText( title );
}

void LLMChatUI::renameChat( const std::string& newName, bool invertLockedState ) {
	auto oldPath = getNewFilePath( mUUID.toString(), mSummary, mChatLocked );
	auto newPath =
		getNewFilePath( mUUID.toString(), newName, invertLockedState ? !mChatLocked : mChatLocked );
	if ( FileSystem::fileMove( oldPath, newPath ) ) {
		mSummary = newName;
		if ( invertLockedState )
			mChatLocked = !mChatLocked;
		saveChat();
		updateTabTitle();
	}
}

UISplitter* LLMChatUI::getSplitter() const {
	return mChatSplitter;
}

const LLMModel& LLMChatUI::getCurModel() const {
	return mCurModel;
}

void LLMChatUI::deleteOldConversations( int days ) {
	auto plugin = getPlugin();
	if ( plugin == nullptr )
		return;
	std::string conversationsPath = plugin->getConversationsPath();
	auto history = ChatHistory::getHistory( conversationsPath );

	Int64 olderThanTime = std::chrono::system_clock::to_time_t( std::chrono::system_clock::now() ) -
						  ( 60 * 60 * 24 * days );

	for ( const auto& chat : history )
		if ( !chat.locked && chat.file.getModificationTime() < olderThanTime )
			FileSystem::fileRemove( chat.file.getFilepath() );
}

void LLMChatUI::updateLocateBarColumns() {
	Float width = eeceil( mLocateTable->getPixelsSize().getWidth() );
	width -= mLocateTable->getVerticalScrollBar()->getPixelsSize().getWidth();
	mLocateTable->setColumnsVisible( { 0, 1 } );
	mLocateTable->setColumnWidth( 0, eeceil( width * 0.5 ) );
	mLocateTable->setColumnWidth( 1, width - mLocateTable->getColumnWidth( 0 ) );
}

// File picker

void LLMChatUI::showAttachFile() {
	if ( getPlugin() == nullptr )
		return;
	hideSelectModel();
	hideSelectAgent();
	auto text = mLocateInput->getText();
	auto ctx = getPlugin()->getPluginContext();
	if ( !ctx->isDirTreeReady() ) {
		mLocateTable->setModel( ProjectDirectoryTree::emptyModel( {}, ctx->getCurrentProject() ) );
		mLocateTable->getSelection().set( mLocateTable->getModel()->index( 0 ) );
	} else if ( !mLocateInput->getText().empty() ) {
		ctx->getDirTree()->asyncMatchTree(
			ProjectDirectoryTree::MatchType::Fuzzy, text, 100,
			[this, text]( auto res ) {
				mUISceneNode->runOnMainThread( [this, res] {
					mLocateTable->setModel( res );
					mLocateTable->getSelection().set( mLocateTable->getModel()->index( 0 ) );
					mLocateTable->scrollToTop();
					updateLocateBarColumns();
				} );
			},
			ctx->getCurrentProject() );
	} else {
		mLocateTable->setModel( ctx->getDirTree()->asModel(
			100, {}, ctx->getCurrentProject(), Image::getImageExtensionsSupported() ) );
		mLocateTable->getSelection().set( mLocateTable->getModel()->index( 0 ) );
	}
	mLocateBarLayout->setVisible( true );
	mLocateInput->setFocus();
	updateLocateBarColumns();
}

void LLMChatUI::hideAttachFile() {
	mLocateBarLayout->setVisible( false );
}

void LLMChatUI::insertFileToDocument( std::string path, std::shared_ptr<TextDocument> cdoc ) {
	std::string prjPath( getPlugin()->getPluginContext()->getCurrentProject() );

	if ( FileSystem::isRelativePath( path ) )
		path = prjPath + path;

	TextDocument doc;
	if ( doc.loadFromFile( path ) == TextDocument::LoadStatus::Loaded ) {
		std::string nameToDisplay = doc.getFilename();
		if ( !prjPath.empty() && String::startsWith( doc.getFilePath(), prjPath ) ) {
			nameToDisplay = doc.getFilePath();
			FileSystem::filePathRemoveBasePath( prjPath, nameToDisplay );
		}
		cdoc->resetSelection();
		cdoc->moveToEndOfLine();
		cdoc->textInput( "\n`" + nameToDisplay + "`:\n" );
		cdoc->textInput( "```" + doc.getSyntaxDefinition().getLSPName() );
		auto lineToFold = cdoc->getSelection().end().line();
		if ( doc.linesCount() >= 1 && !String::startsWith( doc.line( 0 ).getText(), "\n" ) ) {
			cdoc->textInput( "\n" );
		}
		cdoc->textInput( doc.getText() );
		if ( doc.linesCount() >= 1 &&
			 doc.line( doc.linesCount() - 1 ).getText() != String( "\n" ) ) {
			cdoc->textInput( "\n" );
		}
		cdoc->textInput( "```\n" );
		cdoc->getFoldRangeService().findRegionsNative();
		if ( doc.linesCount() > 30 &&
			 cdoc->getFoldRangeService().isFoldingRegionInLine( lineToFold ) ) {
			mChatInput->toggleFoldUnfold( lineToFold );
		}
	}
}

void LLMChatUI::initAttachFile() {
	mLocateBarLayout = findByClass<UIVLinearLayoutCommandExecuter>( "llm_chat_attach" );
	mLocateInput = findByClass<UITextInput>( "llm_chat_locate_input" );
	mLocateTable = findByClass<UITableView>( "llm_chat_attach_locate" );
	mLocateTable->setHeadersVisible( false );

	mLocateTable->on( Event::OnSizeChange, [this]( const Event* ) { updateLocateBarColumns(); } );

	mLocateInput->on( Event::OnTextChanged, [this]( const Event* ) {
		showAttachFile();
		updateLocateBarColumns();
	} );
	mLocateInput->on( Event::OnPressEnter, [this]( const Event* ) {
		KeyEvent keyEvent( mLocateTable, Event::KeyDown, KEY_RETURN, SCANCODE_UNKNOWN, 0, 0 );
		mLocateTable->forceKeyDown( keyEvent );
	} );
	mLocateInput->on( Event::KeyDown, [this]( const Event* event ) {
		const KeyEvent* keyEvent = static_cast<const KeyEvent*>( event );
		mLocateTable->forceKeyDown( *keyEvent );
	} );
	mLocateBarLayout->setCommand( "close-locatebar", [this] {
		hideAttachFile();
		if ( mChatInput )
			mChatInput->setFocus();
	} );
	mLocateBarLayout->getKeyBindings().addKeybindsString( {
		{ "escape", "close-locatebar" },
	} );
	mLocateTable->on( Event::KeyDown, [this]( const Event* event ) {
		const KeyEvent* keyEvent = static_cast<const KeyEvent*>( event );
		if ( keyEvent->getKeyCode() == KEY_ESCAPE )
			mLocateBarLayout->execute( "close-locatebar" );
	} );
	mLocateTable->on( Event::OnModelEvent, [this]( const Event* event ) {
		const ModelEvent* modelEvent = static_cast<const ModelEvent*>( event );
		if ( modelEvent->getModelEventType() == ModelEventType::Open ) {
			Variant vName( modelEvent->getModel()->data(
				modelEvent->getModel()->index( modelEvent->getModelIndex().row(), 0 ),
				ModelRole::Display ) );
			Variant vPath( modelEvent->getModel()->data(
				modelEvent->getModel()->index( modelEvent->getModelIndex().row(), 1 ),
				ModelRole::Display ) );

			if ( !vPath.isValid() )
				return;

			std::string path( vPath.toString() );
			if ( path.empty() )
				return;

			Variant rangeStr( modelEvent->getModel()->data(
				modelEvent->getModel()->index( modelEvent->getModelIndex().row(), 1 ),
				ModelRole::Custom ) );

			auto cdoc = mChatInput->getDocumentRef();

			if ( mLinkMode ) {
				cdoc->resetSelection();
				cdoc->moveToEndOfLine();
				cdoc->textInput( "\n```file://" + path + "```\n" );
			} else {
				insertFileToDocument( path, cdoc );
			}

			mLocateBarLayout->execute( "close-locatebar" );
		}
	} );
}

// Model Picker

void LLMChatUI::updateLocateModelBarColumns() {
	Float width = eeceil( mLocateModelTable->getPixelsSize().getWidth() );
	width -= mLocateModelTable->getVerticalScrollBar()->getPixelsSize().getWidth();
	mLocateModelTable->setColumnsVisible( { 0, 1 } );
	mLocateModelTable->setColumnWidth( 0, eeceil( width * 0.8 ) );
	mLocateModelTable->setColumnWidth( 1, width - mLocateModelTable->getColumnWidth( 0 ) );
}

void LLMChatUI::loadSelectModel() {
	auto ctx = getPlugin()->getPluginContext();
	mLocateModelTable->setModel(
		std::make_shared<LLMModelsModel>( mModels, ctx->getUISceneNode() ) );

	static_cast<LLMModelsModel*>( mLocateModelTable->getModel() )
		->setFilter( mLocateModelInput->getText() );
}

void LLMChatUI::showSelectModel() {
	if ( getPlugin() == nullptr )
		return;
	hideAttachFile();
	hideSelectAgent();

	if ( nullptr == mLocateModelTable->getModel() )
		loadSelectModel();

	static_cast<LLMModelsModel*>( mLocateModelTable->getModel() )
		->setFilter( mLocateModelInput->getText() );

	mLocateModelBarLayout->setVisible( true );
	mLocateModelInput->setFocus();
	updateLocateModelBarColumns();
}

void LLMChatUI::hideSelectModel() {
	mLocateModelBarLayout->setVisible( false );
}

void LLMChatUI::initSelectModel() {
	mLocateModelBarLayout = findByClass<UIVLinearLayoutCommandExecuter>( "llm_chat_select_model" );
	mLocateModelInput = findByClass<UITextInput>( "llm_chat_select_model_input" );
	mLocateModelTable = findByClass<UITableView>( "llm_chat_model_locate" );
	mLocateModelTable->setHeadersVisible( false );

	mLocateModelTable->on( Event::OnSizeChange,
						   [this]( const Event* ) { updateLocateModelBarColumns(); } );

	mLocateModelInput->on( Event::OnTextChanged, [this]( const Event* ) {
		showSelectModel();
		updateLocateModelBarColumns();
	} );
	mLocateModelInput->on( Event::OnPressEnter, [this]( const Event* ) {
		KeyEvent keyEvent( mLocateModelTable, Event::KeyDown, KEY_RETURN, SCANCODE_UNKNOWN, 0, 0 );
		mLocateModelTable->forceKeyDown( keyEvent );
	} );
	mLocateModelInput->on( Event::KeyDown, [this]( const Event* event ) {
		const KeyEvent* keyEvent = static_cast<const KeyEvent*>( event );
		mLocateModelTable->forceKeyDown( *keyEvent );
	} );
	mLocateModelBarLayout->setCommand( "close-locatebar", [this] {
		hideSelectModel();
		if ( mChatInput )
			mChatInput->setFocus();
	} );
	mLocateModelBarLayout->getKeyBindings().addKeybindsString( {
		{ "escape", "close-locatebar" },
	} );
	mLocateModelTable->on( Event::KeyDown, [this]( const Event* event ) {
		const KeyEvent* keyEvent = static_cast<const KeyEvent*>( event );
		if ( keyEvent->getKeyCode() == KEY_ESCAPE )
			mLocateModelBarLayout->execute( "close-locatebar" );
	} );
	mLocateModelTable->on( Event::OnModelEvent, [this]( const Event* event ) {
		const ModelEvent* modelEvent = static_cast<const ModelEvent*>( event );
		if ( modelEvent->getModelEventType() == ModelEventType::Open ) {
			Variant vHash( modelEvent->getModel()->data(
				modelEvent->getModel()->index( modelEvent->getModelIndex().row(),
											   LLMModelsModel::Hash ),
				ModelRole::Display ) );
			selectModel( getModel( vHash.asUint64() ) );
			mLocateModelBarLayout->execute( "close-locatebar" );
		}
	} );
}

} // namespace ecode
