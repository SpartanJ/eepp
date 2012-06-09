#ifndef EE_UICUIEVENTMOUSE_HPP
#define EE_UICUIEVENTMOUSE_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/cuievent.hpp>

namespace EE { namespace UI {

class cUIControl;

class EE_API cUIEventMouse : public cUIEvent {
	public:
		cUIEventMouse( cUIControl * Ctrl, const Uint32& EventNum, const eeVector2i& Pos, const Uint32& Flags );

		~cUIEventMouse();

		const eeVector2i& Pos() const;

		const Uint32& Flags() const;
	protected:
		eeVector2i	mPos;
		Uint32	mFlags;
};

}}

#endif
 
