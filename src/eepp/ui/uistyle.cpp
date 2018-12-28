#include <eepp/ui/uistyle.hpp>

namespace EE { namespace UI {

UIStyle * EE::UI::UIStyle::New() {
	return eeNew( UIStyle, () );
}

UIStyle::UIStyle()
{
}

UIStyle::~UIStyle()
{
}

bool UIStyle::stateExists( const EE::Uint32 &  ) {
	return false;
}

void UIStyle::addAttribute(int state, NodeAttribute attribute) {
	mStates[ state ][ attribute.getName() ] = attribute;
}

}}
