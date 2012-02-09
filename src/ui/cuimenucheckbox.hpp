#ifndef EE_UICUIMENUCHECKBOX_HPP
#define EE_UICUIMENUCHECKBOX_HPP

#include "cuimenuitem.hpp"

namespace EE { namespace UI {

class EE_API cUIMenuCheckBox : public cUIMenuItem {
	public:
		cUIMenuCheckBox( cUIMenuCheckBox::CreateParams& Params );

		virtual ~cUIMenuCheckBox();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void SetTheme( cUITheme * Theme );

		const bool& Active() const;

		const bool& IsActive() const;

		void Active( const bool& active );

		void SwitchActive();

		virtual bool InheritsFrom( const Uint32 Type );
	protected:
		bool		mActive;
		cUISkin *	mSkinActive;
		cUISkin *	mSkinInactive;

		virtual Uint32 OnMouseUp( const eeVector2i &Pos, Uint32 Flags );

		virtual void OnStateChange();
};

}}

#endif
