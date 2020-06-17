#ifndef EE_UI_UIMENUBAR_HPP
#define EE_UI_UIMENUBAR_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uiselectbutton.hpp>
#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

class EE_API UIMenuBar : public UIWidget {
  public:
	static UIMenuBar* New();

	UIMenuBar();

	virtual ~UIMenuBar();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	void addMenuButton( const String& ButtonText, UIPopUpMenu* Menu );

	void removeMenuButton( const String& ButtonText );

	virtual void setTheme( UITheme* Theme );

	UISelectButton* getButton( const String& ButtonText );

	UIPopUpMenu* getPopUpMenu( const String& ButtonText );

	Uint32 getMenuHeight() const;

	void setMenuHeight( const Uint32& menuHeight );

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual void loadFromXmlNode( const pugi::xml_node& node );

  protected:
	typedef std::list<std::pair<UISelectButton*, UIPopUpMenu*>> MenuBarList;

	Uint32 mMenuHeight;
	UIPopUpMenu* mCurrentMenu;
	MenuBarList mButtons;
	UIPopUpMenu* mWaitingUp;

	void refreshButtons();

	virtual Uint32 onMessage( const NodeMessage* Msg );

	virtual void onParentChange();

	virtual void onPaddingChange();

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
