#ifndef EE_UICUIPUSHBUTTON_HPP
#define EE_UICUIPUSHBUTTON_HPP

#include "cuitextbox.hpp"

namespace EE { namespace UI {

class EE_API cUIPushButton : public cUITextBox {
	public:
		cUIPushButton( const cUITextBox::CreateParams& Params );

		~cUIPushButton();

		virtual void SetTheme( cUITheme * Theme );
	protected:

};

}}

#endif

