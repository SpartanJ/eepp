#ifndef EE_UICUIWINMENU_HPP
#define EE_UICUIWINMENU_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/ui/uiselectbutton.hpp>
#include <eepp/ui/uipopupmenu.hpp>

namespace EE { namespace UI {

class EE_API UIWinMenu : public UIWidget {
	public:
		static UIWinMenu * New();

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

		UIWinMenuStyleConfig getStyleConfig() const;

		void setStyleConfig(const UIWinMenuStyleConfig & styleConfig);

		virtual void loadFromXmlNode( const pugi::xml_node& node );
	protected:
		typedef std::list< std::pair< UISelectButton *, UIPopUpMenu * > > WinMenuList;

		UIWinMenuStyleConfig mStyleConfig;
		UIPopUpMenu *	mCurrentMenu;
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
