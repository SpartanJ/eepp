#ifndef EE_WINDOWCOSXIMPL_HPP
#define EE_WINDOWCOSXIMPL_HPP

#include <eepp/window/base.hpp>
#include <eepp/window/cplatformimpl.hpp>

#if EE_PLATFORM == EE_PLATFORM_MACOSX

namespace EE { namespace Window { namespace Platform {

using namespace EE::Window;

class cOSXImpl : public cPlatformImpl {
	public:
		cOSXImpl( Window::cWindow * window );

		~cOSXImpl();

		void MinimizeWindow();

		void MaximizeWindow();

		void HideWindow();

		void RaiseWindow();

		void ShowWindow();

		void MoveWindow( int left, int top );

		void SetContext( eeWindowContex Context );

		eeVector2i Position();

		void ShowMouseCursor();

		void HideMouseCursor();

		cCursor * CreateMouseCursor( cTexture * tex, const eeVector2i& hotspot, const std::string& name );

		cCursor * CreateMouseCursor( cImage * img, const eeVector2i& hotspot, const std::string& name );

		cCursor * CreateMouseCursor( const std::string& path, const eeVector2i& hotspot, const std::string& name );

		void SetMouseCursor( cCursor * cursor );

		void SetSystemMouseCursor( Cursor::EE_SYSTEM_CURSOR syscursor );

		void RestoreCursor();
};

}}}

#endif

#endif
