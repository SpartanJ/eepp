#include <eepp/ui/uisliderbutton.hpp>
#include <eepp/ui/uislider.hpp>

namespace EE { namespace UI { namespace Private {

UISliderButton::UISliderButton( const UIControlAnim::CreateParams& Params ) :
	UIControlAnim( Params )
{
	ApplyDefaultTheme();
}

UISliderButton::~UISliderButton() {
}

void UISliderButton::OnPosChange() {
	UIControlAnim::OnPosChange();

	UISlider * Slider = reinterpret_cast<UISlider*> ( mParentCtrl );
	Slider->FixSliderPos();
}

}}}
