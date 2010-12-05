#ifndef EE_UICUISLIDERBUTTON_HPP
#define EE_UICUISLIDERBUTTON_HPP

#include "cuicontrolanim.hpp"

namespace EE { namespace UI { namespace Private {

class cUISliderButton : public cUIControlAnim {
	public:
		cUISliderButton( const cUIControlAnim::CreateParams& Params );

		~cUISliderButton();
	protected:
		virtual void OnPosChange();
};

}}}

#endif
