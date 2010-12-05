#ifndef EE_UICUIMENUITEM
#define EE_UICUIMENUITEM

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
