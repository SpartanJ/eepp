#include <eepp/window/backend/SDL2/cursormanagersdl2.hpp>
#include <eepp/window/backend/SDL2/cursorsdl2.hpp>
#include <eepp/window/platformimpl.hpp>

#ifdef EE_BACKEND_SDL2

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

static SDL_Cursor * SDL_SYS_CURSORS[ SYS_CURSOR_COUNT ] = {0};

static SDL_Cursor * GetLoadCursor( const EE_SYSTEM_CURSOR& cursor ) {
	if ( 0 == SDL_SYS_CURSORS[ cursor ] ) {
		SDL_SYS_CURSORS[ cursor ] = SDL_CreateSystemCursor( (SDL_SystemCursor)cursor );
	}

	return SDL_SYS_CURSORS[ cursor ];
}

CursorManagerSDL::CursorManagerSDL( EE::Window::Window * window ) :
	CursorManager( window )
{
}

Cursor * CursorManagerSDL::Create( Texture * tex, const Vector2i& hotspot, const std::string& name ) {
	return eeNew( CursorSDL, ( tex, hotspot, name, mWindow ) );
}

Cursor * CursorManagerSDL::Create( Image * img, const Vector2i& hotspot, const std::string& name ) {
	return eeNew( CursorSDL, ( img, hotspot, name, mWindow ) );
}

Cursor * CursorManagerSDL::Create( const std::string& path, const Vector2i& hotspot, const std::string& name ) {
	return eeNew( CursorSDL, ( path, hotspot, name, mWindow ) );
}

void CursorManagerSDL::Set( Cursor * cursor ) {
	if ( NULL != cursor && cursor != mCurrent ) {
		SDL_SetCursor( reinterpret_cast<CursorSDL*>( cursor )->GetCursor() );

		mCurrent		= cursor;
		mCurSysCursor	= false;
		mSysCursor		= SYS_CURSOR_NONE;
	}
}

void CursorManagerSDL::Set( EE_SYSTEM_CURSOR syscurid ) {
	if ( syscurid != mSysCursor ) {
		SDL_SetCursor( GetLoadCursor( syscurid ) );
		
		mCurrent		= NULL;
		mCurSysCursor	= true;
		mSysCursor		= syscurid;
	}
}

void CursorManagerSDL::Show() {
	Visible( true );
}

void CursorManagerSDL::Hide() {
	Visible( false );
}

void CursorManagerSDL::Visible( bool visible ) {
	if ( visible ) {
		SDL_ShowCursor( SDL_ENABLE );
		mVisible = true;
	} else {
		SDL_ShowCursor( SDL_DISABLE );
		mVisible = false;
	}
}

void CursorManagerSDL::Remove( Cursor * cursor, bool Delete ) {
	CursorManager::Remove( cursor, Delete );
}

void CursorManagerSDL::Reload() {
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
