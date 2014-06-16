#ifndef EE_WINDOWCCURSORMANAGERSDL_HPP
#define EE_WINDOWCCURSORMANAGERSDL_HPP

#include <eepp/window/ccursormanager.hpp>
#include <eepp/window/backend/SDL/base.hpp>

#ifdef EE_BACKEND_SDL_1_2

using namespace EE::Window;

namespace EE { namespace Window { namespace Backend { namespace SDL {

class CursorManagerSDL : public CursorManager {
	public:
		CursorManagerSDL( EE::Window::Window * window );

		Cursor * Create( cTexture * tex, const Vector2i& hotspot, const std::string& name );

		Cursor * Create( cImage * img, const Vector2i& hotspot, const std::string& name );

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
