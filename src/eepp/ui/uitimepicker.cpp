#include <eepp/ui/uitimepicker.hpp>

namespace EE { namespace UI {

UITimePicker* UITimePicker::New() {
	return eeNew( UITimePicker, () );
}

UITimePicker::UITimePicker() : UIDateTimeEdit( DateTimeEditMode::Time, "timepicker" ) {}

Uint32 UITimePicker::getType() const {
	return UI_TYPE_TIMEPICKER;
}

bool UITimePicker::isType( const Uint32& type ) const {
	return type == UI_TYPE_TIMEPICKER || UIDateTimeEdit::isType( type );
}

}} // namespace EE::UI
