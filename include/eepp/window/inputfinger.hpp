#ifndef EE_WINDOW_CINPUTFINGER_HPP
#define EE_WINDOW_CINPUTFINGER_HPP

#include <eepp/window/base.hpp>

namespace EE { namespace Window {

#define EE_MAX_FINGERS (10)

/** @brief A representation of the current state of a finger touching a screen */
class InputFinger {
	public:
		InputFinger();
		
		Int64 id;
		Int32 x;
		Int32 y;
		float pressure;
		float xdelta;
		float ydelta;
		Int32 last_x;
		Int32 last_y;
		float lastPressure;
		bool down;
		bool wasDown;

		/** @return If is currently pressed */
		bool isDown();

		/** @return If was down in the last update */
		bool getWasDown();

		/** @return The current position of the finger */
		Vector2i getPos();
	protected:
		friend class Input;

		void writeLast();
};

}}

#endif
