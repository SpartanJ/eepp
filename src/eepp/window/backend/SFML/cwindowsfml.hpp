#ifndef EE_WINDOWCWINDOWSFML_HPP
#define EE_WINDOWCWINDOWSFML_HPP

#ifdef EE_BACKEND_SFML_ACTIVE

#include <eepp/window/cwindow.hpp>

#ifdef None
#undef None
#endif
#include <SFML/Window.hpp>

namespace EE { namespace Window { namespace Backend { namespace SFML {

class EE_API WindowSFML : public Window {
	public:
		WindowSFML( WindowSettings Settings, ContextSettings Context );
		
		virtual ~WindowSFML();
		
		bool Create( WindowSettings Settings, ContextSettings Context );
		
		void ToggleFullscreen();
		
		void Caption( const std::string& Caption );

		bool Icon( const std::string& Path );

		void Hide();

		void Show();

		void Position( Int16 Left, Int16 Top );

		bool Active();

		bool Visible();

		Vector2i Position();

		void Size( Uint32 Width, Uint32 Height, bool Windowed );

		std::vector<DisplayMode> GetDisplayModes() const;

		void SetGamma( Float Red, Float Green, Float Blue );

		eeWindowContex GetContext() const;

		eeWindowHandle	GetWindowHandler();

		void SetDefaultContext();

		sf::Window * GetSFMLWindow();
	protected:
		friend class ClipboardSFML;
		friend class InputSFML;

		sf::Window mSFMLWindow;

		eeWindowHandle mWinHandler;

		bool mVisible;

		void CreatePlatform();

		void SwapBuffers();

		void GetMainContext();

		std::string GetVersion();

		void VideoResize( Uint32 Width, Uint32 Height );
};

}}}}

#endif

#endif
