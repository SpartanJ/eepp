#ifndef EE_WINDOWCINPUTSDL_HPP
#define EE_WINDOWCINPUTSDL_HPP

#include <eepp/window/backend.hpp>
#include <eepp/window/backend/SDL/base.hpp>

#ifdef EE_BACKEND_SDL_1_2

#include <eepp/window/input.hpp>

namespace EE { namespace Window { namespace Backend { namespace SDL {

class EE_API InputSDL : public Input {
	public:
		virtual ~InputSDL();

		void Update();

		bool GrabInput();

		void GrabInput( const bool& Grab );

		void InjectMousePos( const Uint16& x, const Uint16& y );
	protected:
		friend class WindowSDL;

		InputSDL( EE::Window::Window * window );

		virtual void Init();
};

}}}}

#endif

#endif
