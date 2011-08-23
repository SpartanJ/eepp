#ifndef EE_CUIMENUSUBMENU_HPP
#define EE_CUIMENUSUBMENU_HPP

#include "cuimenuitem.hpp"

namespace EE { namespace UI {

class cUIMenu;

class cUIMenuSubMenu : public cUIMenuItem {
	public:
		class CreateParams : public cUIMenuItem::CreateParams {
			public:
				inline CreateParams() :
					cUIMenuItem::CreateParams(),
					SubMenu( NULL ),
					MouseOverTimeShowMenu( 200.f )
				{
				}

				inline ~CreateParams() {}

				cUIMenu * SubMenu;
				eeFloat MouseOverTimeShowMenu;
		};

		cUIMenuSubMenu( cUIMenuSubMenu::CreateParams& Params );

		~cUIMenuSubMenu();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void SetTheme( cUITheme * Theme );

		void SubMenu( cUIMenu * SubMenu );

		cUIMenu * SubMenu() const;

		cUIGfx * Arrow() const;

		void ShowSubMenu();

		virtual bool InheritsFrom( const Uint32 Type );
	protected:
		cUIMenu *	mSubMenu;
		cUISkin *	mSkinArrow;
		cUIGfx	*	mArrow;
		eeFloat		mTimeOver;
		eeFloat		mMaxTime;
		Uint32		mCbId;
		Uint32		mCbId2;

		virtual Uint32 OnMouseExit( const eeVector2i &Pos, Uint32 Flags );

		virtual Uint32 OnMouseMove( const eeVector2i &Pos, Uint32 Flags );

		virtual void OnStateChange();

		virtual void OnSizeChange();

		void OnSubMenuFocusLoss( const cUIEvent * Event );

		void OnHideByClick( const cUIEvent * Event );
};

}}

#endif
