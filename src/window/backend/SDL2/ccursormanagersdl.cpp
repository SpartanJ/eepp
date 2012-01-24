#include "ccursormanagersdl.hpp"
#include "ccursorsdl.hpp"

#ifdef EE_BACKEND_SDL2

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

cCursorManagerSDL::cCursorManagerSDL( cWindow * window ) :
	cCursorManager( window )
{
}

cCursor * cCursorManagerSDL::Create( cTexture * tex, const eeVector2i& hotspot, const std::string& name ) {
	return eeNew( cCursorSDL, ( tex, hotspot, name, mWindow ) );
}

cCursor * cCursorManagerSDL::Create( cImage * img, const eeVector2i& hotspot, const std::string& name ) {
	return eeNew( cCursorSDL, ( img, hotspot, name, mWindow ) );
}

cCursor * cCursorManagerSDL::Create( const std::string& path, const eeVector2i& hotspot, const std::string& name ) {
	return eeNew( cCursorSDL, ( path, hotspot, name, mWindow ) );
}

void cCursorManagerSDL::Set( cCursor * cursor ) {
	SDL_SetCursor( reinterpret_cast<cCursorSDL*>( cursor )->GetCursor() );

	mCurrent		= cursor;
	mCurSysCursor	= false;
	mSysCursor		= Cursor::SYS_CURSOR_NONE;
}

void cCursorManagerSDL::Set( EE_SYSTEM_CURSOR syscurid ) {
	mWindow->GetPlatform()->SetSystemMouseCursor( syscurid );

	mCurrent		= NULL;
	mCurSysCursor	= true;
	mSysCursor		= syscurid;
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
