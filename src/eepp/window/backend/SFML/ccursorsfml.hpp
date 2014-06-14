#ifndef EE_WINDOWCCURSORSFML_HPP
#define EE_WINDOWCCURSORSFML_HPP

#ifdef EE_BACKEND_SFML_ACTIVE

#include <eepp/window/ccursor.hpp>

using namespace EE::Window;

namespace EE { namespace Window { namespace Backend { namespace SFML {

class CursorSFML : public Cursor {
	protected:
		friend class CursorManagerSFML;

		CursorSFML( cTexture * tex, const Vector2i& hotspot, const std::string& name, Window::cWindow * window );

		CursorSFML( cImage * img, const Vector2i& hotspot, const std::string& name, Window::cWindow * window );

		CursorSFML( const std::string& path, const Vector2i& hotspot, const std::string& name, Window::cWindow * window );

		void Create();
};

}}}}

#endif

#endif
