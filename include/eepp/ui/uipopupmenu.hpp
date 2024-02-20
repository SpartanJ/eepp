#ifndef EE_UICUIPOPUPMENU
#define EE_UICUIPOPUPMENU

#include <eepp/ui/uimenu.hpp>

namespace EE { namespace UI {

class EE_API UIPopUpMenu : public UIMenu {
  public:
	static UIPopUpMenu* New();

	UIPopUpMenu();

	virtual ~UIPopUpMenu();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void setTheme( UITheme* Theme );

	virtual bool show();

	virtual bool hide();

	virtual void close();

	bool isHiding() const;

	bool closeOnHide() const;

	void setCloseOnHide( bool closeOnHide );

	bool closeSubMenusOnClose() const;

	void setCloseSubMenusOnClose( bool closeSubMenusOnClose );

	void showAtScreenPosition( Vector2f pos );

	void showOverMouseCursor();

  protected:
	Action* mHidingAction{ nullptr };
	bool mCloseOnHide{ false };
	bool mCloseSubMenusOnClose{ false };
};

class EE_API ContextMenuEvent : public MouseEvent {
  public:
	ContextMenuEvent( Node* node, UIPopUpMenu* menu, const Uint32& eventType, const Vector2i& pos,
					  const Uint32& flags ) :
		MouseEvent( node, eventType, pos, flags ), menu( menu ) {}

	UIPopUpMenu* getMenu() const { return menu; }

  protected:
	UIPopUpMenu* menu;
};

}} // namespace EE::UI

#endif
