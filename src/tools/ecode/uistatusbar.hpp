#ifndef ECODE_UISTATUSBAR_HPP
#define ECODE_UISTATUSBAR_HPP

#include <eepp/ui/tools/uicodeeditorsplitter.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uisplitter.hpp>
#include <eepp/ui/widgetcommandexecuter.hpp>

using namespace EE;
using namespace EE::UI;

namespace ecode {

class PluginContextProvider;

class StatusBarElement {
  public:
	StatusBarElement( UISplitter* mainSplitter, UISceneNode* uiSceneNode,
					  PluginContextProvider* app );

	virtual void toggle();

	virtual void hide();

	virtual void show();

	virtual UIWidget* getWidget() = 0;

	virtual UIWidget* createWidget() = 0;

  protected:
	UISplitter* mMainSplitter;
	UISceneNode* mUISceneNode;
	PluginContextProvider* mContext;
	Tools::UICodeEditorSplitter* mSplitter;
};

class UIStatusBar : public UILinearLayout, public WidgetCommandExecuter {
  public:
	static UIStatusBar* New();

	UIStatusBar();

	void updateState();

	UIPushButton* insertStatusBarElement( std::string id, const String& text,
										  const std::string& icon,
										  std::shared_ptr<StatusBarElement> element );

	void removeStatusBarElement( const std::string& id );

	void setPluginContextProvider( PluginContextProvider* app );

	std::shared_ptr<StatusBarElement> getStatusBarElement( const std::string& id ) const;

	void hideAllElements();

  protected:
	UnorderedMap<std::string, std::pair<UIPushButton*, std::shared_ptr<StatusBarElement>>>
		mElements;

	virtual Uint32 onMessage( const NodeMessage* msg );

	PluginContextProvider* mContext{ nullptr };

	virtual void onVisibilityChange();

	virtual void onChildCountChange( Node* child, const bool& removed );
};

} // namespace ecode

#endif // ECODE_UISTATUSBAR_HPP
