#ifndef EE_UICUITABWIDGET_HPP
#define EE_UICUITABWIDGET_HPP

#include <eepp/ui/uiwidget.hpp>
#include <eepp/ui/uitab.hpp>
#include <deque>

namespace EE { namespace UI {

class EE_API UITabWidget : public UIWidget {
	public:
		class StyleConfig {
			public:
				Int32		TabSeparation = 0;
				Uint32		MaxTextLength = 30;
				Uint32		TabWidgetHeight = 0;
				Uint32		TabTextAlign = ( UI_HALIGN_CENTER | UI_VALIGN_CENTER );
				Uint32		MinTabWidth = 32;
				Uint32		MaxTabWidth = 300;
				bool		TabsClosable = false;
				bool		SpecialBorderTabs = false; //! Indicates if the periferical tabs ( the left and right border tab ) are different from the central tabs.
				bool		DrawLineBelowTabs = false;
				Color		LineBelowTabsColor;
				Int32		LineBelowTabsYOffset = 0;
		};

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

		const StyleConfig & getStyleConfig() const;

		void setStyleConfig(const StyleConfig & styleConfig);

		virtual bool applyProperty( const StyleSheetProperty& attribute );

		virtual std::string getPropertyString(const PropertyDefinition* propertyDef);
	protected:
		friend class UITab;

		UIWidget *		mCtrlContainer;
		UIWidget *		mTabContainer;
		StyleConfig	mStyleConfig;
		std::deque<UITab*>		mTabs;
		UITab *					mTabSelected;
		Uint32					mTabSelectedIndex;

		void onThemeLoaded();

		UITab * createTab( const String& Text, UINode * CtrlOwned, Drawable * Icon );

		virtual void onSizeChange();

		virtual void onChildCountChange();

		virtual void onPaddingChange();

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
