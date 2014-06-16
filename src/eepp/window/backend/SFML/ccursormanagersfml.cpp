#include <eepp/window/backend/SFML/ccursormanagersfml.hpp>
#include <eepp/window/backend/SFML/ccursorsfml.hpp>
#include <eepp/window/backend/SFML/cwindowsfml.hpp>

#ifdef EE_BACKEND_SFML_ACTIVE

#ifdef None
#undef None
#endif
#include <SFML/Window.hpp>
#include <eepp/window/cplatformimpl.hpp>

namespace EE { namespace Window { namespace Backend { namespace SFML {

CursorManagerSFML::CursorManagerSFML( EE::Window::Window * window ) :
	CursorManager( window )
{
}

Cursor * CursorManagerSFML::Create( cTexture * tex, const Vector2i& hotspot, const std::string& name ) {
#if defined( EE_X11_PLATFORM ) || EE_PLATFORM == EE_PLATFORM_WIN
	return mWindow->GetPlatform()->CreateMouseCursor( tex, hotspot, name );
#else
	return eeNew( CursorSFML, ( tex, hotspot, name, mWindow ) );
#endif
}

Cursor * CursorManagerSFML::Create( cImage * img, const Vector2i& hotspot, const std::string& name ) {
#if defined( EE_X11_PLATFORM ) || EE_PLATFORM == EE_PLATFORM_WIN
	return mWindow->GetPlatform()->CreateMouseCursor( img, hotspot, name );
#else
	return eeNew( CursorSFML, ( img, hotspot, name, mWindow ) );
#endif
}

Cursor * CursorManagerSFML::Create( const std::string& path, const Vector2i& hotspot, const std::string& name ) {
#if defined( EE_X11_PLATFORM ) || EE_PLATFORM == EE_PLATFORM_WIN
	return mWindow->GetPlatform()->CreateMouseCursor( path, hotspot, name );
#else
	return eeNew( CursorSFML, ( path, hotspot, name, mWindow ) );
#endif
}

void CursorManagerSFML::Set( Cursor * cursor ) {
	mWindow->GetPlatform()->SetMouseCursor( cursor );
}

void CursorManagerSFML::Set( EE_SYSTEM_CURSOR syscurid ) {
	mWindow->GetPlatform()->SetSystemMouseCursor( syscurid );
}

void CursorManagerSFML::Show() {
	Visible( true );
}

void CursorManagerSFML::Hide() {
	Visible( false );
}

void CursorManagerSFML::Visible( bool visible ) {
	if ( visible ) {
		reinterpret_cast<WindowSFML*>(mWindow)->GetSFMLWindow()->setMouseCursorVisible( true );

		mWindow->GetPlatform()->ShowMouseCursor();

		mVisible = true;
	} else {
		reinterpret_cast<WindowSFML*>(mWindow)->GetSFMLWindow()->setMouseCursorVisible( false );

		mWindow->GetPlatform()->HideMouseCursor();

		mVisible = false;
	}
}

void CursorManagerSFML::Remove( Cursor * cursor, bool Delete ) {
    CursorManager::Remove( cursor, Delete );
}

void CursorManagerSFML::Reload() {
	if ( mVisible ) {
		Show();

		mWindow->GetPlatform()->RestoreCursor();
	} else {
		Hide();
	}
}

}}}}

#endif
