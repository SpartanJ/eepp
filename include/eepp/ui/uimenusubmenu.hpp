#ifndef EE_CUIMENUSUBMENU_HPP
#define EE_CUIMENUSUBMENU_HPP

#include <eepp/ui/uimenuitem.hpp>

namespace EE { namespace UI {

class UIMenu;

class EE_API UIMenuSubMenu : public UIMenuItem {
	public:
		static UIMenuSubMenu * New();

		UIMenuSubMenu();

		virtual ~UIMenuSubMenu();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void setTheme( UITheme * Theme );

		void setSubMenu( UIMenu * subMenu );

		UIMenu * getSubMenu() const;

		UIGfx * getArrow() const;

		void showSubMenu();

		virtual bool inheritsFrom( const Uint32 getType );

		Float getMouseOverTimeShowMenu() const;

		void setMouseOverTimeShowMenu(const Float & maxTime);

	protected:
		UIMenu *	mSubMenu;
		UISkin *	mSkinArrow;
		UIGfx	*	mArrow;
		Float		mTimeOver;
		Float		mMaxTime;
		Uint32		mCbId;
		Uint32		mCbId2;

		virtual Uint32 onMouseExit( const Vector2i &position, const Uint32 flags );

		virtual Uint32 onMouseMove( const Vector2i &position, const Uint32 getFlags );

		virtual void onStateChange();

		virtual void onSizeChange();

		void onSubMenuFocusLoss( const UIEvent * Event );

		void onHideByClick( const UIEvent * Event );
};

}}

#endif
