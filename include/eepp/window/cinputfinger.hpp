#ifndef EE_WINDOW_CINPUTFINGER_HPP
#define EE_WINDOW_CINPUTFINGER_HPP

#include <eepp/window/base.hpp>

namespace EE { namespace Window {

#define EE_MAX_FINGERS (10)

/** @brief A representation of the current state of a finger touching a screen */
class cInputFinger {
	public:
		cInputFinger();
		
		Int64 id;
		Uint16 x;
		Uint16 y;
		Uint16 pressure;
		Uint16 xdelta;
		Uint16 ydelta;
		Uint16 last_x;
		Uint16 last_y;
		Uint16 last_pressure;
		bool down;
		bool was_down;

		/** @return If is currently pressed */
		bool IsDown();

		/** @return If was down in the last update */
		bool WasDown();

		/** @return The current position of the finger */
		eeVector2i Pos();
	protected:
		friend class cInput;

		void WriteLast();
};

}}

#endif
