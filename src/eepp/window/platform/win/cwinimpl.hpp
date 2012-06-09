#ifndef EE_WINDOWCWINIMPL_HPP
#define EE_WINDOWCWINIMPL_HPP

#include <eepp/window/base.hpp>
#include <eepp/window/cplatformimpl.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN

namespace EE { namespace Window { namespace Platform {

using namespace EE::Window;

class cWinImpl : public cPlatformImpl {
	public:
		cWinImpl( Window::cWindow * window, eeWindowHandler handler );

		~cWinImpl();

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

		eeWindowHandler GetHandler() const;
	protected:
		eeWindowHandler mHandler;
		HCURSOR			mCursorCurrent;
		bool			mCursorHidden;
};

}}}

#endif

#endif
