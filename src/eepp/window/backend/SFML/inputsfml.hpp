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
		
		void Update();

		bool GrabInput();

		void GrabInput( const bool& Grab );

		void InjectMousePos( const Uint16& x, const Uint16& y );
	protected:
		friend class WindowSFML;

		bool mWinActive;

		InputSFML( EE::Window::Window * window );
		
		virtual void Init();

		Uint32 GetButton( const Uint32& sfmlBut );

		Uint32 SetMod( sf::Event::KeyEvent& key );

		void InitializeTables();
};

}}}}

#endif

#endif
