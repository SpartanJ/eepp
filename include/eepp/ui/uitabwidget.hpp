#ifndef EE_UICUITABWIDGET_HPP
#define EE_UICUITABWIDGET_HPP

#include <deque>
#include <eepp/ui/uitab.hpp>
#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

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
	class StyleConfig {
	  public:
		Float TabSeparation = 0;
		Uint32 MaxTextLength = 100;
		Float TabHeight = 0;
		Uint32 TabTextAlign = ( UI_HALIGN_CENTER | UI_VALIGN_CENTER );
		Float MinTabWidth = 32;
		Float MaxTabWidth = 300;
		bool TabsClosable = false;
		bool TabsEdgesDiffSkins = false; //! Indicates if the edge tabs ( the left and right
										 //! border tab ) are different from the central tabs.
	};

	typedef std::function<bool( UITab* )> TabTryCloseCallback;

	static UITabWidget* New();

	UITabWidget();

	virtual ~UITabWidget();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	UITab* add( const String& text, UINode* nodeOwned, Drawable* icon = NULL );

	UITabWidget* add( UITab* Tab );

	UITab* getTab( const Uint32& Index );

	UITab* getTab( const String& Text );

	Uint32 getTabIndex( UITab* Tab );

	Uint32 getTabCount() const;

	void removeTab( const Uint32& index, bool destroyOwnedNode = true );

	void removeTab( UITab* Tab, bool destroyOwnedNode = true );

	void removeAllTabs( bool destroyOwnedNode = true );

	void insertTab( const String& Text, UINode* CtrlOwned, Drawable* Icon, const Uint32& Index );

	void insertTab( UITab* Tab, const Uint32& Index );

	virtual void setTheme( UITheme* Theme );

	UITab* getTabSelected() const;

	Uint32 getTabSelectedIndex() const;

	UIWidget* getTabBar() const;

	UIWidget* getControlContainer() const;

	Float getTabSeparation() const;

	void setTabSeparation( const Float& tabSeparation );

	Uint32 getMaxTextLength() const;

	void setMaxTextLength( const Uint32& maxTextLength );

	Float getMinTabWidth() const;

	void setMinTabWidth( const Float& minTabWidth );

	Float getMaxTabWidth() const;

	void setMaxTabWidth( const Float& maxTabWidth );

	bool getTabsClosable() const;

	void setTabsClosable( bool tabsClosable );

	bool getSpecialBorderTabs() const;

	void setTabsEdgesDiffSkins( bool diffSkins );

	void setTabsHeight( const Float& height );

	Float getTabsHeight() const;

	const StyleConfig& getStyleConfig() const;

	void setStyleConfig( const StyleConfig& styleConfig );

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 );

	virtual bool isDrawInvalidator() const;

	void invalidate( Node* invalidator );

	void selectPreviousTab();

	void selectNextTab();

	UITab* setTabSelected( UITab* Tab );

	UITab* setTabSelected( const Uint32& tabIndex );

	const bool& getHideTabBarOnSingleTab() const;

	void setHideTabBarOnSingleTab( const bool& hideTabBarOnSingleTab );

	const TabTryCloseCallback& getTabTryCloseCallback() const;

	void setTabTryCloseCallback( const TabTryCloseCallback& tabTryCloseCallback );

	const bool& getAllowRearrangeTabs() const;

	void setAllowRearrangeTabs( bool allowRearrangeTabs );

  protected:
	friend class UITab;

	UIWidget* mCtrlContainer;
	UIWidget* mTabBar;
	StyleConfig mStyleConfig;
	std::deque<UITab*> mTabs;
	UITab* mTabSelected;
	Uint32 mTabSelectedIndex;
	TabTryCloseCallback mTabTryCloseCallback;
	bool mHideTabBarOnSingleTab;
	bool mAllowRearrangeTabs;

	void onThemeLoaded();

	UITab* createTab( const String& text, UINode* nodeOwned, Drawable* icon );

	virtual void onSizeChange();

	virtual void onChildCountChange( Node* child, const bool& removed );

	virtual void onPaddingChange();

	void setContainerSize();

	void posTabs();

	void zorderTabs();

	void orderTabs();

	void updateTabs();

	void applyThemeToTabs();

	void refreshOwnedWidget( UITab* tab );

	void tryCloseTab( UITab* tab );

	void swapTabs( UITab* left, UITab* right );
};

}} // namespace EE::UI

#endif
