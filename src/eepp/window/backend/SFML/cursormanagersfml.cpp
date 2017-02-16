#include <eepp/window/backend/SFML/cursormanagersfml.hpp>
#include <eepp/window/backend/SFML/cursorsfml.hpp>
#include <eepp/window/backend/SFML/windowsfml.hpp>

#ifdef EE_BACKEND_SFML_ACTIVE

#ifdef None
#undef None
#endif
#include <SFML/Window.hpp>
#include <eepp/window/platformimpl.hpp>

namespace EE { namespace Window { namespace Backend { namespace SFML {

CursorManagerSFML::CursorManagerSFML( EE::Window::Window * window ) :
	CursorManager( window )
{
}

Cursor * CursorManagerSFML::create( Texture * tex, const Vector2i& hotspot, const std::string& name ) {
#if defined( EE_X11_PLATFORM ) || EE_PLATFORM == EE_PLATFORM_WIN
	return mWindow->getPlatform()->createMouseCursor( tex, hotspot, name );
#else
	return eeNew( CursorSFML, ( tex, hotspot, name, mWindow ) );
#endif
}

Cursor * CursorManagerSFML::create( Image * img, const Vector2i& hotspot, const std::string& name ) {
#if defined( EE_X11_PLATFORM ) || EE_PLATFORM == EE_PLATFORM_WIN
	return mWindow->getPlatform()->createMouseCursor( img, hotspot, name );
#else
	return eeNew( CursorSFML, ( img, hotspot, name, mWindow ) );
#endif
}

Cursor * CursorManagerSFML::create( const std::string& path, const Vector2i& hotspot, const std::string& name ) {
#if defined( EE_X11_PLATFORM ) || EE_PLATFORM == EE_PLATFORM_WIN
	return mWindow->getPlatform()->createMouseCursor( path, hotspot, name );
#else
	return eeNew( CursorSFML, ( path, hotspot, name, mWindow ) );
#endif
}

void CursorManagerSFML::set( Cursor * cursor ) {
	mWindow->getPlatform()->setMouseCursor( cursor );
}

void CursorManagerSFML::set( EE_SYSTEM_CURSOR syscurid ) {
	mWindow->getPlatform()->setSystemMouseCursor( syscurid );
}

void CursorManagerSFML::show() {
	visible( true );
}

void CursorManagerSFML::hide() {
	visible( false );
}

void CursorManagerSFML::visible( bool visible ) {
	if ( visible ) {
		reinterpret_cast<WindowSFML*>(mWindow)->getSFMLWindow()->setMouseCursorVisible( true );

		mWindow->getPlatform()->showMouseCursor();

		mVisible = true;
	} else {
		reinterpret_cast<WindowSFML*>(mWindow)->getSFMLWindow()->setMouseCursorVisible( false );

		mWindow->getPlatform()->hideMouseCursor();

		mVisible = false;
	}
}

void CursorManagerSFML::remove( Cursor * cursor, bool Delete ) {
	CursorManager::remove( cursor, Delete );
}

void CursorManagerSFML::reload() {
	if ( mVisible ) {
		show();

		mWindow->getPlatform()->restoreCursor();
	} else {
		hide();
	}
}

}}}}

#endif
