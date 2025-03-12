#pragma once

#include "llmchatcompletionrequest.hpp"
#include "protocol.hpp"

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

class LLMChat {
  public:
	enum class Role {
		User,
		Assistant,
		System,
		Tool,
	};

	static const char* roleToString( Role role );

	static LLMChat::Role stringToRole( UIPushButton* userBut );
};

class ChatUI {
  public:
	ChatUI( UISceneNode* ui, LLMProviders providers );

	nlohmann::json serialize( const std::string& /*provider*/ );

	void unserialize( const nlohmann::json& /*payload*/ );

	const char* getApiKeyFromProvider( const std::string& provider );

	UIWidget* getChatUI();

  protected:
	UIWidget* mChatUI{ nullptr };
	UIWidget* mChatsList{ nullptr };
	UICodeEditor* mChatInput{ nullptr };
	UIPushButton* mChatUserRole{ nullptr };
	UIPushButton* mChatRun{ nullptr };
	UIPushButton* mChatStop{ nullptr };
	UIScrollView* mChatScrollView{ nullptr };
	UIDropDownList* mModelDDL{ nullptr };
	std::unique_ptr<LLMChatCompletionRequest> mRequest;
	LLMProviders mProviders;
	LLMModel mCurModel;
	std::unordered_map<String::HashType, LLMModel> mModelsMap;

	void showMsg( String msg );

	nlohmann::json chatToJson( const std::string& /*provider*/ );

	std::string prepareApiUrl( const std::string& apiKey );

	void doRequest();

	void toggleEnableChat( UIWidget* chat, bool enabled );

	void toggleEnableChats( bool enabled );

	Drawable* findIcon( const std::string& name, const size_t iconSize );

	UIWidget* addChatUI( LLMChat::Role role );

	void fillApiModels( UIDropDownList* modelDDL );

	void fillModelDropDownList( UIDropDownList* modelDDL );

	void resizeToFit( UICodeEditor* editor );

	void addChat( LLMChat::Role role, std::string conversation );

	void removeLastChat();

	void setProviders( LLMProviders&& providers );
};

} // namespace ecode
