#ifndef EE_UICUIWINMENU_HPP
#define EE_UICUIWINMENU_HPP

#include "base.hpp"
#include "cuicomplexcontrol.hpp"
#include "cuiselectbutton.hpp"
#include "cuipopupmenu.hpp"

namespace EE { namespace UI {

class EE_API cUIWinMenu : public cUIComplexControl {
	public:
		class CreateParams : public cUIComplexControl::CreateParams {
			public:
				inline CreateParams() :
					cUIComplexControl::CreateParams(),
					Font( NULL ),
					FontColor( 0, 0, 0, 255 ),
					FontShadowColor( 0, 0, 0, 255 ),
					FontOverColor( 0, 0, 0, 255 ),
					FontSelectedColor( 0, 0, 0, 255 ),
					MarginBetweenButtons(0),
					ButtonMargin(4),
					MenuHeight(22),
					FirstButtonMargin(1)
				{
					cUITheme * Theme = cUIThemeManager::instance()->DefaultTheme();

					if ( NULL != Theme ) {
						Font				= Theme->Font();
						FontColor			= Theme->FontColor();
						FontShadowColor		= Theme->FontShadowColor();
						FontOverColor		= Theme->FontOverColor();
						FontSelectedColor	= FontOverColor;
					}

					if ( NULL == Font )
						Font = cUIThemeManager::instance()->DefaultFont();
				}

				cFont * 	Font;
				eeColorA 	FontColor;
				eeColorA	FontShadowColor;
				eeColorA 	FontOverColor;
				eeColorA	FontSelectedColor;
				Uint32		MarginBetweenButtons;
				Uint32		ButtonMargin;
				Uint32		MenuHeight;
				Uint32		FirstButtonMargin;
		};

		cUIWinMenu( const cUIWinMenu::CreateParams& Params );

		virtual ~cUIWinMenu();

		void AddMenuButton( const String& ButtonText, cUIPopUpMenu * Menu );

		void RemoveMenuButton( const String& ButtonText );

		virtual void SetTheme( cUITheme * Theme );

		void FontColor( const eeColorA& Color );

		const eeColorA& FontColor() const;

		void FontOverColor( const eeColorA& Color );

		const eeColorA& FontOverColor() const;

		void FontSelectedColor( const eeColorA& Color );

		const eeColorA& FontSelectedColor() const;

		cFont * Font() const;

		cUISelectButton * GetButton( const String& ButtonText );

		cUIPopUpMenu * GetPopUpMenu( const String& ButtonText );
	protected:
		typedef std::list< std::pair< cUISelectButton *, cUIPopUpMenu * > > WinMenuList;

		cFont *			mFont;
		eeColorA		mFontColor;
		eeColorA		mFontShadowColor;
		eeColorA		mFontOverColor;
		eeColorA		mFontSelectedColor;
		cUIPopUpMenu *	mCurrentMenu;
		Uint32			mMarginBetweenButtons;
		Uint32			mButtonMargin;
		Uint32			mFirstButtonMargin;
		WinMenuList		mButtons;

		void RefreshButtons();

		virtual Uint32 OnMessage( const cUIMessage * Msg );

		virtual void OnComplexControlFocusLoss();

		cUIPopUpMenu * GetMenuFromButton( cUISelectButton * Button );

		bool IsPopUpMenuChild( cUIControl * Ctrl );

		void OnMenuFocusLoss( const cUIEvent * Event );

		void UnselectButtons();
};

}}

#endif
