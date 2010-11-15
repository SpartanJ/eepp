#include "cuisliderbutton.hpp"
#include "cuislider.hpp"

namespace EE { namespace UI { namespace Private {

cUISliderButton::cUISliderButton( const cUIDragable::CreateParams& Params ) :
	cUIDragable( Params )
{
	ApplyDefaultTheme();
}

cUISliderButton::~cUISliderButton() {
}

void cUISliderButton::OnPosChange() {
	cUIDragable::OnPosChange();

	cUISlider * Slider = reinterpret_cast<cUISlider*> ( mParentCtrl );
	Slider->FixSliderPos();
}

}}}
