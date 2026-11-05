#ifndef EE_UI_UITABWIDGET_HPP
#define EE_UI_UITABWIDGET_HPP

#include <deque>
#include <eepp/scene/eventconnection.hpp>
#include <eepp/ui/splitdirection.hpp>
#include <eepp/ui/uitab.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <optional>

namespace EE { namespace UI {

class UIPopUpMenu;
class UIScrollBar;
class UIListView;
class UISplitter;

class EE_API TabEvent : public Event {
  public:
	TabEvent( Node* node, UITab* tabEvent, Uint32 tabIndex, const Uint32& eventType ) :
		Event( node, eventType ), tab( tabEvent ), tabIndex( tabIndex ) {}

	UITab* getTab() const { return tab; }

	Uint32 getTabIndex() const { return tabIndex; }

  protected:
	UITab* tab;
	Uint32 tabIndex;
};

class EE_API UITabWidget : public UIWidget {
  public:
	using SplitFunctionCb = std::function<UITabWidget*( SplitDirection, UITabWidget* )>;

	enum class FocusTabBehavior { Closest, FocusOrder, Default };

	enum class TabJumpMode { Linear, Chronological };

	static TabJumpMode tabJumpModefromString( std::string_view mode ) {
		return String::iequals( mode, "chronological" ) ? TabJumpMode::Chronological
														: TabJumpMode::Linear;
	}

	static std::string tabJumpModeToString( TabJumpMode mode ) {
		return mode == TabJumpMode::Chronological ? "chronological" : "linear";
	}

	/** The edge of the widget that holds the tab bar. Left and Right stack the tabs in rows inside
	 * a side pane. Left and Right are physical edges: they don't follow the text direction. */
	enum class TabPosition : Uint8 { Top, Bottom, Left, Right };

	static TabPosition tabPositionFromString( std::string_view position ) {
		if ( String::iequals( position, "bottom" ) )
			return TabPosition::Bottom;
		if ( String::iequals( position, "left" ) )
			return TabPosition::Left;
		if ( String::iequals( position, "right" ) )
			return TabPosition::Right;
		return TabPosition::Top;
	}

	static std::string tabPositionToString( TabPosition position ) {
		switch ( position ) {
			case TabPosition::Bottom:
				return "bottom";
			case TabPosition::Left:
				return "left";
			case TabPosition::Right:
				return "right";
			case TabPosition::Top:
			default:
				return "top";
		}
	}

	class StyleConfig {
	  public:
		Float TabSeparation = 0;
		Uint32 MaxTextLength = 100;
		Float TabHeight = 0;
		Uint32 TabTextAlign = ( UI_HALIGN_CENTER | UI_VALIGN_CENTER );
		std::string MinTabWidth = "32dp";
		std::string MaxTabWidth = "300dp";
		bool TabsClosable = false;
		bool TabsEdgesDiffSkins = false; //! Indicates if the edge tabs ( the left and right
										 //! border tab ) are different from the central tabs.
		bool TabCloseButtonVisible = true;
	};

	typedef std::function<bool( UITab*, FocusTabBehavior )> TabTryCloseCallback;

	static UITabWidget* New();

	/** @return The tab widget that holds node as a tab page, or nullptr if it isn't a page. */
	static UITabWidget* fromOwnedNode( const Node* node );

	virtual ~UITabWidget();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	UITab* add( const String& text, UINode* nodeOwned, DrawablePtr icon = {} );

	UITabWidget* add( UITab* tab );

	UITab* getTabFromOwnedWidget( const UIWidget* widget );

	UITab* getTab( const Uint32& index );

	UITab* getTab( const String& text );

	UITab* getTabById( const std::string& text );

	Uint32 getTabIndex( UITab* tab );

	Uint32 getTabCount() const;

	void removeTab( const Uint32& index, bool destroyOwnedNode = true, bool immediateClose = false,
					FocusTabBehavior focusTabBehavior = FocusTabBehavior::Default );

	void removeTab( UITab* tab, bool destroyOwnedNode = true, bool immediateClose = false,
					FocusTabBehavior focusTabBehavior = FocusTabBehavior::Default );

	void removeAllTabs( bool destroyOwnedNode = true, bool immediateClose = false );

	void insertTab( const String& text, UINode* nodeOwned, DrawablePtr icon, const Uint32& index );

	void insertTab( UITab* tab, const Uint32& index );

	bool moveTab( UITab* tab, Uint32 index );

	virtual void setTheme( UITheme* theme );

	UITab* getTabSelected() const;

	Uint32 getTabSelectedIndex() const;

	UIWidget* getTabBar() const;

	UIWidget* getContainerNode() const;

	Float getTabSeparation() const;

	void setTabSeparation( const Float& tabSeparation );

	Uint32 getMaxTextLength() const;

	void setMaxTextLength( const Uint32& maxTextLength );

	std::string getMinTabWidth() const;

	void setMinTabWidth( const std::string& minTabWidth );

	std::string getMaxTabWidth() const;

	void setMaxTabWidth( const std::string& maxTabWidth );

	bool getTabsClosable() const;

	void setTabsClosable( bool tabsClosable );

	bool getTabCloseButtonVisible() const;

	void setTabCloseButtonVisible( bool visible );

	bool getSpecialBorderTabs() const;

	void setTabsEdgesDiffSkins( bool diffSkins );

	void setTabsHeight( const Float& height );

	Float getTabsHeight() const;

	TabPosition getTabPosition() const { return mTabPosition; }

	/** Moves the tab bar to another edge. Tabs, pages, selection and focus history are preserved.
	 * The widget also holds the class of its position (tabs-top, tabs-bottom, tabs-left or
	 * tabs-right) and of its orientation (tabs-horizontal or tabs-vertical). */
	void setTabPosition( TabPosition position );

	bool isTabBarVertical() const {
		return mTabPosition == TabPosition::Left || mTabPosition == TabPosition::Right;
	}

	/** @return The width in dp of the Left/Right tab bar, including the last user resize. It's
	 * retained while the bar is hidden or the tabs are in a horizontal position. Dragging the
	 * separator sends Event::OnValueChange, so the owner can remember the width. */
	Float getTabBarWidth() const { return mTabBarWidth; }

	void setTabBarWidth( Float widthDp );

	const StyleConfig& getStyleConfig() const;

	void setStyleConfig( const StyleConfig& styleConfig );

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

	virtual bool isDrawInvalidator() const;

	void invalidate( Node* invalidator );

	void selectPreviousTab();

	void selectNextTab();

	UITab* setTabSelected( UITab* tab );

	UITab* setTabSelected( const Uint32& tabIndex );

	const bool& getHideTabBar() const;

	const bool& getHideTabBarOnSingleTab() const;

	void setHideTabBarOnSingleTab( const bool& hideTabBarOnSingleTab );

	void setHideTabBar( const bool& hideTabBar );

	const TabTryCloseCallback& getTabTryCloseCallback() const;

	void setTabTryCloseCallback( const TabTryCloseCallback& tabTryCloseCallback );

	const bool& getAllowRearrangeTabs() const;

	void setAllowRearrangeTabs( bool allowRearrangeTabs );

	bool allowDragAndDropTabs() const;

	void setAllowDragAndDropTabs( bool allowDragAndDropTabs );

	/** @return The distance a tab must be dragged away from the tab bar to detach it. It's vertical
	 * for Top/Bottom tabs and horizontal for Left/Right tabs. */
	const Float& getTabVerticalDragResistance() const;

	void setTabVerticalDragResistance( const Float& tabVerticalDragResistance );

	bool getAllowSwitchTabsInEmptySpaces() const;

	void setAllowSwitchTabsInEmptySpaces( bool allowSwitchTabsInEmptySpaces );

	virtual bool acceptsDropOfWidget( const UIWidget* widget );

	void setAcceptsDropOfWidgetFn( std::function<bool( const UIWidget* widget )> fn );

	const Color& getDroppableHoveringColor() const;

	void setDroppableHoveringColor( const Color& droppableHoveringColor );

	FocusTabBehavior getFocusTabBehavior() const;

	void setFocusTabBehavior( FocusTabBehavior focusTabBehavior );

	bool getEnabledCreateContextMenu() const;

	void setEnabledCreateContextMenu( bool enabledCreateContextMenu );

	/** @return The tab bar scrollbar. Changing between horizontal and vertical positions replaces
	 * it, so don't keep the pointer across position changes. */
	UIScrollBar* getTabScroll() const;

	bool swapTabs( UITab* left, UITab* right );

	void setSplitFunction( SplitFunctionCb cb, Float splitEdgePercent = 0.1 );

	const std::deque<UITab*>& getFocusHistory() const { return mFocusHistory; }

	void setEnableTabSwitcher( bool enable ) { mEnableTabSwitcher = enable; }

	bool isTabSwitcherEnabled() const { return mEnableTabSwitcher; }

	void focusNextTab( const std::vector<Keycode>& tabSwitcherMetaTrigger = {} );

	void focusPreviousTab( const std::vector<Keycode>& tabSwitcherMetaTrigger = {} );

	void setTabJumpMode( TabJumpMode mode ) { mTabJumpMode = mode; }

	TabJumpMode getTabJumpMode() const { return mTabJumpMode; }

	void forEachTab( std::function<void( UITab* )> fn, Uint32 filterOwnedType = UI_TYPE_UNKNOWN );

  protected:
	friend class UITab;

	UIWidget* mNodeContainer;
	UIWidget* mTabBar;
	UIScrollBar* mTabScroll;
	StyleConfig mStyleConfig;
	std::deque<UITab*> mTabs;
	UITab* mTabSelected;
	Uint32 mTabSelectedIndex;
	TabTryCloseCallback mTabTryCloseCallback;
	bool mHideTabBarOnSingleTab{ false };
	bool mHideTabBar{ false };
	bool mAllowRearrangeTabs{ false };
	bool mAllowDragAndDropTabs{ false };
	bool mAllowSwitchTabsInEmptySpaces{ false };
	bool mDroppableHoveringColorWasSet{ false };
	bool mEnabledCreateContextMenu{ false };
	bool mEnableTabSwitcher{ false };
	bool mTabSwitcherRunning{ false };
	bool mPositioningTabRows{ false };
	TabPosition mTabPosition{ TabPosition::Top };
	Float mTabVerticalDragResistance;
	Float mTabBarWidth{ 200.f };
	Color mDroppableHoveringColor{ Color::Transparent };
	FocusTabBehavior mFocusTabBehavior{ FocusTabBehavior::Closest };
	std::deque<UITab*> mFocusHistory;
	std::deque<UITab*> mFocusHistoryFreezed;
	UIPopUpMenu* mCurrentMenu{ nullptr };
	SplitFunctionCb mSplitFn;
	Float mSplitEdgePercent{ 0.1 };
	UIListView* mTabSwitcher{ nullptr };
	// Only exist while the tabs are Left/Right: mInternalSplitter holds mNodeContainer and
	// mTabBarViewport, which holds and clips the vertically scrolled mTabBar.
	UISplitter* mInternalSplitter{ nullptr };
	UIWidget* mTabBarViewport{ nullptr };
	Uint32 mContainerSizeCb{ 0 };
	TabJumpMode mTabJumpMode{ TabJumpMode::Linear };
	std::function<bool( const UIWidget* widget )> mAcceptsDropOfWidgetFn;
	Scene::EventConnectionList mEventConnections;

	UITabWidget();

	void onThemeLoaded();

	UITab* createTab( const String& text, UINode* nodeOwned, DrawablePtr icon );

	void removeTab( const Uint32& index, bool destroyOwnedNode, bool destroyTab,
					bool immediateClose,
					FocusTabBehavior focusTabBehavior = FocusTabBehavior::Default );

	void removeTab( UITab* tab, bool destroyOwnedNode, bool destroyTab, bool immediateClose,
					FocusTabBehavior focusTabBehavior = FocusTabBehavior::Default );

	virtual void onSizeChange();

	virtual void onChildCountChange( Node* child, const bool& removed );

	virtual void onPaddingChange();

	virtual void onClassChange();

	virtual Uint32 onMessage( const NodeMessage* msg );

	virtual Uint32 onMouseWheel( const Vector2f& offset, bool flipped );

	virtual void drawDroppableHovering();

	void createTabScroll();

	void setContainerSize();

	bool isTabBarHidden() const;

	bool isSideTabBarCollapsed() const;

	void createInternalSplitter();

	void removeInternalSplitter();

	void updateInternalSplitterOrder();

	void updateInternalSplitterLayout();

	void posTabs();

	void posTabRows();

	void zorderTabs();

	void orderTabs();

	void updateTabs();

	void applyThemeToTabs();

	void refreshOwnedWidget( UITab* tab );

	void tryCloseTab( UITab* tab, FocusTabBehavior focustTabBehavior = FocusTabBehavior::Default );

	void updateScrollBar();

	void updateVerticalScrollBar();

	Float getTabsExtent() const;

	virtual void onDirectionChanged();

	void updateScroll( bool updateFocus = false );

	void updateVerticalScroll( bool updateFocus );

	void updateTabSelected( FocusTabBehavior tabBehavior );

	void insertFocusHistory( UITab* tab );

	void eraseFocusHistory( UITab* tab );

	Rectf getDropBounds() const;

	std::optional<SplitDirection> getDropDirection() const;

	void createTabSwitcher( const std::vector<Keycode>& tabSwitcherMetaTrigger,
							bool fromPrev = false );

	void enableTabSwitcher( const std::vector<Keycode>& tabSwitcherMetaTrigger );

	Uint32 getTabSelectedFocusHistoryIndex() const;

	Uint32 getTabSelectedFocusHistoryFreezedIndex() const;
};

}} // namespace EE::UI

#endif
