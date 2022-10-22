#ifndef EE_WINDOWCCURSORMANAGERSDL2_HPP
#define EE_WINDOWCCURSORMANAGERSDL2_HPP

#include <eepp/window/backend/SDL2/base.hpp>
#include <eepp/window/cursormanager.hpp>

#ifdef EE_BACKEND_SDL2

using namespace EE::Window;

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

class CursorManagerSDL : public CursorManager {
  public:
	CursorManagerSDL( EE::Window::Window* window );

	Cursor* create( Texture* tex, const Vector2i& hotspot, const std::string& name );

	Cursor* create( Image* img, const Vector2i& hotspot, const std::string& name );

	Cursor* create( const std::string& path, const Vector2i& hotspot, const std::string& name );

	void set( Cursor* cursor );

	void set( Cursor::SysType syscurid );

	void show();

	void hide();

	void setVisible( bool visible );

	void remove( Cursor* cursor, bool Delete = false );

	void reload();
};

}}}} // namespace EE::Window::Backend::SDL2

#endif

#endif
