#ifndef EE_CUIMENUSUBMENU_HPP
#define EE_CUIMENUSUBMENU_HPP

#include <eepp/ui/uimenuitem.hpp>

namespace EE { namespace UI {

class UIMenu;

class EE_API UIMenuSubMenu : public UIMenuItem {
  public:
	static UIMenuSubMenu* New();

	UIMenuSubMenu();

	virtual ~UIMenuSubMenu();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void setTheme( UITheme* Theme );

	void setSubMenu( UIMenu* subMenu );

	UIMenu* getSubMenu() const;

	UINode* getArrow() const;

	void showSubMenu();

	const Time& getMouseOverTimeShowMenu() const;

	void setMouseOverTimeShowMenu( const Time& maxTime );

	virtual UIWidget* getExtraInnerWidget() const;

  protected:
	UIMenu* mSubMenu;
	UIWidget* mArrow;
	Time mMaxTime;
	Action* mCurWait;

	virtual Uint32 onMouseOver( const Vector2i& pos, const Uint32& flags );

	virtual Uint32 onMouseLeave( const Vector2i& pos, const Uint32& flags );

	virtual Uint32 onMouseClick( const Vector2i& pos, const Uint32& flags );

	virtual void onStateChange();

	virtual void onSizeChange();

	virtual void onAlphaChange();

	void onSubMenuFocusLoss( const Event* Event );
};

}} // namespace EE::UI

#endif
