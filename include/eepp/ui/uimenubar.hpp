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

	void addMenuButton( const String& buttonText, UIPopUpMenu* Menu );

	void removeMenuButton( const String& buttonText );

	virtual void setTheme( UITheme* theme );

	UISelectButton* getButton( const String& buttonText );

	UIPopUpMenu* getPopUpMenu( const String& buttonText );

	Uint32 getMenuHeight() const;

	void setMenuHeight( const Uint32& menuHeight );

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual void loadFromXmlNode( const pugi::xml_node& node );

  protected:
	typedef std::vector<std::pair<UISelectButton*, UIPopUpMenu*>> MenuBarList;

	Uint32 mMenuHeight;
	UIPopUpMenu* mCurrentMenu;
	MenuBarList mButtons;
	UIPopUpMenu* mWaitingUp;

	void refreshButtons();

	virtual Uint32 onMessage( const NodeMessage* Msg );

	virtual void onParentChange();

	virtual void onPaddingChange();

	UIPopUpMenu* getMenuFromButton( UISelectButton* Button );

	bool isPopUpMenuChild( Node* node );

	void unselectButtons();

	void destroyMenues();

	void autoHeight();
};

}} // namespace EE::UI

#endif
