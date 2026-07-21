#include <eepp/window/backend/SDL3/cursorsdl3.hpp>

#ifdef EE_BACKEND_SDL3

#include <cstring>
#include <eepp/graphics/image.hpp>

namespace EE { namespace Window { namespace Backend { namespace SDL3 {

CursorSDL::CursorSDL( Texture* tex, const Vector2i& hotspot, const std::string& name,
					  EE::Window::Window* window ) :
	Cursor( tex, hotspot, name, window ), mCursor( nullptr ) {
	create();
}

CursorSDL::CursorSDL( Graphics::Image* img, const Vector2i& hotspot, const std::string& name,
					  EE::Window::Window* window ) :
	Cursor( img, hotspot, name, window ), mCursor( nullptr ) {
	create();
}

CursorSDL::CursorSDL( const std::string& path, const Vector2i& hotspot, const std::string& name,
					  EE::Window::Window* window ) :
	Cursor( path, hotspot, name, window ), mCursor( nullptr ) {
	create();
}

CursorSDL::~CursorSDL() {
	if ( nullptr != mCursor )
		SDL_DestroyCursor( mCursor );
}

void CursorSDL::create() {
	if ( nullptr == mImage )
		return;

	int x = mImage->getWidth();
	int y = mImage->getHeight();
	int c = mImage->getChannels();

	SDL_Surface* surface = SDL_CreateSurface( x, y, SDL_PIXELFORMAT_RGBA32 );
	if ( !surface )
		return;

	Uint8* src = (Uint8*)mImage->getPixels();
	Uint8* dst = (Uint8*)surface->pixels;
	int srcPitch = x * c;
	int dstPitch = surface->pitch;

	for ( int row = 0; row < y; ++row ) {
		memcpy( dst + row * dstPitch, src + row * srcPitch, srcPitch );
	}

	mCursor = SDL_CreateColorCursor( surface, mHotSpot.x, mHotSpot.y );

	SDL_DestroySurface( surface );
}

SDL_Cursor* CursorSDL::GetCursor() const {
	return mCursor;
}

}}}} // namespace EE::Window::Backend::SDL3

#endif
