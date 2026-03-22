#pragma once

#include "../pluginmanager.hpp"
#include "acp/agentsession.hpp"
#include "llmchatcompletionrequest.hpp"
#include "protocol.hpp"

#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/widgetcommandexecuter.hpp>

#include <nlohmann/json_fwd.hpp>

namespace EE { namespace UI {
class UIWidget;
class UICodeEditor;
class UIScrollView;
class UISceneNode;
class UIDropDownList;
class UIPushButton;
class UITableView;
}} // namespace EE::UI

namespace EE { namespace Graphics {
class Drawable;
}} // namespace EE::Graphics

using namespace EE;
using namespace EE::UI;
using namespace EE::Graphics;

namespace ecode {

class UIVLinearLayoutCommandExecuter;
class AIAssistantPlugin;

class LLMChat {
  public:
	enum class Role {
		User,
		Assistant,
		System,
		Tool,
	};

	static const char* roleToString( Role role );

	static LLMChat::Role stringToRole( const std::string& roleStr );

	static LLMChat::Role stringToRole( UIPushButton* userBut );
};

class LLMChatUI : public UILinearLayout, public WidgetCommandExecuter {
  public:
	static LLMChatUI* New( PluginManager* manager ) { return eeNew( LLMChatUI, ( manager ) ); }

	virtual ~LLMChatUI();

	nlohmann::json serialize();

	std::string unserialize( const nlohmann::json& payload ); // returns the input value

	UISplitter* getSplitter() const;

	const LLMModel& getCurModel() const;

	const UUID& getUUID() const { return mUUID; }

	const std::string& getSummary() const { return mSummary; }

	long getTimestamp() const { return mTimestamp; }

	bool hasChat() const { return getTimestamp() && !getSummary().empty(); }

	std::string getNewFilePath( const std::string& uuid, const std::string& summary,
								bool isLocked ) const;

	std::string getFilePath() const;

	void updateTabTitle();

	void renameChat( const std::string& newName, bool invertLockedState = false );

	virtual Uint32 onKeyDown( const KeyEvent& event ) {
		return WidgetCommandExecuter::onKeyDown( event );
	}

	UICodeEditor* getChatInput() const { return mChatInput; }

	bool isLocked() const { return mChatLocked; }

	void setManager( PluginManager* manager ) { mManager = manager; }

	bool chatExistsInDisk() const;

	const std::string& getCurAgent() const { return mCurAgent; }

  protected:
	UUID mUUID;
	std::string mSummary;
	long mTimestamp{ 0 };
	PluginManager* mManager{ nullptr };
	UISplitter* mChatSplitter{ nullptr };
	UIWidget* mChatsList{ nullptr };
	UICodeEditor* mChatInput{ nullptr };
	UIPushButton* mChatAdd{ nullptr };
	UIPushButton* mChatSettings{ nullptr };
	UIPushButton* mChatUserRole{ nullptr };
	UIPushButton* mChatRun{ nullptr };
	UIPushButton* mChatStop{ nullptr };
	UIPushButton* mChatHistory{ nullptr };
	UIPushButton* mChatMore{ nullptr };
	UIPushButton* mRefreshModels{ nullptr };
	UIPushButton* mChatAttach{ nullptr };
	UISelectButton* mChatPrivate{ nullptr };
	UISelectButton* mChatAgentMode{ nullptr };
	UIScrollView* mChatScrollView{ nullptr };
	UIPushButton* mModelBtn{ nullptr };
	UIPushButton* mAgentBtn{ nullptr };

	// Locate file
	UIVLinearLayoutCommandExecuter* mLocateBarLayout{ nullptr };
	UITextInput* mLocateInput{ nullptr };
	UITableView* mLocateTable{ nullptr };

	// Select model
	UIVLinearLayoutCommandExecuter* mLocateModelBarLayout{ nullptr };
	UITextInput* mLocateModelInput{ nullptr };
	UITableView* mLocateModelTable{ nullptr };

	// Select agent
	UIVLinearLayoutCommandExecuter* mLocateAgentBarLayout{ nullptr };
	UITextInput* mLocateAgentInput{ nullptr };
	UITableView* mLocateAgentTable{ nullptr };

	std::unique_ptr<LLMChatCompletionRequest> mRequest;
	std::unique_ptr<LLMChatCompletionRequest> mSummaryRequest;
	LLMProviders mProviders;
	LLMModel mCurModel;
	std::vector<LLMModel> mModels;

	std::map<std::string, ACPAgent> mAgents;
	std::string mCurAgent;

	struct SlashCommand {
		std::string name;
		std::string description;
	};
	std::vector<SlashCommand> mAvailableCommands;

	std::unique_ptr<acp::AgentSession> mAgentSession;
	UIWidget* mThinkingBubble{ nullptr };
	std::string mCurThinking;
	std::string mCurToolCall;
	int mPendingModelsToLoad{ 0 };
	bool mChatIsPrivate{ false };
	bool mIsAgentMode{ false };
	bool mChatLocked{ false };
	bool mLinkMode{ false };
	std::vector<LLMModel> mNewModels;

	LLMModel findModel( const std::string& provider, const std::string& model );

	LLMModel getDefaultModel();

	LLMChatUI( PluginManager* manager );

	AIAssistantPlugin* getPlugin() const;

	void showMsg( String msg );

	nlohmann::json serializeChat( const LLMModel& model, bool forRequest = false );

	nlohmann::json chatToJson( bool forRequest );

	std::string prepareApiUrl( const std::string& apiKey );

	void doRequest();

	void doAgentRequest();

	void sendAgentPrompt();

	void toggleEnableChat( UIWidget* chat, bool enabled );

	void toggleEnableChats( bool enabled );

	Drawable* findIcon( const std::string& name, const size_t iconSize );

	UIWidget* addChatUI( LLMChat::Role role );

	UIWidget* addMarkdownBubble( const std::string& layout, const std::string& markdown );

	void addPlanBubble( const std::string& markdown );

	void addToolCallBubble( const std::string& markdown );

	void addThinkingBubble();

	void updateThinkingBubble( const std::string& chunk );

	void addPermissionUI( const acp::RequestPermissionRequest& req,
						  std::function<void( const acp::RequestPermissionResponse& )> cb );

	void fillApiModels();

	String getModelDisplayName( const LLMModel& model ) const;

	bool selectModel( std::optional<LLMModel> model );

	bool selectAgent( const std::string& agent );

	void fillModelDropDownList();

	void loadSelectAgent();

	void showSelectAgent();

	void initSelectAgent();

	void hideSelectAgent();

	void updateLocateAgentBarColumns();

	void updateAgentModeUI();

	void setupAgentSession();

	void resizeToFit( UICodeEditor* editor );

	void addChat( LLMChat::Role role, std::string conversation );

	void writeToLastChat( const std::string& text );

	void removeLastChat();

	void setProviders( LLMProviders&& providers );

	virtual Uint32 onMessage( const NodeMessage* );

	const LLMModel& getCheapestModelFromCurrentProvider() const;

	std::optional<LLMModel> getModel( const std::string& provider, const std::string& modelName );

	std::optional<LLMModel> getModel( Uint64 hash );

	void saveChat();

	void onInit();

	void showChatHistory();

	void bindCmds( UICodeEditor* editor, bool bindToChatUI );

	void addKb( UICodeEditor* editor, std::string kb, const std::string& cmd,
				bool bindToChatUI = false, bool searchDefined = true );

	void deleteOldConversations( int days );

	void initAttachFile();

	void initSelectModel();

	void updateLocateBarColumns();

	void showAttachFile();

	void hideAttachFile();

	void updateLocateModelBarColumns();

	void loadSelectModel();

	void showSelectModel();

	void hideSelectModel();

	void insertFileToDocument( std::string path, std::shared_ptr<TextDocument> cdoc );

	void replaceFileLinksToContents( std::string& text );

	nlohmann::json promptToContentBlocks( std::string text );

	void generateChatName( bool isRenaming );

	void regenerateChatName();

	void removeWaitingBubble();

	UIWidget* getLastConversation( bool skipEmpty = false ) const;
};

} // namespace ecode
