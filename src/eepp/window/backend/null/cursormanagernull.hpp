#ifndef EE_WINDOWCCURSORMANAGERNULL_HPP
#define EE_WINDOWCCURSORMANAGERNULL_HPP

#include <eepp/window/cursormanager.hpp>

using namespace EE::Window;

namespace EE { namespace Window { namespace Backend { namespace Null {

class CursorManagerNull : public CursorManager {
	public:
		CursorManagerNull( EE::Window::Window * window );

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
