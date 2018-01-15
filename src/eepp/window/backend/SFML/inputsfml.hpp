#ifndef EE_WINDOWCINPUTSFML_HPP
#define EE_WINDOWCINPUTSFML_HPP

#ifdef EE_BACKEND_SFML_ACTIVE

#include <eepp/window/input.hpp>
#ifdef None
#undef None
#endif
#include <SFML/Window.hpp>

namespace EE { namespace Window { namespace Backend { namespace SFML {

class EE_API InputSFML : public Input {
	public:
		virtual ~InputSFML();
		
		void update();

		bool grabInput();

		void grabInput( const bool& Grab );

		void injectMousePos( const Uint16& x, const Uint16& y );
	protected:
		friend class WindowSFML;

		bool mWinActive;

		InputSFML( EE::Window::Window * window );
		
		virtual void init();

		Uint32 getButton( const Uint32& sfmlBut );

		Uint32 setMod( sf::Event::KeyEvent& key );

		void initializeTables();
};

}}}}

#endif

#endif
