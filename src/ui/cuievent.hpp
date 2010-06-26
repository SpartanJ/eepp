#ifndef EE_UICUIEVENT_HPP
#define EE_UICUIEVENT_HPP

#include "base.hpp"

namespace EE { namespace UI {

class cUIControl;

class EE_API cUIEvent {
	public:
		cUIEvent( cUIControl * Ctrl );
		
		~cUIEvent();
		
		cUIControl * Ctrl() const;
	protected:
		cUIControl	* mCtrl;
};

}}

#endif 
