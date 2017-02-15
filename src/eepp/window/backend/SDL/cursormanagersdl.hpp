#ifndef EE_WINDOWCCURSORMANAGERSDL_HPP
#define EE_WINDOWCCURSORMANAGERSDL_HPP

#include <eepp/window/cursormanager.hpp>
#include <eepp/window/backend/SDL/base.hpp>

#ifdef EE_BACKEND_SDL_1_2

using namespace EE::Window;

namespace EE { namespace Window { namespace Backend { namespace SDL {

class CursorManagerSDL : public CursorManager {
	public:
		CursorManagerSDL( EE::Window::Window * window );

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
