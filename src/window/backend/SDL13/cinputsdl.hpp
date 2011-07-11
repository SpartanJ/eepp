#ifndef EE_WINDOWCINPUTSDL13_HPP
#define EE_WINDOWCINPUTSDL13_HPP

#include "../../cbackend.hpp"
#include "base.hpp"

#ifdef EE_BACKEND_SDL_1_3

#include "../../cinput.hpp"

namespace EE { namespace Window { namespace Backend { namespace SDL13 {

class EE_API cInputSDL : public cInput {
	public:
		virtual ~cInputSDL();
		
		void Update();

		bool GrabInput();

		void GrabInput( const bool& Grab );

		void InjectMousePos( const Uint16& x, const Uint16& y );
	protected:
		friend class cWindowSDL;

		cInputSDL( Window::cWindow * window );
		
		virtual void Init();

		Uint32 mKeyCodesTable[ SDL_NUM_SCANCODES ];

		void InitializeTables();
};

}}}}

#endif

#endif
