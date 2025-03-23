#pragma once

#include "llmchatcompletionrequest.hpp"
#include "protocol.hpp"

#include "../pluginmanager.hpp"
#include <eepp/ui/uilinearlayout.hpp>

#include "nlohmann/json_fwd.hpp"

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

class LLMChatUI : public UILinearLayout {
  public:
	static LLMChatUI* New( PluginManager* manager ) { return eeNew( LLMChatUI, ( manager ) ); }

	nlohmann::json serialize();

	void unserialize( const nlohmann::json& payload );

  protected:
	UUID mUUID;
	std::string mSummary;
	long mTimestamp;
	PluginManager* mManager{ nullptr };
	UIWidget* mChatsList{ nullptr };
	UICodeEditor* mChatInput{ nullptr };
	UIPushButton* mChatUserRole{ nullptr };
	UIPushButton* mChatRun{ nullptr };
	UIPushButton* mChatStop{ nullptr };
	UIPushButton* mChatHistory{ nullptr };
	UIScrollView* mChatScrollView{ nullptr };
	UIDropDownList* mModelDDL{ nullptr };
	std::unique_ptr<LLMChatCompletionRequest> mRequest;
	std::unique_ptr<LLMChatCompletionRequest> mSummaryRequest;
	LLMProviders mProviders;
	LLMModel mCurModel;
	std::unordered_map<String::HashType, LLMModel> mModelsMap;
	int mPendingModelsToLoad{ 0 };

	LLMModel findModel( const std::string& provider, const std::string& model );

	LLMModel getDefaultModel();

	LLMChatUI( PluginManager* manager );

	AIAssistantPlugin* getPlugin();

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

	void saveChat();

	void onInit();

	void showChatHistory();

	void updateTabTitle();

};

} // namespace ecode
