#ifndef EE_WINDOWCCURSORMANAGERSFML_HPP
#define EE_WINDOWCCURSORMANAGERSFML_HPP

#ifdef EE_BACKEND_SFML_ACTIVE

#include <eepp/window/ccursormanager.hpp>

using namespace EE::Window;

namespace EE { namespace Window { namespace Backend { namespace SFML {

class cCursorManagerSFML : public cCursorManager {
	public:
		cCursorManagerSFML( Window::cWindow * window );

		cCursor * Create( cTexture * tex, const Vector2i& hotspot, const std::string& name );

		cCursor * Create( cImage * img, const Vector2i& hotspot, const std::string& name );

		cCursor * Create( const std::string& path, const Vector2i& hotspot, const std::string& name );

		void Set( cCursor * cursor );

		void Set( EE_SYSTEM_CURSOR syscurid );

		void Show();

		void Hide();

		void Visible( bool visible );

		void Remove( cCursor * cursor, bool Delete = false );

		void Reload();
};

}}}}

#endif

#endif
