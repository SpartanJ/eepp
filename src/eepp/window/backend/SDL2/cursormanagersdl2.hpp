#ifndef EE_WINDOWCCURSORMANAGERSDL2_HPP
#define EE_WINDOWCCURSORMANAGERSDL2_HPP

#include <eepp/window/cursormanager.hpp>
#include <eepp/window/backend/SDL2/base.hpp>

#ifdef EE_BACKEND_SDL2

using namespace EE::Window;

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

class CursorManagerSDL : public CursorManager {
	public:
		CursorManagerSDL( EE::Window::Window * window );

		Cursor * Create( Texture * tex, const Vector2i& hotspot, const std::string& name );

		Cursor * Create( Image * img, const Vector2i& hotspot, const std::string& name );

		Cursor * Create( const std::string& path, const Vector2i& hotspot, const std::string& name );

		void Set( Cursor * cursor );

		void Set( EE_SYSTEM_CURSOR syscurid );

		void Show();

		void Hide();

		void Visible( bool visible );

		void Remove( Cursor * cursor, bool Delete = false );

		void Reload();
};

}}}}

#endif

#endif
