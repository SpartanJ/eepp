#ifndef EE_WINDOWCWINDOWNULL_HPP
#define EE_WINDOWCWINDOWNULL_HPP

#include <eepp/window/window.hpp>

namespace EE { namespace Window { namespace Backend { namespace Null {

class EE_API WindowNull : public Window {
	public:
		WindowNull( WindowSettings Settings, ContextSettings Context );
		
		virtual ~WindowNull();
		
		bool create( WindowSettings Settings, ContextSettings Context );
		
		void toggleFullscreen();
		
		void caption( const std::string& caption );
		
		std::string caption();

		bool icon( const std::string& Path );

		void minimize();

		void maximize();

		void hide();

		void raise();

		void show();

		void position( Int16 Left, Int16 Top );

		bool active();

		bool visible();

		Vector2i position();

		void size( Uint32 Width, Uint32 Height, bool isWindowed );

		std::vector<DisplayMode> getDisplayModes() const;

		void setGamma( Float Red, Float Green, Float Blue );

		eeWindowContex getContext() const;

		eeWindowHandle	getWindowHandler();

		void setDefaultContext();
	protected:
		friend class ClipboardNull;

		void swapBuffers();

		void getMainContext();
};

}}}}

#endif
