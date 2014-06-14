#ifndef EE_WINDOWCCURSORNULL_HPP
#define EE_WINDOWCCURSORNULL_HPP

#include <eepp/window/ccursor.hpp>

using namespace EE::Window;

namespace EE { namespace Window { namespace Backend { namespace Null {

class CursorNull : public Cursor {
	protected:
		friend class CursorManagerNull;

		CursorNull( cTexture * tex, const Vector2i& hotspot, const std::string& name, Window::cWindow * window );

		CursorNull( cImage * img, const Vector2i& hotspot, const std::string& name, Window::cWindow * window );

		CursorNull( const std::string& path, const Vector2i& hotspot, const std::string& name, Window::cWindow * window );

		void Create();
};

}}}}

#endif
