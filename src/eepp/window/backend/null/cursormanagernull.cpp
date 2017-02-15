#include <eepp/window/backend/null/cursormanagernull.hpp>
#include <eepp/window/backend/null/cursornull.hpp>

namespace EE { namespace Window { namespace Backend { namespace Null {

CursorManagerNull::CursorManagerNull( EE::Window::Window * window ) :
	CursorManager( window )
{
}

Cursor * CursorManagerNull::create( Texture * tex, const Vector2i& hotspot, const std::string& name ) {
	return eeNew( CursorNull, ( tex, hotspot, name, mWindow ) );
}

Cursor * CursorManagerNull::create( Image * img, const Vector2i& hotspot, const std::string& name ) {
	return eeNew( CursorNull, ( img, hotspot, name, mWindow ) );
}

Cursor * CursorManagerNull::create( const std::string& path, const Vector2i& hotspot, const std::string& name ) {
	return eeNew( CursorNull, ( path, hotspot, name, mWindow ) );
}

void CursorManagerNull::set( Cursor * cursor ) {
}

void CursorManagerNull::set( EE_SYSTEM_CURSOR syscurid ) {
}

void CursorManagerNull::show() {
}

void CursorManagerNull::hide() {
}

void CursorManagerNull::visible( bool visible ) {
}

void CursorManagerNull::remove( Cursor * cursor, bool Delete ) {
	CursorManager::remove( cursor, Delete );
}

void CursorManagerNull::reload() {
}

}}}}
