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

Cursor * CursorManagerSDL::Create( cTexture * tex, const Vector2i& hotspot, const std::string& name ) {
	#if defined( EE_X11_PLATFORM ) || EE_PLATFORM == EE_PLATFORM_WIN
	return mWindow->GetPlatform()->CreateMouseCursor( tex, hotspot, name );
	#else
	return eeNew( CursorSDL, ( tex, hotspot, name, mWindow ) );
	#endif
}

Cursor * CursorManagerSDL::Create( cImage * img, const Vector2i& hotspot, const std::string& name ) {
	#if defined( EE_X11_PLATFORM ) || EE_PLATFORM == EE_PLATFORM_WIN
	return mWindow->GetPlatform()->CreateMouseCursor( img, hotspot, name );
	#else
	return eeNew( CursorSDL, ( img, hotspot, name, mWindow ) );
	#endif
}

Cursor * CursorManagerSDL::Create( const std::string& path, const Vector2i& hotspot, const std::string& name ) {
	#if defined( EE_X11_PLATFORM ) || EE_PLATFORM == EE_PLATFORM_WIN
	return mWindow->GetPlatform()->CreateMouseCursor( path, hotspot, name );
	#else
	return eeNew( CursorSDL, ( path, hotspot, name, mWindow ) );
	#endif
}

void CursorManagerSDL::Set( Cursor * cursor ) {
	mWindow->GetPlatform()->SetMouseCursor( cursor );
}

void CursorManagerSDL::Set( EE_SYSTEM_CURSOR syscurid ) {
	mWindow->GetPlatform()->SetSystemMouseCursor( syscurid );
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

        mWindow->GetPlatform()->ShowMouseCursor();

		mVisible = true;
	} else {
		SDL_ShowCursor( SDL_DISABLE );

		mWindow->GetPlatform()->HideMouseCursor();

		mVisible = false;
	}
}

void CursorManagerSDL::Remove( Cursor * cursor, bool Delete ) {
    CursorManager::Remove( cursor, Delete );
}

void CursorManagerSDL::Reload() {
	if ( mVisible ) {
		Show();

		mWindow->GetPlatform()->RestoreCursor();
	} else {
		Hide();
	}
}

}}}}

#endif
