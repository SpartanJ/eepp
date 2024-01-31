#ifndef ECODE_UISTATUSBAR_HPP
#define ECODE_UISTATUSBAR_HPP

#include <eepp/ui/tools/uicodeeditorsplitter.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uisplitter.hpp>
#include <eepp/ui/widgetcommandexecuter.hpp>

using namespace EE;
using namespace EE::UI;

namespace ecode {

class App;

class StatusBarElement {
  public:
	StatusBarElement( UISplitter* mainSplitter, UISceneNode* uiSceneNode, App* app );

	virtual void toggle();

	virtual void hide();

	virtual void show();

	virtual UIWidget* getWidget() = 0;

	virtual UIWidget* createWidget() = 0;

  protected:
	UISplitter* mMainSplitter;
	UISceneNode* mUISceneNode;
	App* mApp;
	Tools::UICodeEditorSplitter* mSplitter;
};

class UIStatusBar : public UILinearLayout, public WidgetCommandExecuter {
  public:
	static UIStatusBar* New();

	UIStatusBar();

	void updateState();

  protected:
	friend class App;
	virtual Uint32 onMessage( const NodeMessage* msg );

	void setApp( App* app );

	App* mApp{ nullptr };

	virtual void onVisibilityChange();

	virtual void onChildCountChange( Node* child, const bool& removed );
};

} // namespace ecode

#endif // ECODE_UISTATUSBAR_HPP
