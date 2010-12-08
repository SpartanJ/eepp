#ifndef EE_UICUIMENUCHECKBOX_HPP
#define EE_UICUIMENUCHECKBOX_HPP

#include "cuimenuitem.hpp"

namespace EE { namespace UI {

class cUIMenuCheckBox : public cUIMenuItem {
	public:
		cUIMenuCheckBox( cUIMenuCheckBox::CreateParams& Params );

		~cUIMenuCheckBox();

		virtual void SetTheme( cUITheme * Theme );

		const bool& Active() const;

		const bool& IsActive() const;

		void Active( const bool& active );

		void SwitchActive();
	protected:
		bool		mActive;
		cUISkin *	mSkinActive;
		cUISkin *	mSkinInactive;

		virtual Uint32 OnMouseClick( const eeVector2i &Pos, Uint32 Flags );

		virtual void OnStateChange();
};

}}

#endif
