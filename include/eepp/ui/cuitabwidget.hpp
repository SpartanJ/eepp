#ifndef EE_UICUITABWIDGET_HPP
#define EE_UICUITABWIDGET_HPP

#include <eepp/ui/cuicomplexcontrol.hpp>
#include <eepp/ui/cuitab.hpp>

namespace EE { namespace UI {

class EE_API cUITabWidget : public cUIComplexControl {
	public:
		class CreateParams : public cUIComplexControl::CreateParams {
			public:
				inline CreateParams() :
					cUIComplexControl::CreateParams(),
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
					SpecialBorderTabs( false )
				{
					Flags = ( UI_VALIGN_BOTTOM | UI_HALIGN_LEFT | UI_ANCHOR_LEFT | UI_ANCHOR_TOP );

					cUITheme * Theme = cUIThemeManager::instance()->DefaultTheme();

					if ( NULL != Theme ) {
						Font				= Theme->Font();
						FontColor			= Theme->FontColor();
						FontShadowColor		= Theme->FontShadowColor();
						FontOverColor		= Theme->FontOverColor();
						FontSelectedColor	= Theme->FontSelectedColor();
					}

					if ( NULL == Font )
						Font = cUIThemeManager::instance()->DefaultFont();
				}

				inline ~CreateParams() {}

				cFont * 	Font;
				eeColorA 	FontColor;
				eeColorA	FontShadowColor;
				eeColorA 	FontOverColor;
				eeColorA	FontSelectedColor;
				Int32		TabSeparation;
				Uint32		MaxTextLength;
				Uint32		TabWidgetHeight;
				Uint32		TabTextAlign;
				Uint32		MinTabWidth;
				Uint32		MaxTabWidth;
				bool		TabsClosable;
				bool		SpecialBorderTabs; //! Indicates if the periferical tabs ( the left and right border tab ) are different from the central tabs.
		};

		cUITabWidget( cUITabWidget::CreateParams& Params );

		virtual ~cUITabWidget();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		Uint32 Add( const String& Text, cUIControl * CtrlOwned, cShape * Icon = NULL );

		Uint32 Add( cUITab * Tab );

		cUITab * GetTab( const Uint32& Index );

		cUITab * GetTab( const String& Text );

		Uint32 GetTabIndex( cUITab * Tab );

		Uint32 Count() const;

		void Remove( const Uint32& Index );

		void Remove( cUITab * Tab );

		void RemoveAll();

		void Insert( const String& Text, cUIControl * CtrlOwned, cShape * Icon, const Uint32& Index );

		void Insert( cUITab * Tab, const Uint32& Index );

		virtual void SetTheme( cUITheme * Theme );

		cUITab * GetSelectedTab() const;

		Uint32 GetSelectedTabIndex() const;

		cUIComplexControl * TabContainer() const;

		cUIComplexControl * ControlContainer() const;
	protected:
		friend class cUITab;

		cUIComplexControl *		mCtrlContainer;
		cUIComplexControl *		mTabContainer;
		cFont *					mFont;
		eeColorA				mFontColor;
		eeColorA				mFontShadowColor;
		eeColorA				mFontOverColor;
		eeColorA				mFontSelectedColor;
		Int32					mTabSeparation;
		Uint32					mMaxTextLength;
		Uint32					mTabWidgetHeight;
		Uint32					mMinTabWidth;
		Uint32					mMaxTabWidth;
		bool					mTabsClosable;
		bool					mSpecialBorderTabs;

		std::deque<cUITab*>		mTabs;
		cUITab *				mTabSelected;
		Uint32					mTabSelectedIndex;

		void DoAfterSetTheme();

		cUITab * CreateTab( const String& Text, cUIControl * CtrlOwned, cShape * Icon );

		virtual void OnSizeChange();

		void SetTabSelected( cUITab * Tab );

		void SetTabContainerSize();

		void SetContainerSize();

		void PosTabs();

		void ZOrderTabs();

		void OrderTabs();

		void SelectPrev();

		void SelectNext();

		void ApplyThemeToTabs();
};

}}

#endif
