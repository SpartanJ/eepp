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
					UITheme * Theme = UIThemeManager::instance()->defaultTheme();

					if ( NULL != Theme ) {
						Font				= Theme->font();
						FontColor			= Theme->fontColor();
						FontShadowColor		= Theme->fontShadowColor();
						FontOverColor		= Theme->fontOverColor();
						FontSelectedColor	= Theme->fontSelectedColor();
					}

					if ( NULL == Font )
						Font = UIThemeManager::instance()->defaultFont();
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

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		void addMenuButton( const String& ButtonText, UIPopUpMenu * Menu );

		void removeMenuButton( const String& ButtonText );

		virtual void setTheme( UITheme * Theme );

		void fontColor( const ColorA& Color );

		const ColorA& fontColor() const;

		void fontOverColor( const ColorA& Color );

		const ColorA& fontOverColor() const;

		void fontSelectedColor( const ColorA& Color );

		const ColorA& fontSelectedColor() const;

		Graphics::Font * font() const;

		UISelectButton * getButton( const String& ButtonText );

		UIPopUpMenu * getPopUpMenu( const String& ButtonText );
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

		void refreshButtons();

		virtual Uint32 onMessage( const UIMessage * Msg );

		virtual void onComplexControlFocusLoss();

		UIPopUpMenu * getMenuFromButton( UISelectButton * Button );

		bool isPopUpMenuChild( UIControl * Ctrl );

		void onMenuFocusLoss( const UIEvent * Event );

		void unselectButtons();

		void destroyMenues();
};

}}

#endif
