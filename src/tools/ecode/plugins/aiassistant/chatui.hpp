#pragma once

#include "../pluginmanager.hpp"
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
}} // namespace EE::UI

namespace EE { namespace Graphics {
class Drawable;
}} // namespace EE::Graphics

using namespace EE;
using namespace EE::UI;
using namespace EE::Graphics;

namespace ecode {

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
	UISelectButton* mChatPrivate{ nullptr };
	UIScrollView* mChatScrollView{ nullptr };
	UIDropDownList* mModelDDL{ nullptr };
	std::unique_ptr<LLMChatCompletionRequest> mRequest;
	std::unique_ptr<LLMChatCompletionRequest> mSummaryRequest;
	LLMProviders mProviders;
	LLMModel mCurModel;
	std::unordered_map<String::HashType, LLMModel> mModelsMap;
	int mPendingModelsToLoad{ 0 };
	bool mChatIsPrivate{ false };
	bool mChatLocked{ false };

	LLMModel findModel( const std::string& provider, const std::string& model );

	LLMModel getDefaultModel();

	LLMChatUI( PluginManager* manager );

	AIAssistantPlugin* getPlugin() const;

	void showMsg( String msg );

	nlohmann::json serializeChat( const LLMModel& model );

	nlohmann::json chatToJson();

	std::string prepareApiUrl( const std::string& apiKey );

	void doRequest();

	void toggleEnableChat( UIWidget* chat, bool enabled );

	void toggleEnableChats( bool enabled );

	Drawable* findIcon( const std::string& name, const size_t iconSize );

	UIWidget* addChatUI( LLMChat::Role role );

	void fillApiModels( UIDropDownList* modelDDL );

	String getModelDisplayName( const LLMModel& model ) const;

	bool selectModel( UIDropDownList* modelDDL, const LLMModel& model );

	void fillModelDropDownList( UIDropDownList* modelDDL );

	void resizeToFit( UICodeEditor* editor );

	void addChat( LLMChat::Role role, std::string conversation );

	void removeLastChat();

	void setProviders( LLMProviders&& providers );

	virtual Uint32 onMessage( const NodeMessage* );

	const LLMModel& getCheapestModelFromCurrentProvider() const;

	std::optional<LLMModel> getModel( const std::string& provider, const std::string& modelName );

	void saveChat();

	void onInit();

	void showChatHistory();

	void bindCmds( UICodeEditor* editor, bool bindToChatUI );

	void addKb( UICodeEditor* editor, std::string kb, const std::string& cmd,
				bool bindToChatUI = false, bool searchDefined = true );

	void deleteOldConversations( int days );
};

} // namespace ecode
