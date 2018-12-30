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

		Uint32 getButtonMargin() const;

		void setButtonMargin( const Uint32& buttonMargin );

		Uint32 getMenuHeight() const;

		void setMenuHeight( const Uint32& menuHeight );

		Uint32 getFirstButtonMargin() const;

		void setFirstButtonMargin( const Uint32& buttonMargin );

		UIWinMenuStyleConfig getStyleConfig() const;

		void setStyleConfig(const UIWinMenuStyleConfig & styleConfig);

		virtual bool setAttribute( const NodeAttribute& attribute, const Uint32& state = UIState::StateFlagNormal );

		virtual void loadFromXmlNode( const pugi::xml_node& node );
	protected:
		typedef std::list< std::pair< UISelectButton *, UIPopUpMenu * > > WinMenuList;

		UIWinMenuStyleConfig mStyleConfig;
		UIPopUpMenu *	mCurrentMenu;
		WinMenuList		mButtons;

		void refreshButtons();

		virtual Uint32 onMessage( const NodeMessage * Msg );

		virtual void onParentChange();

		virtual void onWidgetFocusLoss();

		UIPopUpMenu * getMenuFromButton( UISelectButton * Button );

		bool isPopUpMenuChild( Node * Ctrl );

		void onMenuFocusLoss( const Event * Event );

		void unselectButtons();

		void destroyMenues();

		void autoHeight();
};

}}

#endif
