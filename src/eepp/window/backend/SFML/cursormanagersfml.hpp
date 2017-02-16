#ifndef EE_WINDOWCCURSORMANAGERSFML_HPP
#define EE_WINDOWCCURSORMANAGERSFML_HPP

#ifdef EE_BACKEND_SFML_ACTIVE

#include <eepp/window/cursormanager.hpp>

using namespace EE::Window;

namespace EE { namespace Window { namespace Backend { namespace SFML {

class CursorManagerSFML : public CursorManager {
	public:
		CursorManagerSFML( EE::Window::Window * window );

		Cursor * create( Texture * tex, const Vector2i& hotspot, const std::string& name );

		Cursor * create( Image * img, const Vector2i& hotspot, const std::string& name );

		Cursor * create( const std::string& path, const Vector2i& hotspot, const std::string& name );

		void set( Cursor * cursor );

		void set( EE_SYSTEM_CURSOR syscurid );

		void show();

		void hide();

		void visible( bool visible );

		void remove( Cursor * cursor, bool Delete = false );

		void reload();
};

}}}}

#endif

#endif
