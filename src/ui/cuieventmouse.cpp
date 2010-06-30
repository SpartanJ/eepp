#include "cuieventmouse.hpp"
#include "cuicontrol.hpp"

namespace EE { namespace UI {

cUIEventMouse::cUIEventMouse( cUIControl * Ctrl, const Uint32& EventNum, const eeVector2i& Pos, const Uint32& Flags ) :
	cUIEvent( Ctrl, EventNum ),
	mPos( Pos ),
	mFlags( Flags )
{
}

cUIEventMouse::~cUIEventMouse()
{
}

const eeVector2i& cUIEventMouse::Pos() const {
	return mPos;
}

const Uint32& cUIEventMouse::Flags() const {
	return mFlags;
}

}}

