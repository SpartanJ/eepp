#ifndef EE_WINDOWCWINDOWSFML_HPP
#define EE_WINDOWCWINDOWSFML_HPP

#ifdef EE_BACKEND_SFML_ACTIVE

#include <eepp/window/cwindow.hpp>

#ifdef None
#undef None
#endif
#include <SFML/Window.hpp>

namespace EE { namespace Window { namespace Backend { namespace SFML {

class EE_API cWindowSFML : public cWindow {
	public:
		cWindowSFML( WindowSettings Settings, ContextSettings Context );
		
		virtual ~cWindowSFML();
		
		bool Create( WindowSettings Settings, ContextSettings Context );
		
		void ToggleFullscreen();
		
		void Caption( const std::string& Caption );

		bool Icon( const std::string& Path );

		void Hide();

		void Show();

		void Position( Int16 Left, Int16 Top );

		bool Active();

		bool Visible();

		eeVector2i Position();

		void Size( Uint32 Width, Uint32 Height, bool Windowed );

		std::vector< std::pair<unsigned int, unsigned int> > GetPossibleResolutions() const;

		void SetGamma( eeFloat Red, eeFloat Green, eeFloat Blue );

		void SetCurrentContext( eeWindowContex Context );

		eeWindowContex GetContext() const;

		eeWindowHandler	GetWindowHandler();

		void SetDefaultContext();

		sf::Window * GetSFMLWindow();
	protected:
		friend class cClipboardSFML;
		friend class cInputSFML;

		sf::Window mSFMLWindow;

		eeWindowHandler mWinHandler;

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
