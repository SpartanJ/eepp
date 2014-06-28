#ifndef EE_CUIMENUSUBMENU_HPP
#define EE_CUIMENUSUBMENU_HPP

#include <eepp/ui/uimenuitem.hpp>

namespace EE { namespace UI {

class UIMenu;

class EE_API UIMenuSubMenu : public UIMenuItem {
	public:
		class CreateParams : public UIMenuItem::CreateParams {
			public:
				inline CreateParams() :
					UIMenuItem::CreateParams(),
					SubMenu( NULL ),
					MouseOverTimeShowMenu( 200.f )
				{
				}

				inline ~CreateParams() {}

				UIMenu * SubMenu;
				Float MouseOverTimeShowMenu;
		};

		UIMenuSubMenu( UIMenuSubMenu::CreateParams& Params );

		virtual ~UIMenuSubMenu();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void SetTheme( UITheme * Theme );

		void SubMenu( UIMenu * SubMenu );

		UIMenu * SubMenu() const;

		UIGfx * Arrow() const;

		void ShowSubMenu();

		virtual bool InheritsFrom( const Uint32 Type );
	protected:
		UIMenu *	mSubMenu;
		UISkin *	mSkinArrow;
		UIGfx	*	mArrow;
		Float		mTimeOver;
		Float		mMaxTime;
		Uint32		mCbId;
		Uint32		mCbId2;

		virtual Uint32 OnMouseExit( const Vector2i &Pos, const Uint32 Flags );

		virtual Uint32 OnMouseMove( const Vector2i &Pos, const Uint32 Flags );

		virtual void OnStateChange();

		virtual void OnSizeChange();

		void OnSubMenuFocusLoss( const UIEvent * Event );

		void OnHideByClick( const UIEvent * Event );
};

}}

#endif
