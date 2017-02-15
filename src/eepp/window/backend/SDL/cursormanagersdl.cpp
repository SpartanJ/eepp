#include <eepp/window/backend/SDL/cursormanagersdl.hpp>
#include <eepp/window/backend/SDL/cursorsdl.hpp>

#ifdef EE_BACKEND_SDL_1_2

#include <eepp/window/platformimpl.hpp>

#if !defined( EE_COMPILER_MSVC )
#include <SDL/SDL.h>
#else
#include <SDL.h>
#endif

namespace EE { namespace Window { namespace Backend { namespace SDL {

CursorManagerSDL::CursorManagerSDL( EE::Window::Window * window ) :
	CursorManager( window )
{
}

Cursor * CursorManagerSDL::create( Texture * tex, const Vector2i& hotspot, const std::string& name ) {
	#if defined( EE_X11_PLATFORM ) || EE_PLATFORM == EE_PLATFORM_WIN
	return mWindow->getPlatform()->createMouseCursor( tex, hotspot, name );
	#else
	return eeNew( CursorSDL, ( tex, hotspot, name, mWindow ) );
	#endif
}

Cursor * CursorManagerSDL::create( Image * img, const Vector2i& hotspot, const std::string& name ) {
	#if defined( EE_X11_PLATFORM ) || EE_PLATFORM == EE_PLATFORM_WIN
	return mWindow->getPlatform()->createMouseCursor( img, hotspot, name );
	#else
	return eeNew( CursorSDL, ( img, hotspot, name, mWindow ) );
	#endif
}

Cursor * CursorManagerSDL::create( const std::string& path, const Vector2i& hotspot, const std::string& name ) {
	#if defined( EE_X11_PLATFORM ) || EE_PLATFORM == EE_PLATFORM_WIN
	return mWindow->getPlatform()->createMouseCursor( path, hotspot, name );
	#else
	return eeNew( CursorSDL, ( path, hotspot, name, mWindow ) );
	#endif
}

void CursorManagerSDL::set( Cursor * cursor ) {
	mWindow->getPlatform()->setMouseCursor( cursor );
}

void CursorManagerSDL::set( EE_SYSTEM_CURSOR syscurid ) {
	mWindow->getPlatform()->setSystemMouseCursor( syscurid );
}

void CursorManagerSDL::show() {
	visible( true );
}

void CursorManagerSDL::hide() {
	visible( false );
}

void CursorManagerSDL::visible( bool visible ) {
	if ( visible ) {
		SDL_ShowCursor( SDL_ENABLE );

		mWindow->getPlatform()->showMouseCursor();

		mVisible = true;
	} else {
		SDL_ShowCursor( SDL_DISABLE );

		mWindow->getPlatform()->hideMouseCursor();

		mVisible = false;
	}
}

void CursorManagerSDL::remove( Cursor * cursor, bool Delete ) {
	CursorManager::remove( cursor, Delete );
}

void CursorManagerSDL::reload() {
	if ( mVisible ) {
		show();

		mWindow->getPlatform()->restoreCursor();
	} else {
		hide();
	}
}

}}}}

#endif
