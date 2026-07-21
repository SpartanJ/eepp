#ifndef EE_UI_TOOLS_UITABWIDGETSPLITTER_HPP
#define EE_UI_TOOLS_UITABWIDGETSPLITTER_HPP

#include <eepp/system/mutex.hpp>
#include <eepp/ui/splitdirection.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uisplitter.hpp>
#include <eepp/ui/uitabwidget.hpp>

#include <nlohmann/json_fwd.hpp>

namespace EE { namespace Graphics {
class Drawable;
}} // namespace EE::Graphics

namespace EE { namespace UI { namespace Tools {

struct WidgetLoadResult {
	UIWidget* widget{ nullptr };
	Graphics::Drawable* icon{ nullptr };
	std::string title;
};

struct WidgetTypeCallback {
	std::function<nlohmann::json( UIWidget* )> onSave;
	std::function<WidgetLoadResult( const nlohmann::json& )> onLoad;
};

class EE_API UITabWidgetSplitter {
  public:
	class EE_API Client {
	  public:
		virtual ~Client() {}

		virtual void onTabCreated( UITab* tab, UIWidget* widget ) = 0;

		virtual void onWidgetFocusChange( UIWidget* widget ) = 0;
	};

	static UITabWidgetSplitter* New( Client* client, UISceneNode* sceneNode );

	virtual ~UITabWidgetSplitter();

	UITabWidget* createTabWidget( Node* parent );

	std::pair<UITab*, UIWidget*> createWidget( UIWidget* widget, const std::string& tabName,
											   bool focus = true );

	std::pair<UITab*, UIWidget*> createWidgetInTabWidget( UITabWidget* tabWidget, UIWidget* widget,
														  const std::string& tabName,
														  bool focus = true );

	std::vector<std::pair<UITab*, UITabWidget*>> getTabFromOwnedWidgetId( const std::string& id );

	bool ownedWidgetExists( UIWidget* widget );

	bool removeTabWithOwnedWidgetId( const std::string& id, bool destroyOwnedNode = true,
									 bool immediateClose = false );

	UITab* getTabFromWidget( UIWidget* widget ) const;

	virtual bool tryTabClose( UIWidget* widget, UITabWidget::FocusTabBehavior focusTabBehavior,
							  std::function<void()> onMsgBoxCloseCb = {} );

	virtual bool tryCloseAllTabs( UIWidget* widget,
								  UITabWidget::FocusTabBehavior focusTabBehavior );

	virtual bool tryCloseOtherTabs( UIWidget* widget,
									UITabWidget::FocusTabBehavior focusTabBehavior );

	virtual bool tryCloseTabsToDirection( UIWidget* widget,
										  UITabWidget::FocusTabBehavior focusTabBehavior,
										  bool toTheRight );

	void closeTab( UIWidget* widget, UITabWidget::FocusTabBehavior focusTabBehavior );

	UISplitter* split( const SplitDirection& direction, UIWidget* widget );

	UITabWidget* splitTabWidget( SplitDirection direction, UITabWidget* tabWidget );

	void switchToTab( Int32 index );

	UITabWidget* findPreviousSplit( UIWidget* widget );

	void switchPreviousSplit( UIWidget* widget );

	UITabWidget* findNextSplit( UIWidget* widget );

	void switchNextSplit( UIWidget* widget );

	void setCurrentWidget( UIWidget* curWidget );

	void focusSomeWidget( Node* searchFrom = nullptr );

	UIWidget* getSomeWidget();

	UITabWidget* tabWidgetFromWidget( UIWidget* widget ) const;

	UISplitter* splitterFromWidget( UIWidget* widget ) const;

	bool curWidgetExists() const;

	bool hasSplit() const;

	UIOrientation getMainSplitOrientation() const;

	UIWidget* getCurWidget() const;

	const std::vector<UITabWidget*>& getTabWidgets() const;

	UITabWidget* getFirstTabWidget() const;

	Node* getBaseLayout() const;

	UISceneNode* getUISceneNode() const;

	bool getHideTabBarOnSingleTab() const;

	void setHideTabBarOnSingleTab( bool hideTabBarOnSingleTab );

	void setHideTabBar( bool hideTabBar );

	bool getVisualSplitting() const;

	void setVisualSplitting( bool visualSplitting );

	Float getVisualSplitEdgePercent() const;

	void setVisualSplitEdgePercent( Float visualSplitEdgePercent );

	void setOnTabWidgetCreateCb( std::function<void( UITabWidget* )> cb );

	void closeSplitter( UISplitter* splitter );

	void addRemainingTabWidgets( Node* widget );

	void closeTabWidgets( UISplitter* splitter );

	bool checkWidgetExists( UIWidget* ) const;

	bool isWidgetInAnyWidget( UIWidget* ) const;

	void forEachWidget( std::function<void( UIWidget* )> run ) const;

	void forEachWidgetStoppable( std::function<bool( UIWidget* )> run ) const;

	void forEachWidgetClass( const std::string& className,
							 std::function<void( UIWidget* )> run ) const;

	void forEachWidgetClassStoppable( const std::string& className,
									  std::function<bool( UIWidget* )> run ) const;

	void forEachWidgetType( const UINodeType& nodeType,
							std::function<void( UIWidget* )> run ) const;

	void forEachWidgetTypeStoppable( const UINodeType& nodeType,
									 std::function<bool( UIWidget* )> run ) const;

	void forEachTabWidget( std::function<void( UITabWidget* )> run ) const;

	void forEachTabWidgetStoppable( std::function<bool( UITabWidget* )> run ) const;

	void forEachTab( std::function<void( UITab* )> run ) const;

	template <typename T> void registerSplitterCommands( T& t ) {
		t.setCommand( "switch-to-previous-split", [this] { switchPreviousSplit( mCurWidget ); } );
		t.setCommand( "switch-to-next-split", [this] { switchNextSplit( mCurWidget ); } );
		t.setCommand( "close-tab", [this] {
			if ( tryTabClose( mCurWidget, UITabWidget::FocusTabBehavior::Default ) )
				closeTab( mCurWidget, UITabWidget::FocusTabBehavior::Default );
		} );
		t.setCommand( "close-other-tabs", [this] {
			tryCloseOtherTabs( mCurWidget, UITabWidget::FocusTabBehavior::Default );
		} );
		t.setCommand( "close-all-tabs", [this] {
			tryCloseAllTabs( mCurWidget, UITabWidget::FocusTabBehavior::Default );
		} );
		t.setCommand( "close-tabs-to-the-left", [this] {
			tryCloseTabsToDirection( mCurWidget, UITabWidget::FocusTabBehavior::Default, false );
		} );
		t.setCommand( "close-tabs-to-the-right", [this] {
			tryCloseTabsToDirection( mCurWidget, UITabWidget::FocusTabBehavior::Default, true );
		} );
		t.setCommand( "next-tab", [this] {
			UITabWidget* tabWidget = tabWidgetFromWidget( mCurWidget );
			if ( tabWidget )
				tabWidget->focusNextTab();
		} );
		t.setCommand( "previous-tab", [this] {
			UITabWidget* tabWidget = tabWidgetFromWidget( mCurWidget );
			if ( tabWidget )
				tabWidget->focusPreviousTab();
		} );
		for ( int i = 1; i <= 10; i++ )
			t.setCommand( String::format( "switch-to-tab-%d", i ),
						  [this, i] { switchToTab( i - 1 ); } );
		t.setCommand( "switch-to-first-tab", [this] {
			UITabWidget* tabWidget = tabWidgetFromWidget( mCurWidget );
			if ( tabWidget && tabWidget->getTabCount() ) {
				switchToTab( 0 );
			}
		} );
		t.setCommand( "switch-to-last-tab", [this] {
			UITabWidget* tabWidget = tabWidgetFromWidget( mCurWidget );
			if ( tabWidget && tabWidget->getTabCount() ) {
				switchToTab( tabWidget->getTabCount() - 1 );
			}
		} );
		t.setCommand( "split-right", [this] { split( SplitDirection::Right, mCurWidget ); } );
		t.setCommand( "split-bottom", [this] { split( SplitDirection::Bottom, mCurWidget ); } );
		t.setCommand( "split-left", [this] { split( SplitDirection::Left, mCurWidget ); } );
		t.setCommand( "split-top", [this] { split( SplitDirection::Top, mCurWidget ); } );
		t.setCommand( "split-swap", [this] {
			if ( UISplitter* splitter = splitterFromWidget( mCurWidget ) )
				splitter->swap();
		} );
	}

	void registerWidgetType( const std::string& type, const WidgetTypeCallback& cb ) {
		Lock l( mWidgetTypesMutex );
		mWidgetTypes[type] = cb;
	}

	void unregisterWidgetType( const std::string& type ) {
		Lock l( mWidgetTypesMutex );
		mWidgetTypes.erase( type );
	}

	nlohmann::json serializeNode( Node* node ) const;

	void unserializeNode( const nlohmann::json& j, UITabWidget* curTabWidget );

	nlohmann::json toJSON() const;

	void fromJSON( const nlohmann::json& j );

	using TabTryCloseCallback = std::function<bool( UIWidget*, UITabWidget::FocusTabBehavior,
													std::function<void()> onMsgBoxCloseCb )>;

	void setTabTryCloseCallback( TabTryCloseCallback cb );

	void
	setCanCreateSplitFn( std::function<bool( SplitDirection direction, UIWidget* widget )> fn );

  protected:
	UISceneNode* mUISceneNode{ nullptr };
	UIWidget* mCurWidget{ nullptr };
	std::vector<UITabWidget*> mTabWidgets;
	Node* mBaseLayout{ nullptr };
	Client* mClient;
	bool mHideTabBar{ false };
	bool mHideTabBarOnSingleTab{ true };
	bool mVisualSplitting{ true };
	Float mVisualSplitEdgePercent{ 0.1 };
	Mutex mTabWidgetMutex;
	std::function<void( UITabWidget* )> mOnTabWidgetCreateCb;
	std::function<bool( SplitDirection direction, UIWidget* widget )> mCanCreateSplitFn;
	TabTryCloseCallback mTabTryCloseCb;
	mutable Mutex mWidgetTypesMutex;
	std::unordered_map<std::string, WidgetTypeCallback> mWidgetTypes;

	UITabWidgetSplitter( Client* client, UISceneNode* sceneNode );

	virtual void onTabClosed( const TabEvent* tabEvent );

	void closeAllTabs( std::vector<UITab*> tabs, UITabWidget::FocusTabBehavior focusTabBehavior );

	void updateTabWidgetVisualSplitting();
};

}}} // namespace EE::UI::Tools

#endif // EE_UI_TOOLS_UITABWIDGETSPLITTER_HPP
