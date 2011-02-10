#ifndef EE_WINDOWCINPUTSDL_HPP 
#define EE_WINDOWCINPUTSDL_HPP

#include "../../cbackend.hpp"

#ifdef EE_BACKEND_SDL_ACTIVE

#include "../../cinput.hpp"
#include <SDL/SDL.h>
#include <SDL/SDL_syswm.h>

namespace EE { namespace Window { namespace Backend { namespace SDL {

class EE_API cInputSDL : public cInput {
	public:
		virtual ~cInputSDL();
		
		void Update();

		bool GrabInput();

		void GrabInput( const bool& Grab );

		void InjectMousePos( const Uint16& x, const Uint16& y );
	protected:
		friend class cWindowSDL;

		cInputSDL( cWindow * window );
		
		virtual void Init();
};

}}}}

#endif

#endif
