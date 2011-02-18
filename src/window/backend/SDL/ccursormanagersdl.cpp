#include "ccursormanagersdl.hpp"
#include "ccursorsdl.hpp"

namespace EE { namespace Window { namespace Backend { namespace SDL {

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
}

void cCursorManagerSDL::Set( EE_SYSTEM_CURSOR syscurid ) {
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

void cCursorManagerSDL::Reload() {
	if ( mVisible ) {
		Show();
	} else {
		Hide();
	}
}

}}}}
