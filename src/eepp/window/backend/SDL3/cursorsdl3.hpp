#ifndef EE_WINDOWCCURSORSDL3_HPP
#define EE_WINDOWCCURSORSDL3_HPP

#include <eepp/window/backend/SDL3/base.hpp>
#include <eepp/window/cursor.hpp>

#ifdef EE_BACKEND_SDL3

namespace EE { namespace Window { namespace Backend { namespace SDL3 {

class EE_API CursorSDL : public Cursor {
  public:
	SDL_Cursor* GetCursor() const;

  protected:
	friend class CursorManagerSDL;

	SDL_Cursor* mCursor;

	CursorSDL( Texture* tex, const Vector2i& hotspot, const std::string& name,
			   EE::Window::Window* window );

	CursorSDL( Graphics::Image* img, const Vector2i& hotspot, const std::string& name,
			   EE::Window::Window* window );

	CursorSDL( const std::string& path, const Vector2i& hotspot, const std::string& name,
			   EE::Window::Window* window );

	virtual ~CursorSDL();

	void create();
};

}}}} // namespace EE::Window::Backend::SDL3

#endif
#endif
