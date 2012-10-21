#include <eepp/window/backend/SFML/ccursormanagersfml.hpp>
#include <eepp/window/backend/SFML/ccursorsfml.hpp>
#include <eepp/window/backend/SFML/cwindowsfml.hpp>

#ifdef EE_BACKEND_SFML_ACTIVE

#ifdef None
#undef None
#endif
#include <SFML/Window.hpp>


namespace EE { namespace Window { namespace Backend { namespace SFML {

cCursorManagerSFML::cCursorManagerSFML( cWindow * window ) :
	cCursorManager( window )
{
}

cCursor * cCursorManagerSFML::Create( cTexture * tex, const eeVector2i& hotspot, const std::string& name ) {
#if defined( EE_X11_PLATFORM ) || EE_PLATFORM == EE_PLATFORM_WIN
	return mWindow->GetPlatform()->CreateMouseCursor( tex, hotspot, name );
#else
	return eeNew( cCursorSFML, ( tex, hotspot, name, mWindow ) );
#endif
}

cCursor * cCursorManagerSFML::Create( cImage * img, const eeVector2i& hotspot, const std::string& name ) {
#if defined( EE_X11_PLATFORM ) || EE_PLATFORM == EE_PLATFORM_WIN
	return mWindow->GetPlatform()->CreateMouseCursor( img, hotspot, name );
#else
	return eeNew( cCursorSFML, ( img, hotspot, name, mWindow ) );
#endif
}

cCursor * cCursorManagerSFML::Create( const std::string& path, const eeVector2i& hotspot, const std::string& name ) {
#if defined( EE_X11_PLATFORM ) || EE_PLATFORM == EE_PLATFORM_WIN
	return mWindow->GetPlatform()->CreateMouseCursor( path, hotspot, name );
#else
	return eeNew( cCursorSFML, ( path, hotspot, name, mWindow ) );
#endif
}

void cCursorManagerSFML::Set( cCursor * cursor ) {
	mWindow->GetPlatform()->SetMouseCursor( cursor );
}

void cCursorManagerSFML::Set( EE_SYSTEM_CURSOR syscurid ) {
	mWindow->GetPlatform()->SetSystemMouseCursor( syscurid );
}

void cCursorManagerSFML::Show() {
	Visible( true );
}

void cCursorManagerSFML::Hide() {
	Visible( false );
}

void cCursorManagerSFML::Visible( bool visible ) {
	if ( visible ) {
		reinterpret_cast<cWindowSFML*>(mWindow)->GetSFMLWindow()->setMouseCursorVisible( true );

		mWindow->GetPlatform()->ShowMouseCursor();

		mVisible = true;
	} else {
		reinterpret_cast<cWindowSFML*>(mWindow)->GetSFMLWindow()->setMouseCursorVisible( false );

		mWindow->GetPlatform()->HideMouseCursor();

		mVisible = false;
	}
}

void cCursorManagerSFML::Remove( cCursor * cursor, bool Delete ) {
    cCursorManager::Remove( cursor, Delete );
}

void cCursorManagerSFML::Reload() {
	if ( mVisible ) {
		Show();

		mWindow->GetPlatform()->RestoreCursor();
	} else {
		Hide();
	}
}

}}}}

#endif
