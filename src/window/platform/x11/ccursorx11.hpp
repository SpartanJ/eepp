#ifndef EE_WINDOWCCURSORX11_HPP
#define EE_WINDOWCCURSORX11_HPP

#include "../../ccursor.hpp"

#if defined( EE_X11_PLATFORM )

using namespace EE::Window;

namespace EE { namespace Window { namespace Platform {

class cX11Impl;

class cCursorX11 : public cCursor {
	public:
		X11Cursor	GetCursor() const;
	protected:
		friend class cX11Impl;

		X11Cursor	mCursor;

		cCursorX11( cTexture * tex, const eeVector2i& hotspot, const std::string& name, Window::cWindow * window );

		cCursorX11( cImage * img, const eeVector2i& hotspot, const std::string& name, Window::cWindow * window );

		cCursorX11( const std::string& path, const eeVector2i& hotspot, const std::string& name, Window::cWindow * window );

		~cCursorX11();

		void Create();

		cX11Impl * GetPlatform();
};

}}}

#endif

#endif
