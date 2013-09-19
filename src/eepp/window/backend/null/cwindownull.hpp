#ifndef EE_WINDOWCWINDOWNULL_HPP
#define EE_WINDOWCWINDOWNULL_HPP

#include <eepp/window/cwindow.hpp>

namespace EE { namespace Window { namespace Backend { namespace Null {

class EE_API cWindowNull : public cWindow {
	public:
		cWindowNull( WindowSettings Settings, ContextSettings Context );
		
		virtual ~cWindowNull();
		
		bool Create( WindowSettings Settings, ContextSettings Context );
		
		void ToggleFullscreen();
		
		void Caption( const std::string& Caption );
		
		std::string Caption();

		bool Icon( const std::string& Path );

		void Minimize();

		void Maximize();

		void Hide();

		void Raise();

		void Show();

		void Position( Int16 Left, Int16 Top );

		bool Active();

		bool Visible();

		eeVector2i Position();

		void Size( Uint32 Width, Uint32 Height, bool Windowed );

		std::vector<DisplayMode> GetDisplayModes() const;

		void SetGamma( eeFloat Red, eeFloat Green, eeFloat Blue );

		eeWindowContex GetContext() const;

		eeWindowHandle	GetWindowHandler();

		void SetDefaultContext();
	protected:
		friend class cClipboardNull;

		void SwapBuffers();

		void GetMainContext();
};

}}}}

#endif
