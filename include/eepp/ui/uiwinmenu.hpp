#ifndef EE_UICUIWINMENU_HPP
#define EE_UICUIWINMENU_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uiselectbutton.hpp>
#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

class EE_API UIWinMenu : public UIWidget {
  public:
	class StyleConfig {
	  public:
		Uint32 MarginBetweenButtons = 0;
		Uint32 ButtonMargin = 4;
		Uint32 MenuHeight = 0;
		Uint32 FirstButtonMargin = 1;
	};

	static UIWinMenu* New();

	UIWinMenu();

	virtual ~UIWinMenu();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	void addMenuButton( const String& ButtonText, UIPopUpMenu* Menu );

	void removeMenuButton( const String& ButtonText );

	virtual void setTheme( UITheme* Theme );

	UISelectButton* getButton( const String& ButtonText );

	UIPopUpMenu* getPopUpMenu( const String& ButtonText );

	Uint32 getMarginBetweenButtons() const;

	void setMarginBetweenButtons( const Uint32& marginBetweenButtons );

	Uint32 getButtonMargin() const;

	void setButtonMargin( const Uint32& buttonMargin );

	Uint32 getMenuHeight() const;

	void setMenuHeight( const Uint32& menuHeight );

	Uint32 getFirstButtonMargin() const;

	void setFirstButtonMargin( const Uint32& buttonMargin );

	const StyleConfig& getStyleConfig() const;

	void setStyleConfig( const StyleConfig& styleConfig );

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual void loadFromXmlNode( const pugi::xml_node& node );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 );

  protected:
	typedef std::list<std::pair<UISelectButton*, UIPopUpMenu*>> WinMenuList;

	StyleConfig mStyleConfig;
	UIPopUpMenu* mCurrentMenu;
	WinMenuList mButtons;

	void refreshButtons();

	virtual Uint32 onMessage( const NodeMessage* Msg );

	virtual void onParentChange();

	virtual void onWidgetFocusLoss();

	UIPopUpMenu* getMenuFromButton( UISelectButton* Button );

	bool isPopUpMenuChild( Node* Ctrl );

	void onMenuFocusLoss( const Event* Event );

	void onHideByClick( const Event* Event );

	void unselectButtons();

	void destroyMenues();

	void autoHeight();
};

}} // namespace EE::UI

#endif
