#ifndef EE_WINDOWCWINDOWNULL_HPP
#define EE_WINDOWCWINDOWNULL_HPP

#include <eepp/window/window.hpp>

namespace EE { namespace Window { namespace Backend { namespace Null {

class EE_API WindowNull : public Window {
	public:
		WindowNull( WindowSettings Settings, ContextSettings Context );
		
		virtual ~WindowNull();
		
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

		Vector2i Position();

		void Size( Uint32 Width, Uint32 Height, bool Windowed );

		std::vector<DisplayMode> GetDisplayModes() const;

		void SetGamma( Float Red, Float Green, Float Blue );

		eeWindowContex GetContext() const;

		eeWindowHandle	GetWindowHandler();

		void SetDefaultContext();
	protected:
		friend class ClipboardNull;

		void SwapBuffers();

		void GetMainContext();
};

}}}}

#endif
