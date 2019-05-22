#include <eepp/window/backend/SDL2/cursormanagersdl2.hpp>
#include <eepp/window/backend/SDL2/cursorsdl2.hpp>
#include <eepp/window/platformimpl.hpp>

#ifdef EE_BACKEND_SDL2

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

static SDL_Cursor * SDL_SYS_CURSORS[ Cursor::SysCursorCount ] = {0};

static SDL_Cursor * getLoadCursor( const Cursor::SysType& cursor ) {
	if ( 0 == SDL_SYS_CURSORS[ cursor ] ) {
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
		SDL_SYS_CURSORS[ cursor ] = SDL_CreateSystemCursor( (SDL_SystemCursor)cursor );
#endif
	}

	return SDL_SYS_CURSORS[ cursor ];
}

CursorManagerSDL::CursorManagerSDL( EE::Window::Window * window ) :
	CursorManager( window )
{
}

Cursor * CursorManagerSDL::create( Texture * tex, const Vector2i& hotspot, const std::string& name ) {
	return eeNew( CursorSDL, ( tex, hotspot, name, mWindow ) );
}

Cursor * CursorManagerSDL::create( Image * img, const Vector2i& hotspot, const std::string& name ) {
	return eeNew( CursorSDL, ( img, hotspot, name, mWindow ) );
}

Cursor * CursorManagerSDL::create( const std::string& path, const Vector2i& hotspot, const std::string& name ) {
	return eeNew( CursorSDL, ( path, hotspot, name, mWindow ) );
}

void CursorManagerSDL::set( Cursor * cursor ) {
	if ( NULL != cursor && cursor != mCurrent ) {
		SDL_SetCursor( reinterpret_cast<CursorSDL*>( cursor )->GetCursor() );

		mCurrent		= cursor;
		mCurSysCursor	= false;
		mSysCursor		= Cursor::SysCursorNone;
	}
}

void CursorManagerSDL::set( Cursor::SysType syscurid ) {
	if ( syscurid != mSysCursor ) {
		SDL_SetCursor( getLoadCursor( syscurid ) );
		
		mCurrent		= NULL;
		mCurSysCursor	= true;
		mSysCursor		= syscurid;
	}
}

void CursorManagerSDL::show() {
	setVisible( true );
}

void CursorManagerSDL::hide() {
	setVisible( false );
}

void CursorManagerSDL::setVisible( bool visible ) {
	if ( visible ) {
		SDL_ShowCursor( SDL_ENABLE );
		mVisible = true;
	} else {
		SDL_ShowCursor( SDL_DISABLE );
		mVisible = false;
	}
}

void CursorManagerSDL::remove( Cursor * cursor, bool Delete ) {
	CursorManager::remove( cursor, Delete );
}

void CursorManagerSDL::reload() {
	if ( mVisible ) {
		show();

		if ( mCurSysCursor ) {
			set( mSysCursor );
		} else {
			set( mCurrent );
		}
	} else {
		hide();
	}
}

}}}}

#endif
