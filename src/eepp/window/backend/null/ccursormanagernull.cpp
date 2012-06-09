#include <eepp/window/backend/null/ccursormanagernull.hpp>
#include <eepp/window/backend/null/ccursornull.hpp>

namespace EE { namespace Window { namespace Backend { namespace Null {

cCursorManagerNull::cCursorManagerNull( cWindow * window ) :
	cCursorManager( window )
{
}

cCursor * cCursorManagerNull::Create( cTexture * tex, const eeVector2i& hotspot, const std::string& name ) {
	return eeNew( cCursorNull, ( tex, hotspot, name, mWindow ) );
}

cCursor * cCursorManagerNull::Create( cImage * img, const eeVector2i& hotspot, const std::string& name ) {
	return eeNew( cCursorNull, ( img, hotspot, name, mWindow ) );
}

cCursor * cCursorManagerNull::Create( const std::string& path, const eeVector2i& hotspot, const std::string& name ) {
	return eeNew( cCursorNull, ( path, hotspot, name, mWindow ) );
}

void cCursorManagerNull::Set( cCursor * cursor ) {
}

void cCursorManagerNull::Set( EE_SYSTEM_CURSOR syscurid ) {
}

void cCursorManagerNull::Show() {
}

void cCursorManagerNull::Hide() {
}

void cCursorManagerNull::Visible( bool visible ) {
}

void cCursorManagerNull::Remove( cCursor * cursor, bool Delete ) {
    cCursorManager::Remove( cursor, Delete );
}

void cCursorManagerNull::Reload() {
}

}}}}
