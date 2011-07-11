#ifndef EE_WINDOWCCURSORAL_HPP
#define EE_WINDOWCCURSORAL_HPP

#include "../../ccursor.hpp"

#ifdef EE_BACKEND_ALLEGRO_ACTIVE

#include <allegro5/allegro.h>

using namespace EE::Window;

namespace EE { namespace Window { namespace Backend { namespace Al {

class cCursorAl : public cCursor {
	public:
		ALLEGRO_MOUSE_CURSOR * GetCursor() const;
	protected:
		friend class cCursorManagerAl;

		ALLEGRO_MOUSE_CURSOR * mCursor;

		cCursorAl( cTexture * tex, const eeVector2i& hotspot, const std::string& name, Window::cWindow * window );

		cCursorAl( cImage * img, const eeVector2i& hotspot, const std::string& name, Window::cWindow * window );

		cCursorAl( const std::string& path, const eeVector2i& hotspot, const std::string& name, Window::cWindow * window );

		~cCursorAl();

		void Create();
};

}}}}

#endif

#endif
