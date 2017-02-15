#ifndef EE_WINDOWCCURSORNULL_HPP
#define EE_WINDOWCCURSORNULL_HPP

#include <eepp/window/cursor.hpp>

using namespace EE::Window;

namespace EE { namespace Window { namespace Backend { namespace Null {

class CursorNull : public Cursor {
	protected:
		friend class CursorManagerNull;

		CursorNull( Texture * tex, const Vector2i& hotspot, const std::string& getName, EE::Window::Window * window );

		CursorNull( getImage * img, const Vector2i& hotspot, const std::string& getName, EE::Window::Window * window );

		CursorNull( const std::string& path, const Vector2i& hotspot, const std::string& getName, EE::Window::Window * window );

		void create();
};

}}}}

#endif
