#ifndef EE_WINDOWCCURSORSFML_HPP
#define EE_WINDOWCCURSORSFML_HPP

#ifdef EE_BACKEND_SFML_ACTIVE

#include <eepp/window/ccursor.hpp>

using namespace EE::Window;

namespace EE { namespace Window { namespace Backend { namespace SFML {

class cCursorSFML : public cCursor {
	protected:
		friend class cCursorManagerSFML;

		cCursorSFML( cTexture * tex, const Vector2i& hotspot, const std::string& name, Window::cWindow * window );

		cCursorSFML( cImage * img, const Vector2i& hotspot, const std::string& name, Window::cWindow * window );

		cCursorSFML( const std::string& path, const Vector2i& hotspot, const std::string& name, Window::cWindow * window );

		void Create();
};

}}}}

#endif

#endif
