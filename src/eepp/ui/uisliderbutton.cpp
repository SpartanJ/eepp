#include <eepp/ui/uisliderbutton.hpp>
#include <eepp/ui/uislider.hpp>

namespace EE { namespace UI { namespace Private {

UISliderButton *UISliderButton::New() {
	return eeNew( UISliderButton, () );
}

UISliderButton::UISliderButton() :
	UIWidget( "slider::button" )
{
	applyDefaultTheme();
}

UISliderButton::~UISliderButton() {
}

void UISliderButton::onPositionChange() {
	UIWidget::onPositionChange();

	mParentCtrl->asType<UISlider>()->fixSliderPos();
}

}}}
