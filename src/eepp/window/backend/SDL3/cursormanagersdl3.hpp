#ifndef EE_WINDOWCCURSORMANAGERSDL3_HPP
#define EE_WINDOWCCURSORMANAGERSDL3_HPP

#include <eepp/window/backend/SDL3/base.hpp>
#include <eepp/window/cursormanager.hpp>

#ifdef EE_BACKEND_SDL3

using namespace EE::Window;

namespace EE { namespace Window { namespace Backend { namespace SDL3 {

class EE_API CursorManagerSDL : public CursorManager {
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

}}}} // namespace EE::Window::Backend::SDL3

#endif
#endif
