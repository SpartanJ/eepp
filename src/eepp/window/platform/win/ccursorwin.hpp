#ifndef EE_WINDOWCCURSORWIN_HPP
#define EE_WINDOWCCURSORWIN_HPP

#include <eepp/window/ccursor.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN

using namespace EE::Window;

namespace EE { namespace Window { namespace Platform {

class cWinImpl;

class cCursorWin : public cCursor {
    public:
        HCURSOR     GetCursor() const;
	protected:
		friend class cWinImpl;

		HCURSOR       mCursor;

		cCursorWin( cTexture * tex, const eeVector2i& hotspot, const std::string& name, cWindow * window );

		cCursorWin( cImage * img, const eeVector2i& hotspot, const std::string& name, cWindow * window );

		cCursorWin( const std::string& path, const eeVector2i& hotspot, const std::string& name, cWindow * window );

		~cCursorWin();

		void Create();

		cWinImpl * GetPlatform();
};

}}}

#endif

#endif
