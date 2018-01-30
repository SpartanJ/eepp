#ifndef EE_UICUITABWIDGET_HPP
#define EE_UICUITABWIDGET_HPP

#include <eepp/ui/uiwidget.hpp>
#include <eepp/ui/uitab.hpp>
#include <deque>

namespace EE { namespace UI {

class EE_API UITabWidget : public UIWidget {
	public:
		static UITabWidget * New();

		UITabWidget();

		virtual ~UITabWidget();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		UITabWidget * add( const String& Text, UINode * CtrlOwned, Drawable * Icon = NULL );

		UITabWidget * add( UITab * Tab );

		UITab * getTab( const Uint32& Index );

		UITab * getTab( const String& Text );

		Uint32 getTabIndex( UITab * Tab );

		Uint32 getCount() const;

		void remove( const Uint32& Index );

		void remove( UITab * Tab );

		void removeAll();

		void insert( const String& Text, UINode * CtrlOwned, Drawable * Icon, const Uint32& Index );

		void insert( UITab * Tab, const Uint32& Index );

		virtual void setTheme( UITheme * Theme );

		UITab * getSelectedTab() const;

		Uint32 getSelectedTabIndex() const;

		UIWidget * getTabContainer() const;

		UIWidget * getControlContainer() const;

		virtual void draw();

		Font * getFont() const;

		void setFont(Font * font);

		Color getFontColor() const;

		void setFontColor(const Color & fontColor);

		Color getFontShadowColor() const;

		void setFontShadowColor(const Color & fontShadowColor);

		Color getFontOverColor() const;

		void setFontOverColor(const Color & fontOverColor);

		Color getFontSelectedColor() const;

		void setFontSelectedColor(const Color & fontSelectedColor);

		Uint32 getCharacterSize();

		void setCharacterSize(const Uint32 & characterSize);

		const Uint32& getFontStyle() const;

		UITabWidget * setFontStyle( const Uint32& fontStyle );

		const Float & getOutlineThickness() const;

		UITabWidget * setOutlineThickness( const Float& outlineThickness );

		const Color& getOutlineColor() const;

		UITabWidget * setOutlineColor( const Color& outlineColor );

		Int32 getTabSeparation() const;

		void setTabSeparation(const Int32 & tabSeparation);

		Uint32 getMaxTextLength() const;

		void setMaxTextLength(const Uint32 & maxTextLength);

		Uint32 getTabWidgetHeight() const;

		Uint32 getMinTabWidth() const;

		void setMinTabWidth(const Uint32 & minTabWidth);

		Uint32 getMaxTabWidth() const;

		void setMaxTabWidth(const Uint32 & maxTabWidth);

		bool getTabsClosable() const;

		void setTabsClosable(bool tabsClosable);

		bool getSpecialBorderTabs() const;

		void setSpecialBorderTabs(bool specialBorderTabs);

		bool getDrawLineBelowTabs() const;

		void setDrawLineBelowTabs(bool drawLineBelowTabs);

		Color getLineBelowTabsColor() const;

		void setLineBelowTabsColor(const Color & lineBelowTabsColor);

		Int32 getLineBelowTabsYOffset() const;

		void setLineBelowTabsYOffset(const Int32 & lineBelowTabsYOffset);

		UITooltipStyleConfig getFontStyleConfig() const;

		void setFontStyleConfig(const UITooltipStyleConfig & fontStyleConfig);

		UITabWidgetStyleConfig getStyleConfig() const;

		void setStyleConfig(const UITabWidgetStyleConfig & styleConfig);

		virtual void loadFromXmlNode( const pugi::xml_node& node );
	protected:
		friend class UITab;

		UIWidget *		mCtrlContainer;
		UIWidget *		mTabContainer;
		UITabWidgetStyleConfig	mStyleConfig;
		std::deque<UITab*>		mTabs;
		UITab *					mTabSelected;
		Uint32					mTabSelectedIndex;

		void onThemeLoaded();

		UITab * createTab( const String& Text, UINode * CtrlOwned, Drawable * Icon );

		virtual void onSizeChange();

		virtual void onChildCountChange();

		void setTabSelected( UITab * Tab );

		void setTabContainerSize();

		void setContainerSize();

		void posTabs();

		void zorderTabs();

		void orderTabs();

		void selectPrev();

		void selectNext();

		void applyThemeToTabs();

		void refreshControlOwned( UITab * tab );
};

}}

#endif
