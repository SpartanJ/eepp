#ifndef EE_UICUIMENU_HPP
#define EE_UICUIMENU_HPP

#include "cuicontrolanim.hpp"
#include "cuimenuitem.hpp"
#include "cuimenucheckbox.hpp"
#include "cuimenusubmenu.hpp"
#include "cuiseparator.hpp"

namespace EE { namespace UI {

class EE_API cUIMenu : public cUIComplexControl {
	public:
		class CreateParams : public cUIComplexControl::CreateParams {
			public:
				inline CreateParams() :
					cUIComplexControl::CreateParams(),
					RowHeight( 0 ),
					PaddingContainer(),
					Font( NULL ),
					FontColor( 0, 0, 0, 255 ),
					FontOverColor( 0, 0, 0, 255 ),
					FontSelectedColor( 0, 0, 0, 255 ),
					MinWidth( 0 ),
					MinSpaceForIcons( 0 ),
					MinRightMargin( 0 )
				{
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

				Uint32		RowHeight;
				eeRecti		PaddingContainer;
				cFont * 	Font;
				eeColorA 	FontColor;
				eeColorA	FontShadowColor;
				eeColorA 	FontOverColor;
				eeColorA	FontSelectedColor;
				Uint32		MinWidth;
				Uint32		MinSpaceForIcons;
				Uint32		MinRightMargin;

		};

		static void FixMenuPos( eeVector2i& Pos, cUIMenu * Menu, cUIMenu * Parent = NULL, cUIMenuSubMenu * SubMenu = NULL );

		cUIMenu( cUIMenu::CreateParams& Params );

		~cUIMenu();

		Uint32 Add( const std::wstring& Text, cShape * Icon = NULL );

		Uint32 Add( cUIControl * Control );

		Uint32 AddSeparator();

		Uint32 AddCheckBox( const std::wstring& Text );

		Uint32 AddSubMenu( const std::wstring& Text, cShape * Icon = NULL, cUIMenu * SubMenu = NULL );

		cUIControl * GetItem( const Uint32& Index );
		
		cUIControl * GetItem( const std::wstring& Text );

		Uint32 GetItemIndex( cUIControl * Item );

		Uint32 Count() const;

		void Remove( const Uint32& Index );

		void Remove( cUIControl * Ctrl );

		void RemoveAll();

		void Insert( const std::wstring& Text, cShape * Icon, const Uint32& Index );

		void Insert( cUIControl * Control, const Uint32& Index );

		virtual void SetTheme( cUITheme * Theme );

		virtual bool Show();

		virtual bool Hide();

		const eeRecti& Padding() const;
	protected:
		friend class cUIMenuItem;
		friend class cUIMenuCheckBox;
		friend class cUIMenuSubMenu;

		std::deque<cUIControl *> mItems;
		eeRecti				mPadding;
		cFont * 			mFont;
		eeColorA 			mFontColor;
		eeColorA			mFontShadowColor;
		eeColorA 			mFontOverColor;
		eeColorA			mFontSelectedColor;
		Uint32				mMinWidth;
		Uint32				mMinSpaceForIcons;
		Uint32				mMinRightMargin;
		Uint32				mMaxWidth;
		Uint32				mRowHeight;
		Uint32				mNextPosY;
		Uint32				mBiggestIcon;
		cUIControl *		mItemSelected;
		Uint32				mItemSelectedIndex;
		bool				mClickHide;
		Uint32				mLastTickMove;

		virtual void OnSizeChange();

		void AutoPadding();

		virtual Uint32 OnMessage( const cUIMessage * Msg );

		void SetControlSize( cUIControl * Control, const Uint32& Pos );
		
		void ResizeControls();
		
		void ReposControls();
		
		void ResizeMe();
		
		cUIMenuItem * CreateMenuItem( const std::wstring& Text, cShape * Icon );

		cUIMenuCheckBox * CreateMenuCheckBox( const std::wstring& Text );

		cUIMenuSubMenu * CreateSubMenu( const std::wstring& Text, cShape * Icon, cUIMenu * SubMenu );
		
		void DoAfterSetTheme();

		bool CheckControlSize( cUIControl * Control, const bool& Resize = true );

		bool IsSubMenu( cUIControl * Ctrl );

		void SetItemSelected( cUIControl * Item );

		virtual Uint32 OnKeyDown( const cUIEventKey& Event );

		void PrevSel();

		void NextSel();

		void TrySelect( cUIControl * Ctrl, bool Up );
};

}}

#endif
