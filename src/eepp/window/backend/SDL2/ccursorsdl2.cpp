#include <eepp/window/backend/SDL2/ccursorsdl2.hpp>

#ifdef EE_BACKEND_SDL2

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

cCursorSDL::cCursorSDL( cTexture * tex, const eeVector2i& hotspot, const std::string& name, Window::cWindow * window ) :
	cCursor( tex, hotspot, name, window ),
	mCursor( NULL )
{
	Create();
}

cCursorSDL::cCursorSDL( cImage * img, const eeVector2i& hotspot, const std::string& name, Window::cWindow * window ) :
	cCursor( img, hotspot, name, window ),
	mCursor( NULL )
{
	Create();
}

cCursorSDL::cCursorSDL( const std::string& path, const eeVector2i& hotspot, const std::string& name, Window::cWindow * window ) :
	cCursor( path, hotspot, name, window ),
	mCursor( NULL )
{
	Create();
}

cCursorSDL::~cCursorSDL() {
	if ( NULL != mCursor )
		SDL_FreeCursor( mCursor );
}

void cCursorSDL::Create() {
	if ( NULL == mImage )
		return;

	Uint32 rmask, gmask, bmask, amask;
	#if SDL_BYTEORDER == SDL_BIG_ENDIAN
		rmask = 0xff000000;
		gmask = 0x00ff0000;
		bmask = 0x0000ff00;
		amask = 0x000000ff;
	#else
		rmask = 0x000000ff;
		gmask = 0x0000ff00;
		bmask = 0x00ff0000;
		amask = 0xff000000;
	#endif
	SDL_Surface * TempGlyphSheet = SDL_CreateRGBSurface( SDL_SWSURFACE, mImage->Width(), mImage->Height(), mImage->Channels() * 8, rmask, gmask, bmask, amask );

	SDL_LockSurface( TempGlyphSheet );

	Uint32 ssize = TempGlyphSheet->w * TempGlyphSheet->h * mImage->Channels();

	Uint8 * Ptr = mImage->GetPixels();

	for ( Uint32 i=0; i < ssize; i++ ) {
		( static_cast<Uint8*>( TempGlyphSheet->pixels ) )[i] = Ptr[i];
	}

	mCursor = SDL_CreateColorCursor( TempGlyphSheet, mHotSpot.x, mHotSpot.y );

	SDL_UnlockSurface( TempGlyphSheet );

	SDL_FreeSurface( TempGlyphSheet );
}

SDL_Cursor * cCursorSDL::GetCursor() const {
	return mCursor;
}

}}}}

#endif
