#ifndef EE_UICUITABWIDGET_HPP
#define EE_UICUITABWIDGET_HPP

#include <eepp/ui/uicomplexcontrol.hpp>
#include <eepp/ui/uitab.hpp>
#include <deque>

namespace EE { namespace UI {

class EE_API UITabWidget : public UIComplexControl {
	public:
		class CreateParams : public UIComplexControl::CreateParams {
			public:
				inline CreateParams() :
					UIComplexControl::CreateParams(),
					Font( NULL ),
					FontColor( 0, 0, 0, 255 ),
					FontOverColor( 0, 0, 0, 255 ),
					FontSelectedColor( 0, 0, 0, 255 ),
					TabSeparation( 0 ),
					MaxTextLength( 30 ),
					TabWidgetHeight( 0 ),
					TabTextAlign( UI_HALIGN_CENTER | UI_VALIGN_CENTER ),
					MinTabWidth( 32 ),
					MaxTabWidth( 210 ),
					TabsClosable( false ),
					SpecialBorderTabs( false ),
					DrawLineBelowTabs( false ),
					LineBewowTabsYOffset( 0 )
				{
					Flags = ( UI_VALIGN_BOTTOM | UI_HALIGN_LEFT | UI_ANCHOR_LEFT | UI_ANCHOR_TOP );

					UITheme * Theme = UIThemeManager::instance()->getDefaultTheme();

					if ( NULL != Theme ) {
						Font				= Theme->getFont();
						FontColor			= Theme->getFontColor();
						FontShadowColor		= Theme->getFontShadowColor();
						FontOverColor		= Theme->getFontOverColor();
						FontSelectedColor	= Theme->getFontSelectedColor();
					}

					if ( NULL == Font )
						Font = UIThemeManager::instance()->getDefaultFont();
				}

				inline ~CreateParams() {}

				Graphics::Font * 	Font;
				ColorA				FontColor;
				ColorA				FontShadowColor;
				ColorA				FontOverColor;
				ColorA				FontSelectedColor;
				Int32		TabSeparation;
				Uint32		MaxTextLength;
				Uint32		TabWidgetHeight;
				Uint32		TabTextAlign;
				Uint32		MinTabWidth;
				Uint32		MaxTabWidth;
				bool		TabsClosable;
				bool		SpecialBorderTabs; //! Indicates if the periferical tabs ( the left and right border tab ) are different from the central tabs.
				bool		DrawLineBelowTabs;
				ColorA		LineBelowTabsColor;
				Int32		LineBewowTabsYOffset;

		};

		UITabWidget( UITabWidget::CreateParams& Params );

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

		UIComplexControl * getTabContainer() const;

		UIComplexControl * getControlContainer() const;

		virtual void draw();
	protected:
		friend class UITab;

		UIComplexControl *		mCtrlContainer;
		UIComplexControl *		mTabContainer;
		Font *					mFont;
		ColorA				mFontColor;
		ColorA				mFontShadowColor;
		ColorA				mFontOverColor;
		ColorA				mFontSelectedColor;
		Int32					mTabSeparation;
		Uint32					mMaxTextLength;
		Uint32					mTabWidgetHeight;
		Uint32					mMinTabWidth;
		Uint32					mMaxTabWidth;
		bool					mTabsClosable;
		bool					mSpecialBorderTabs;
		bool					mDrawLineBelowTabs;
		ColorA				mLineBelowTabsColor;
		Int32					mLineBewowTabsYOffset;

		std::deque<UITab*>		mTabs;
		UITab *				mTabSelected;
		Uint32					mTabSelectedIndex;

		void doAftersetTheme();

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
