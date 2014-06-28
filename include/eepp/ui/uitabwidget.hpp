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

					UITheme * Theme = UIThemeManager::instance()->DefaultTheme();

					if ( NULL != Theme ) {
						Font				= Theme->Font();
						FontColor			= Theme->FontColor();
						FontShadowColor		= Theme->FontShadowColor();
						FontOverColor		= Theme->FontOverColor();
						FontSelectedColor	= Theme->FontSelectedColor();
					}

					if ( NULL == Font )
						Font = UIThemeManager::instance()->DefaultFont();
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

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		Uint32 Add( const String& Text, UIControl * CtrlOwned, SubTexture * Icon = NULL );

		Uint32 Add( UITab * Tab );

		UITab * GetTab( const Uint32& Index );

		UITab * GetTab( const String& Text );

		Uint32 GetTabIndex( UITab * Tab );

		Uint32 Count() const;

		void Remove( const Uint32& Index );

		void Remove( UITab * Tab );

		void RemoveAll();

		void Insert( const String& Text, UIControl * CtrlOwned, SubTexture * Icon, const Uint32& Index );

		void Insert( UITab * Tab, const Uint32& Index );

		virtual void SetTheme( UITheme * Theme );

		UITab * GetSelectedTab() const;

		Uint32 GetSelectedTabIndex() const;

		UIComplexControl * TabContainer() const;

		UIComplexControl * ControlContainer() const;

		virtual void Draw();
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

		void DoAfterSetTheme();

		UITab * CreateTab( const String& Text, UIControl * CtrlOwned, SubTexture * Icon );

		virtual void OnSizeChange();

		void SetTabSelected( UITab * Tab );

		void SetTabContainerSize();

		void SeContainerSize();

		void PosTabs();

		void ZOrderTabs();

		void OrderTabs();

		void SelectPrev();

		void SelectNext();

		void ApplyThemeToTabs();
};

}}

#endif
