#include <eepp/ui/uilayout.hpp>

namespace EE { namespace UI {

UILayout * UILayout::New() {
	return eeNew( UILayout, () );
}

UILayout::UILayout() :
	UIWidget()
{
}

Uint32 UILayout::getType() const {
	return UI_TYPE_LAYOUT;
}

bool UILayout::isType( const Uint32& type ) const {
	return UIWidget::getType() == type ? true : UIWidget::isType( type );
}

}}
