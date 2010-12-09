#ifndef EE_UICUIMENUITEM_HPP
#define EE_UICUIMENUITEM_HPP

#include "cuipushbutton.hpp"

namespace EE { namespace UI {

class EE_API cUIMenuItem : public cUIPushButton {
	public:
		cUIMenuItem( cUIMenuItem::CreateParams& Params );

		~cUIMenuItem();

		virtual void SetTheme( cUITheme * Theme );
	protected:
		virtual Uint32 OnMouseEnter( const eeVector2i &Pos, Uint32 Flags );

		virtual void OnStateChange();
};

}}

#endif
