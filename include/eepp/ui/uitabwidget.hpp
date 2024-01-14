#ifndef EE_UI_UITABWIDGET_HPP
#define EE_UI_UITABWIDGET_HPP

#include <deque>
#include <eepp/ui/uitab.hpp>
#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

class UIPopUpMenu;
class UIScrollBar;

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
	enum class FocusTabBehavior { Closest, FocusOrder, Default };

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

	UITabWidget();

	virtual ~UITabWidget();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	UITab* add( const String& text, UINode* nodeOwned, Drawable* icon = NULL );

	UITabWidget* add( UITab* tab );

	UITab* getTabFromOwnedWidget( const UIWidget* widget );

	UITab* getTab( const Uint32& index );

	UITab* getTab( const String& text );

	Uint32 getTabIndex( UITab* tab );

	Uint32 getTabCount() const;

	void removeTab( const Uint32& index, bool destroyOwnedNode = true, bool immediateClose = false,
					FocusTabBehavior focusTabBehavior = FocusTabBehavior::Default );

	void removeTab( UITab* tab, bool destroyOwnedNode = true, bool immediateClose = false,
					FocusTabBehavior focusTabBehavior = FocusTabBehavior::Default );

	void removeAllTabs( bool destroyOwnedNode = true, bool immediateClose = false );

	void insertTab( const String& text, UINode* nodeOwned, Drawable* icon, const Uint32& index );

	void insertTab( UITab* tab, const Uint32& index );

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

	const bool& getHideTabBarOnSingleTab() const;

	void setHideTabBarOnSingleTab( const bool& hideTabBarOnSingleTab );

	const TabTryCloseCallback& getTabTryCloseCallback() const;

	void setTabTryCloseCallback( const TabTryCloseCallback& tabTryCloseCallback );

	const bool& getAllowRearrangeTabs() const;

	void setAllowRearrangeTabs( bool allowRearrangeTabs );

	bool getAllowDragAndDropTabs() const;

	void setAllowDragAndDropTabs( bool allowDragAndDropTabs );

	const Float& getTabVerticalDragResistance() const;

	void setTabVerticalDragResistance( const Float& tabVerticalDragResistance );

	bool getAllowSwitchTabsInEmptySpaces() const;

	void setAllowSwitchTabsInEmptySpaces( bool allowSwitchTabsInEmptySpaces );

	virtual bool acceptsDropOfWidget( const UIWidget* widget );

	const Color& getDroppableHoveringColor() const;

	void setDroppableHoveringColor( const Color& droppableHoveringColor );

	FocusTabBehavior getFocusTabBehavior() const;

	void setFocusTabBehavior( FocusTabBehavior focusTabBehavior );

	bool getEnabledCreateContextMenu() const;

	void setEnabledCreateContextMenu( bool enabledCreateContextMenu );

	UIScrollBar* getTabScroll() const;

	void swapTabs( UITab* left, UITab* right );

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
	bool mAllowRearrangeTabs{ false };
	bool mAllowDragAndDropTabs{ false };
	bool mAllowSwitchTabsInEmptySpaces{ false };
	bool mDroppableHoveringColorWasSet{ false };
	bool mEnabledCreateContextMenu{ false };
	Float mTabVerticalDragResistance;
	Color mDroppableHoveringColor{ Color::Transparent };
	FocusTabBehavior mFocusTabBehavior{ FocusTabBehavior::Closest };
	std::deque<UITab*> mFocusHistory;
	UIPopUpMenu* mCurrentMenu{ nullptr };

	void onThemeLoaded();

	UITab* createTab( const String& text, UINode* nodeOwned, Drawable* icon );

	void removeTab( const Uint32& index, bool destroyOwnedNode, bool destroyTab,
					bool immediateClose,
					FocusTabBehavior focusTabBehavior = FocusTabBehavior::Default );

	void removeTab( UITab* tab, bool destroyOwnedNode, bool destroyTab, bool immediateClose,
					FocusTabBehavior focusTabBehavior = FocusTabBehavior::Default );

	virtual void onSizeChange();

	virtual void onChildCountChange( Node* child, const bool& removed );

	virtual void onPaddingChange();

	virtual Uint32 onMessage( const NodeMessage* msg );

	void setContainerSize();

	void posTabs();

	void zorderTabs();

	void orderTabs();

	void updateTabs();

	void applyThemeToTabs();

	void refreshOwnedWidget( UITab* tab );

	void tryCloseTab( UITab* tab, FocusTabBehavior focustTabBehavior = FocusTabBehavior::Default );

	void updateScrollBar();

	void updateScroll( bool updateFocus = false );

	void updateTabSelected( FocusTabBehavior tabBehavior );

	void insertFocusHistory( UITab* tab );

	void eraseFocusHistory( UITab* tab );
};

}} // namespace EE::UI

#endif
