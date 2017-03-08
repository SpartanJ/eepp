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

		Uint32 add( const String& Text, UIControl * CtrlOwned, SubTexture * Icon = NULL );

		Uint32 add( UITab * Tab );

		UITab * getTab( const Uint32& Index );

		UITab * getTab( const String& Text );

		Uint32 getTabIndex( UITab * Tab );

		Uint32 getCount() const;

		void remove( const Uint32& Index );

		void remove( UITab * Tab );

		void removeAll();

		void insert( const String& Text, UIControl * CtrlOwned, SubTexture * Icon, const Uint32& Index );

		void insert( UITab * Tab, const Uint32& Index );

		virtual void setTheme( UITheme * Theme );

		UITab * getSelectedTab() const;

		Uint32 getSelectedTabIndex() const;

		UIWidget * getTabContainer() const;

		UIWidget * getControlContainer() const;

		virtual void draw();

		Font * getFont() const;

		void setFont(Font * font);

		ColorA getFontColor() const;

		void setFontColor(const ColorA & fontColor);

		ColorA getFontShadowColor() const;

		void setFontShadowColor(const ColorA & fontShadowColor);

		ColorA getFontOverColor() const;

		void setFontOverColor(const ColorA & fontOverColor);

		ColorA getFontSelectedColor() const;

		void setFontSelectedColor(const ColorA & fontSelectedColor);

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

		ColorA getLineBelowTabsColor() const;

		void setLineBelowTabsColor(const ColorA & lineBelowTabsColor);

		Int32 getLineBelowTabsYOffset() const;

		void setLineBelowTabsYOffset(const Int32 & lineBelowTabsYOffset);

		TooltipStyleConfig getFontStyleConfig() const;

		void setFontStyleConfig(const TooltipStyleConfig & fontStyleConfig);

		TabWidgetStyleConfig getStyleConfig() const;

		void setStyleConfig(const TabWidgetStyleConfig & styleConfig);
	protected:
		friend class UITab;

		UIWidget *		mCtrlContainer;
		UIWidget *		mTabContainer;
		TabWidgetStyleConfig	mStyleConfig;
		std::deque<UITab*>		mTabs;
		UITab *					mTabSelected;
		Uint32					mTabSelectedIndex;

		void onThemeLoaded();

		UITab * createTab( const String& Text, UIControl * CtrlOwned, SubTexture * Icon );

		virtual void onSizeChange();

		void setTabSelected( UITab * Tab );

		void setTabContainerSize();

		void seContainerSize();

		void posTabs();

		void zorderTabs();

		void orderTabs();

		void selectPrev();

		void selectNext();

		void applyThemeToTabs();
};

}}

#endif
