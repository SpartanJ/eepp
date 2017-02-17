#ifndef EE_WINDOWCWINDOWSFML_HPP
#define EE_WINDOWCWINDOWSFML_HPP

#ifdef EE_BACKEND_SFML_ACTIVE

#include <eepp/window/window.hpp>

#ifdef None
#undef None
#endif
#include <SFML/Window.hpp>

namespace EE { namespace Window { namespace Backend { namespace SFML {

class EE_API WindowSFML : public Window {
	public:
		WindowSFML( WindowSettings Settings, ContextSettings Context );
		
		virtual ~WindowSFML();
		
		bool create( WindowSettings Settings, ContextSettings Context );
		
		void toggleFullscreen();
		
		void setCaption( const std::string& setCaption );

		bool setIcon( const std::string& Path );

		void hide();

		void show();

		void setPosition( Int16 Left, Int16 Top );

		bool isActive();

		bool isVisible();

		Vector2i getPosition();

		void setSize( Uint32 Width, Uint32 Height, bool Windowed );

		std::vector<DisplayMode> getDisplayModes() const;

		void setGamma( Float Red, Float Green, Float Blue );

		eeWindowContex getContext() const;

		eeWindowHandle	getWindowHandler();

		void setDefaultContext();

		sf::Window * getSFMLWindow();
	protected:
		friend class ClipboardSFML;
		friend class InputSFML;

		sf::Window mSFMLWindow;

		eeWindowHandle mWinHandler;

		bool mVisible;

		void createPlatform();

		void swapBuffers();

		void getMainContext();

		std::string getVersion();

		void videoResize( Uint32 Width, Uint32 Height );
};

}}}}

#endif

#endif
