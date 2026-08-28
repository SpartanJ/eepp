#ifndef ECODE_STATUSDEBUGGERCONTROLLER_HPP
#define ECODE_STATUSDEBUGGERCONTROLLER_HPP

#include "../../uistatusbar.hpp"
#include "models/breakpointsmodel.hpp"
#include <eepp/core/containers.hpp>
#include <eepp/scene/eventconnection.hpp>
#include <eepp/scene/mainthreadlifetime.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/ui/tools/uicodeeditorsplitter.hpp>
#include <eepp/ui/tools/uitabwidgetsplitter.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uirelativelayout.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uisplitter.hpp>
#include <eepp/ui/uitableview.hpp>
#include <eepp/ui/uitreeview.hpp>

using namespace EE;
using namespace EE::UI;
using namespace EE::UI::Tools;

namespace ecode {

class UIHLinearLayoutCommandExecuter;

class UIBreakpointsTableView : public UITableView {
  public:
	static UIWidget* New() { return eeNew( UIBreakpointsTableView, () ); }

	UIBreakpointsTableView() : UITableView() {}

	std::function<void( const std::string& file, int line, bool enabled )>
		onBreakpointEnabledChange;

	std::function<void( const std::string& file, int line )> onBreakpointRemove;

	UIWidget* createCell( UIWidget* rowWidget, const ModelIndex& index );
};

class StatusDebuggerController : public StatusBarElement, public UITabWidgetSplitter::Client {
  public:
	enum class State { NotStarted, Running, Paused };

	static const std::map<KeyBindings::Shortcut, std::string> getLocalDefaultKeybindings();

	StatusDebuggerController( UISplitter* mainSplitter, UISceneNode* uiSceneNode,
							  PluginContextProvider* pluginContext );

	virtual ~StatusDebuggerController();

	virtual void show();

	virtual void hide();

	UIWidget* getWidget();

	UIWidget* createWidget();

	UITableView* getUIThreads();

	UITableView* getUIStack();

	UIBreakpointsTableView* getUIBreakpoints();

	UITreeView* getUIVariables() const;

	UITreeView* getUIExpressions() const;

	UITabWidget* getUITabWidget() const;

	UICodeEditor* getUIConsole() const;

	bool getConsoleScrollLocked() const { return mScrollLocked; }

	void insertConsoleBuffer( std::string&& buffer );

	void clearConsoleBuffer();

	void setDebuggingState( State state );

	const std::string& saveLayout();

	std::function<void( StatusDebuggerController*, UIWidget* )> onWidgetCreated{ nullptr };

	void onTabCreated( UITab* tab, UIWidget* widget );

	void onWidgetFocusChange( UIWidget* widget );

  protected:
	UIHLinearLayoutCommandExecuter* mContainer{ nullptr };
	UITableView* mUIThreads{ nullptr };
	UITableView* mUIStack{ nullptr };
	UIBreakpointsTableView* mUIBreakpoints{ nullptr };
	UITreeView* mUIVariables{ nullptr };
	UITreeView* mUIExpressions{ nullptr };
	UISplitter* mUIThreadsSplitter{ nullptr };
	UICodeEditor* mUIConsole{ nullptr };
	UIPushButton* mUIButStart{ nullptr };
	UIPushButton* mUIButStop{ nullptr };
	UIPushButton* mUIButContinue{ nullptr };
	UIPushButton* mUIButPause{ nullptr };
	UIPushButton* mUIButStepInto{ nullptr };
	UIPushButton* mUIButStepOver{ nullptr };
	UIPushButton* mUIButStepOut{ nullptr };
	UITabWidget* mUITabWidget{ nullptr };
	UITabWidget* mUIRightTabWidget{ nullptr };
	UITabWidgetSplitter* mTabWidgetSplitter{ nullptr };
	UILayout* mDebuggerTabsContainer{ nullptr };
	UILayout* mRightPanelContainer{ nullptr };
	Scene::EventConnectionList mEventConnections;
	UnorderedMap<UITabWidget*, Scene::EventConnectionList> mTabWidgetEventConnections;
	bool mScrollLocked{ true };
	bool mRestoringLayout{ false };
	bool mRightPanelDropPreview{ false };
	std::string mSerializedLayout;
	MainThreadLifetime<StatusDebuggerController> mLifetime;

	void createContainer();

	void createTabWidgets();

	void restoreTabLayout();

	void saveTabLayout();

	bool rightPanelHasTabs() const;

	void beginRightPanelDropPreview();

	void endRightPanelDropPreview();

	void updateRightPanel();
};

} // namespace ecode

#endif // ECODE_STATUSDEBUGGERCONTROLLER_HPP
