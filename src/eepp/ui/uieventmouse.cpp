#include <eepp/ui/uieventmouse.hpp>
#include <eepp/ui/uicontrol.hpp>

namespace EE { namespace UI {

UIEventMouse::UIEventMouse( UIControl * Ctrl, const Uint32& EventNum, const Vector2i& Pos, const Uint32& Flags ) :
	UIEvent( Ctrl, EventNum ),
	mPos( Pos ),
	mFlags( Flags )
{
}

UIEventMouse::~UIEventMouse()
{
}

const Vector2i& UIEventMouse::Pos() const {
	return mPos;
}

const Uint32& UIEventMouse::Flags() const {
	return mFlags;
}

}}

