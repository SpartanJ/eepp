#ifndef EE_WINDOWCCURSORMANAGERSDL2_HPP
#define EE_WINDOWCCURSORMANAGERSDL2_HPP

#include <eepp/window/ccursormanager.hpp>
#include <eepp/window/backend/SDL2/base.hpp>

#ifdef EE_BACKEND_SDL2

using namespace EE::Window;

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

class cCursorManagerSDL : public cCursorManager {
	public:
		cCursorManagerSDL( Window::cWindow * window );

		cCursor * Create( cTexture * tex, const eeVector2i& hotspot, const std::string& name );

		cCursor * Create( cImage * img, const eeVector2i& hotspot, const std::string& name );

		cCursor * Create( const std::string& path, const eeVector2i& hotspot, const std::string& name );

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
