#include "cuisliderbutton.hpp"
#include "cuislider.hpp"

namespace EE { namespace UI { namespace Private {

cUISliderButton::cUISliderButton( const cUIControlAnim::CreateParams& Params ) :
	cUIControlAnim( Params )
{
	ApplyDefaultTheme();
}

cUISliderButton::~cUISliderButton() {
}

void cUISliderButton::OnPosChange() {
	cUIControlAnim::OnPosChange();

	cUISlider * Slider = reinterpret_cast<cUISlider*> ( mParentCtrl );
	Slider->FixSliderPos();
}

}}}
