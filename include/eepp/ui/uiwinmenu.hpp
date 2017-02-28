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
					marginBetweenButtons(0),
					buttonMargin(4),
					menuHeight(0),
					firstButtonMargin(1)
				{
					fontStyleConfig = UIThemeManager::instance()->getDefaultFontStyleConfig();
				}

				FontStyleConfig		fontStyleConfig;
				Uint32				marginBetweenButtons;
				Uint32				buttonMargin;
				Uint32				menuHeight;
				Uint32				firstButtonMargin;
		};

		UIWinMenu( const UIWinMenu::CreateParams& Params );

		UIWinMenu();

		virtual ~UIWinMenu();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		void addMenuButton( const String& ButtonText, UIPopUpMenu * Menu );

		void removeMenuButton( const String& ButtonText );

		virtual void setTheme( UITheme * Theme );

		UISelectButton * getButton( const String& ButtonText );

		UIPopUpMenu * getPopUpMenu( const String& ButtonText );

		Uint32 getMarginBetweenButtons() const;

		void setMarginBetweenButtons(const Uint32 & marginBetweenButtons);

		FontStyleConfig getFontStyleConfig() const;

		void setFontStyleConfig(const FontStyleConfig & fontStyleConfig);
	protected:
		typedef std::list< std::pair< UISelectButton *, UIPopUpMenu * > > WinMenuList;

		FontStyleConfig mFontStyleConfig;
		UIPopUpMenu *	mCurrentMenu;
		Uint32			mMarginBetweenButtons;
		Uint32			mButtonMargin;
		Uint32			mFirstButtonMargin;
		Uint32			mMenuHeight;
		WinMenuList		mButtons;

		void refreshButtons();

		virtual Uint32 onMessage( const UIMessage * Msg );

		virtual void onParentChange();

		virtual void onComplexControlFocusLoss();

		UIPopUpMenu * getMenuFromButton( UISelectButton * Button );

		bool isPopUpMenuChild( UIControl * Ctrl );

		void onMenuFocusLoss( const UIEvent * Event );

		void unselectButtons();

		void destroyMenues();
};

}}

#endif
