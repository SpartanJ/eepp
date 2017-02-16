#ifndef EE_UICUIEVENTMOUSE_HPP
#define EE_UICUIEVENTMOUSE_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/uievent.hpp>

namespace EE { namespace UI {

class UIControl;

class EE_API UIEventMouse : public UIEvent {
	public:
		UIEventMouse( UIControl * getControl, const Uint32& EventNum, const Vector2i& getPosition, const Uint32& getFlags );

		~UIEventMouse();

		const Vector2i& getPosition() const;

		const Uint32& getFlags() const;
	protected:
		Vector2i	mPos;
		Uint32	mFlags;
};

}}

#endif
 
