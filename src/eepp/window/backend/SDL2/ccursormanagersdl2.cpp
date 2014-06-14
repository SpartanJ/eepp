#include <eepp/window/backend/SDL2/ccursormanagersdl2.hpp>
#include <eepp/window/backend/SDL2/ccursorsdl2.hpp>
#include <eepp/window/cplatformimpl.hpp>

#ifdef EE_BACKEND_SDL2

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

static SDL_Cursor * SDL_SYS_CURSORS[ SYS_CURSOR_COUNT ] = {0};

static SDL_Cursor * GetLoadCursor( const EE_SYSTEM_CURSOR& cursor ) {
	if ( 0 == SDL_SYS_CURSORS[ cursor ] ) {
		SDL_SYS_CURSORS[ cursor ] = SDL_CreateSystemCursor( (SDL_SystemCursor)cursor );
	}

	return SDL_SYS_CURSORS[ cursor ];
}

cCursorManagerSDL::cCursorManagerSDL( cWindow * window ) :
	cCursorManager( window )
{
}

cCursor * cCursorManagerSDL::Create( cTexture * tex, const Vector2i& hotspot, const std::string& name ) {
	return eeNew( cCursorSDL, ( tex, hotspot, name, mWindow ) );
}

cCursor * cCursorManagerSDL::Create( cImage * img, const Vector2i& hotspot, const std::string& name ) {
	return eeNew( cCursorSDL, ( img, hotspot, name, mWindow ) );
}

cCursor * cCursorManagerSDL::Create( const std::string& path, const Vector2i& hotspot, const std::string& name ) {
	return eeNew( cCursorSDL, ( path, hotspot, name, mWindow ) );
}

void cCursorManagerSDL::Set( cCursor * cursor ) {
	if ( NULL != cursor && cursor != mCurrent ) {
		SDL_SetCursor( reinterpret_cast<cCursorSDL*>( cursor )->GetCursor() );

		mCurrent		= cursor;
		mCurSysCursor	= false;
		mSysCursor		= Cursor::SYS_CURSOR_NONE;
	}
}

void cCursorManagerSDL::Set( EE_SYSTEM_CURSOR syscurid ) {
	if ( syscurid != mSysCursor ) {
		SDL_SetCursor( GetLoadCursor( syscurid ) );
		
		mCurrent		= NULL;
		mCurSysCursor	= true;
		mSysCursor		= syscurid;
	}
}

void cCursorManagerSDL::Show() {
	Visible( true );
}

void cCursorManagerSDL::Hide() {
	Visible( false );
}

void cCursorManagerSDL::Visible( bool visible ) {
	if ( visible ) {
		SDL_ShowCursor( SDL_ENABLE );
		mVisible = true;
	} else {
		SDL_ShowCursor( SDL_DISABLE );
		mVisible = false;
	}
}

void cCursorManagerSDL::Remove( cCursor * cursor, bool Delete ) {
	cCursorManager::Remove( cursor, Delete );
}

void cCursorManagerSDL::Reload() {
	if ( mVisible ) {
		Show();

		if ( mCurSysCursor ) {
			Set( mSysCursor );
		} else {
			Set( mCurrent );
		}
	} else {
		Hide();
	}
}

}}}}

#endif
