#include <eepp/window/backend/allegro5/ccursormanageral.hpp>
#include <eepp/window/backend/allegro5/ccursoral.hpp>
#include <eepp/window/backend/allegro5/cwindowal.hpp>

#ifdef EE_BACKEND_ALLEGRO_ACTIVE

namespace EE { namespace Window { namespace Backend { namespace Al {

cCursorManagerAl::cCursorManagerAl( cWindow * window ) :
	cCursorManager( window )
{
}

cCursor * cCursorManagerAl::Create( cTexture * tex, const eeVector2i& hotspot, const std::string& name ) {
	return eeNew( cCursorAl, ( tex, hotspot, name, mWindow ) );
}

cCursor * cCursorManagerAl::Create( cImage * img, const eeVector2i& hotspot, const std::string& name ) {
	return eeNew( cCursorAl, ( img, hotspot, name, mWindow ) );
}

cCursor * cCursorManagerAl::Create( const std::string& path, const eeVector2i& hotspot, const std::string& name ) {
	return eeNew( cCursorAl, ( path, hotspot, name, mWindow ) );
}

void cCursorManagerAl::Set( cCursor * cursor ) {
	if ( NULL == cursor )
		return;

	al_set_mouse_cursor( reinterpret_cast<cWindowAl*>( mWindow )->GetDisplay(), reinterpret_cast<cCursorAl*>( cursor )->GetCursor() );
	mCurrent		= cursor;
	mCurSysCursor	= false;
	mSysCursor		= Cursor::SYS_CURSOR_NONE;
}

void cCursorManagerAl::Set( EE_SYSTEM_CURSOR syscurid ) {
	al_set_system_mouse_cursor( reinterpret_cast<cWindowAl*>( mWindow )->GetDisplay(), (ALLEGRO_SYSTEM_MOUSE_CURSOR)syscurid );
	mCurrent		= NULL;
	mCurSysCursor	= true;
	mSysCursor		= syscurid;
}

void cCursorManagerAl::Visible( bool visible ) {
	if ( visible ) {
		al_show_mouse_cursor( reinterpret_cast<cWindowAl*>( mWindow )->GetDisplay() );
		mVisible = true;
	} else {
		al_hide_mouse_cursor( reinterpret_cast<cWindowAl*>( mWindow )->GetDisplay() );
		mVisible = false;
	}
}

void cCursorManagerAl::Remove( cCursor * cursor, bool Delete ) {
    cCursorManager::Remove( cursor, Delete );
}

void cCursorManagerAl::Show() {
	Visible( true );
}

void cCursorManagerAl::Hide() {
	Visible( false );
}

void cCursorManagerAl::Reload() {
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
