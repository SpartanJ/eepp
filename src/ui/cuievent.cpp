#include "cuievent.hpp"
#include "cuicontrol.hpp"

namespace EE { namespace UI { 

cUIEvent::cUIEvent( cUIControl * Ctrl ) {
	mCtrl = Ctrl;
}

cUIEvent::~cUIEvent()
{
}

cUIControl * cUIEvent::Ctrl() const {
	return mCtrl;
}

}}