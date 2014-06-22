#ifndef EE_UICUIMENU_HPP
#define EE_UICUIMENU_HPP

#include <eepp/ui/cuicontrolanim.hpp>
#include <eepp/ui/cuimenuitem.hpp>
#include <eepp/ui/cuimenucheckbox.hpp>
#include <eepp/ui/cuimenusubmenu.hpp>
#include <eepp/ui/cuiseparator.hpp>
#include <deque>

namespace EE { namespace UI {

class EE_API cUIMenu : public cUIComplexControl {
	public:
		class CreateParams : public cUIComplexControl::CreateParams {
			public:
				inline CreateParams() :
					cUIComplexControl::CreateParams(),
					RowHeight( 0 ),
					PaddingContainer(),
					MinWidth( 0 ),
					MinSpaceForIcons( 0 ),
					MinRightMargin( 0 ),
					Font( NULL ),
					FontColor( 0, 0, 0, 255 ),
					FontOverColor( 0, 0, 0, 255 ),
					FontSelectedColor( 0, 0, 0, 255 )
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

				Uint32				RowHeight;
				Recti				PaddingContainer;
				Uint32				MinWidth;
				Uint32				MinSpaceForIcons;
				Uint32				MinRightMargin;
				Graphics::Font * 	Font;
				ColorA				FontColor;
				ColorA				FontShadowColor;
				ColorA				FontOverColor;
				ColorA				FontSelectedColor;

		};

		static void FixMenuPos( Vector2i& Pos, cUIMenu * Menu, cUIMenu * Parent = NULL, cUIMenuSubMenu * SubMenu = NULL );

		cUIMenu( cUIMenu::CreateParams& Params );

		virtual ~cUIMenu();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		Uint32 Add( const String& Text, SubTexture * Icon = NULL );

		Uint32 Add( cUIControl * Control );

		Uint32 AddSeparator();

		Uint32 AddCheckBox( const String& Text, const bool& Active = false );

		Uint32 AddSubMenu( const String& Text, SubTexture * Icon = NULL, cUIMenu * SubMenu = NULL );

		cUIControl * GetItem( const Uint32& Index );
		
		cUIControl * GetItem( const String& Text );

		Uint32 GetItemIndex( cUIControl * Item );

		Uint32 Count() const;

		void Remove( const Uint32& Index );

		void Remove( cUIControl * Ctrl );

		void RemoveAll();

		void Insert( const String& Text, SubTexture * Icon, const Uint32& Index );

		void Insert( cUIControl * Control, const Uint32& Index );

		virtual void SetTheme( cUITheme * Theme );

		virtual bool Show();

		virtual bool Hide();

		const Recti& Padding() const;
	protected:
		friend class cUIMenuItem;
		friend class cUIMenuCheckBox;
		friend class cUIMenuSubMenu;

		std::deque<cUIControl *> mItems;
		Recti				mPadding;
		Font * 			mFont;
		ColorA 			mFontColor;
		ColorA			mFontShadowColor;
		ColorA 			mFontOverColor;
		ColorA			mFontSelectedColor;
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
		
		cUIMenuItem * CreateMenuItem( const String& Text, SubTexture * Icon );

		cUIMenuCheckBox * CreateMenuCheckBox( const String& Text, const bool& Active );

		cUIMenuSubMenu * CreateSubMenu( const String& Text, SubTexture * Icon, cUIMenu * SubMenu );
		
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
