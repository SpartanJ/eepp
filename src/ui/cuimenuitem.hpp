#ifndef EE_UICUIMENUITEM_HPP
#define EE_UICUIMENUITEM_HPP

#include "cuipushbutton.hpp"

namespace EE { namespace UI {

class EE_API cUIMenuItem : public cUIPushButton {
	public:
		cUIMenuItem( cUIPushButton::CreateParams& Params );

		~cUIMenuItem();

		virtual void SetTheme( cUITheme * Theme );
};

}}

#endif
