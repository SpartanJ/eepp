#ifndef EE_UICUIMENU_HPP
#define EE_UICUIMENU_HPP

#include <eepp/ui/uicontrolanim.hpp>
#include <eepp/ui/uimenuitem.hpp>
#include <eepp/ui/uimenucheckbox.hpp>
#include <eepp/ui/uimenusubmenu.hpp>
#include <eepp/ui/uiseparator.hpp>
#include <deque>

namespace EE { namespace UI {

class EE_API UIMenu : public UIComplexControl {
	public:
		class CreateParams : public UIComplexControl::CreateParams {
			public:
				inline CreateParams() :
					UIComplexControl::CreateParams(),
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

		static void FixMenuPos( Vector2i& Pos, UIMenu * Menu, UIMenu * Parent = NULL, UIMenuSubMenu * SubMenu = NULL );

		UIMenu( UIMenu::CreateParams& Params );

		virtual ~UIMenu();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		Uint32 Add( const String& Text, SubTexture * Icon = NULL );

		Uint32 Add( UIControl * Control );

		Uint32 AddSeparator();

		Uint32 AddCheckBox( const String& Text, const bool& Active = false );

		Uint32 AddSubMenu( const String& Text, SubTexture * Icon = NULL, UIMenu * SubMenu = NULL );

		UIControl * GetItem( const Uint32& Index );
		
		UIControl * GetItem( const String& Text );

		Uint32 GetItemIndex( UIControl * Item );

		Uint32 Count() const;

		void Remove( const Uint32& Index );

		void Remove( UIControl * Ctrl );

		void RemoveAll();

		void Insert( const String& Text, SubTexture * Icon, const Uint32& Index );

		void Insert( UIControl * Control, const Uint32& Index );

		virtual void SetTheme( UITheme * Theme );

		virtual bool Show();

		virtual bool Hide();

		const Recti& Padding() const;
	protected:
		friend class UIMenuItem;
		friend class UIMenuCheckBox;
		friend class UIMenuSubMenu;

		std::deque<UIControl *> mItems;
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
		UIControl *		mItemSelected;
		Uint32				mItemSelectedIndex;
		bool				mClickHide;
		Uint32				mLastTickMove;

		virtual void OnSizeChange();

		void AutoPadding();

		virtual Uint32 OnMessage( const UIMessage * Msg );

		void SetControlSize( UIControl * Control, const Uint32& Pos );
		
		void ResizeControls();
		
		void ReposControls();
		
		void ResizeMe();
		
		UIMenuItem * CreateMenuItem( const String& Text, SubTexture * Icon );

		UIMenuCheckBox * CreateMenuCheckBox( const String& Text, const bool& Active );

		UIMenuSubMenu * CreateSubMenu( const String& Text, SubTexture * Icon, UIMenu * SubMenu );
		
		void DoAfterSetTheme();

		bool CheckControlSize( UIControl * Control, const bool& Resize = true );

		bool IsSubMenu( UIControl * Ctrl );

		void SetItemSelected( UIControl * Item );

		virtual Uint32 OnKeyDown( const UIEventKey& Event );

		void PrevSel();

		void NextSel();

		void TrySelect( UIControl * Ctrl, bool Up );
};

}}

#endif
