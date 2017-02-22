#include <eepp/ui/uisliderbutton.hpp>
#include <eepp/ui/uislider.hpp>

namespace EE { namespace UI { namespace Private {

UISliderButton::UISliderButton( const UIControlAnim::CreateParams& Params ) :
	UIControlAnim( Params )
{
	applyDefaultTheme();
}

UISliderButton::UISliderButton() :
	UIControlAnim()
{
	applyDefaultTheme();
}

UISliderButton::~UISliderButton() {
}

void UISliderButton::onPositionChange() {
	UIControlAnim::onPositionChange();

	UISlider * Slider = reinterpret_cast<UISlider*> ( mParentCtrl );
	Slider->fixSliderPos();
}

}}}
