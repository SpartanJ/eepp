#ifndef EE_UICUISLIDERBUTTON_HPP
#define EE_UICUISLIDERBUTTON_HPP

#include "cuidragable.hpp"

namespace EE { namespace UI { namespace Private {

class cUISliderButton : public cUIDragable {
	public:
		cUISliderButton( const cUIDragable::CreateParams& Params );

		~cUISliderButton();
	protected:
		virtual void OnPosChange();
};

}}}

#endif
