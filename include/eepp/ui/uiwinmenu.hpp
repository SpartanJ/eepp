#ifndef EE_UICUIWINMENU_HPP
#define EE_UICUIWINMENU_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/uicomplexcontrol.hpp>
#include <eepp/ui/uiselectbutton.hpp>
#include <eepp/ui/uipopupmenu.hpp>

namespace EE { namespace UI {

class EE_API UIWinMenu : public UIComplexControl {
	public:
		class CreateParams : public UIComplexControl::CreateParams {
			public:
				inline CreateParams() :
					UIComplexControl::CreateParams(),
					Font( NULL ),
					FontColor( 0, 0, 0, 255 ),
					FontShadowColor( 0, 0, 0, 255 ),
					FontOverColor( 0, 0, 0, 255 ),
					FontSelectedColor( 0, 0, 0, 255 ),
					MarginBetweenButtons(0),
					ButtonMargin(4),
					MenuHeight(0),
					FirstButtonMargin(1)
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

				Graphics::Font * 	Font;
				ColorA				FontColor;
				ColorA				FontShadowColor;
				ColorA				FontOverColor;
				ColorA				FontSelectedColor;
				Uint32				MarginBetweenButtons;
				Uint32				ButtonMargin;
				Uint32				MenuHeight;
				Uint32				FirstButtonMargin;
		};

		UIWinMenu( const UIWinMenu::CreateParams& Params );

		virtual ~UIWinMenu();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		void AddMenuButton( const String& ButtonText, UIPopUpMenu * Menu );

		void RemoveMenuButton( const String& ButtonText );

		virtual void SetTheme( UITheme * Theme );

		void FontColor( const ColorA& Color );

		const ColorA& FontColor() const;

		void FontOverColor( const ColorA& Color );

		const ColorA& FontOverColor() const;

		void FontSelectedColor( const ColorA& Color );

		const ColorA& FontSelectedColor() const;

		Graphics::Font * Font() const;

		UISelectButton * GetButton( const String& ButtonText );

		UIPopUpMenu * GetPopUpMenu( const String& ButtonText );
	protected:
		typedef std::list< std::pair< UISelectButton *, UIPopUpMenu * > > WinMenuList;

		Graphics::Font *mFont;
		ColorA			mFontColor;
		ColorA			mFontShadowColor;
		ColorA			mFontOverColor;
		ColorA			mFontSelectedColor;
		UIPopUpMenu *	mCurrentMenu;
		Uint32			mMarginBetweenButtons;
		Uint32			mButtonMargin;
		Uint32			mFirstButtonMargin;
		Uint32			mMenuHeight;
		WinMenuList		mButtons;

		void RefreshButtons();

		virtual Uint32 OnMessage( const UIMessage * Msg );

		virtual void OnComplexControlFocusLoss();

		UIPopUpMenu * GetMenuFromButton( UISelectButton * Button );

		bool IsPopUpMenuChild( UIControl * Ctrl );

		void OnMenuFocusLoss( const UIEvent * Event );

		void UnselectButtons();

		void DestroyMenues();
};

}}

#endif
