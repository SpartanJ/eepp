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

	virtual ~UIMenuBar();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	void addMenuButton( const String& buttonText, UIPopUpMenu* Menu );

	void removeMenuButton( const String& buttonText );

	virtual void setTheme( UITheme* theme );

	UISelectButton* getButton( const String& buttonText ) const;

	UISelectButton* getButton( const Uint32& index ) const;

	UIPopUpMenu* getPopUpMenu( const String& buttonText ) const;

	UIPopUpMenu* getPopUpMenu( const Uint32& index ) const;

	UIMenuBar* setPopUpMenu( const Uint32& index, UIPopUpMenu* menu );

	size_t getButtonsCount() const;

	UIPopUpMenu* getMenuFromButton( UISelectButton* Button );

	Uint32 getMenuHeight() const;

	void setMenuHeight( const Uint32& menuHeight );

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual void loadFromXmlNode( const pugi::xml_node& node );

	UIPopUpMenu* getCurrentMenu() const;

	void setCurrentMenu( UIPopUpMenu* currentMenu );

	void showMenu( const Uint32& index );

	void showNextMenu();

	void showPrevMenu();

  protected:
	UIMenuBar();

	typedef std::vector<std::pair<UISelectButton*, UIPopUpMenu*>> MenuBarList;

	Uint32 mMenuHeight;
	UIPopUpMenu* mCurrentMenu;
	MenuBarList mButtons;
	UIPopUpMenu* mWaitingUp;

	Uint32 getMenuIndex( UIPopUpMenu* menu );

	void refreshButtons();

	virtual Uint32 onMessage( const NodeMessage* Msg );

	virtual void onParentChange();

	virtual void onPaddingChange();

	bool isPopUpMenuChild( Node* node );

	void unselectButtons();

	void destroyMenues();

	void autoHeight();
};

}} // namespace EE::UI

#endif
