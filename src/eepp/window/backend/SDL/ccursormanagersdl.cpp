#include <eepp/window/backend/SDL/ccursormanagersdl.hpp>
#include <eepp/window/backend/SDL/ccursorsdl.hpp>

#ifdef EE_BACKEND_SDL_1_2

namespace EE { namespace Window { namespace Backend { namespace SDL {

cCursorManagerSDL::cCursorManagerSDL( cWindow * window ) :
	cCursorManager( window )
{
}

cCursor * cCursorManagerSDL::Create( cTexture * tex, const eeVector2i& hotspot, const std::string& name ) {
	#if defined( EE_X11_PLATFORM ) || EE_PLATFORM == EE_PLATFORM_WIN
	return mWindow->GetPlatform()->CreateMouseCursor( tex, hotspot, name );
	#else
	return eeNew( cCursorSDL, ( tex, hotspot, name, mWindow ) );
	#endif
}

cCursor * cCursorManagerSDL::Create( cImage * img, const eeVector2i& hotspot, const std::string& name ) {
	#if defined( EE_X11_PLATFORM ) || EE_PLATFORM == EE_PLATFORM_WIN
	return mWindow->GetPlatform()->CreateMouseCursor( img, hotspot, name );
	#else
	return eeNew( cCursorSDL, ( img, hotspot, name, mWindow ) );
	#endif
}

cCursor * cCursorManagerSDL::Create( const std::string& path, const eeVector2i& hotspot, const std::string& name ) {
	#if defined( EE_X11_PLATFORM ) || EE_PLATFORM == EE_PLATFORM_WIN
	return mWindow->GetPlatform()->CreateMouseCursor( path, hotspot, name );
	#else
	return eeNew( cCursorSDL, ( path, hotspot, name, mWindow ) );
	#endif
}

void cCursorManagerSDL::Set( cCursor * cursor ) {
	mWindow->GetPlatform()->SetMouseCursor( cursor );
}

void cCursorManagerSDL::Set( EE_SYSTEM_CURSOR syscurid ) {
	mWindow->GetPlatform()->SetSystemMouseCursor( syscurid );
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

        mWindow->GetPlatform()->ShowMouseCursor();

		mVisible = true;
	} else {
		SDL_ShowCursor( SDL_DISABLE );

		mWindow->GetPlatform()->HideMouseCursor();

		mVisible = false;
	}
}

void cCursorManagerSDL::Remove( cCursor * cursor, bool Delete ) {
    cCursorManager::Remove( cursor, Delete );
}

void cCursorManagerSDL::Reload() {
	if ( mVisible ) {
		Show();

		mWindow->GetPlatform()->RestoreCursor();
	} else {
		Hide();
	}
}

}}}}

#endif
