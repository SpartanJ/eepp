#ifndef EE_WINDOW_CINPUTFINGER_HPP
#define EE_WINDOW_CINPUTFINGER_HPP

#include "base.hpp"

namespace EE { namespace Window {

#define EE_MAX_FINGERS (10)

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

		bool IsDown();

		bool WasDown();

		eeVector2i Pos();
	protected:
		friend class cInput;

		void WriteLast();
};

}}

#endif
