#include <eepp/ui/uisliderbutton.hpp>
#include <eepp/ui/uislider.hpp>

namespace EE { namespace UI { namespace Private {

UISliderButton *UISliderButton::New() {
	return eeNew( UISliderButton, () );
}

UISliderButton::UISliderButton() :
	UIWidget( "sliderbutton" )
{
	applyDefaultTheme();
}

UISliderButton::~UISliderButton() {
}

void UISliderButton::onPositionChange() {
	UIWidget::onPositionChange();

	UISlider * Slider = reinterpret_cast<UISlider*> ( mParentCtrl );
	Slider->fixSliderPos();
}

}}}
