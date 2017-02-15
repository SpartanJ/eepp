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

		void update();

		bool grabInput();

		void grabInput( const bool& Grab );

		void injectMousePos( const Uint16& x, const Uint16& y );
	protected:
		friend class WindowSDL;

		InputSDL( EE::Window::Window * window );

		virtual void init();
};

}}}}

#endif

#endif
