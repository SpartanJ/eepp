#include <eepp/window/platform/null/cnullimpl.hpp>

namespace EE { namespace Window { namespace Platform {

cNullImpl::cNullImpl( cWindow * window ) :
	cPlatformImpl( window )
{
}

cNullImpl::~cNullImpl() {
}

void cNullImpl::MinimizeWindow() {
}

void cNullImpl::MaximizeWindow() {
}

void cNullImpl::HideWindow() {
}

void cNullImpl::RaiseWindow() {
}

void cNullImpl::ShowWindow() {
}

void cNullImpl::MoveWindow( int left, int top ) {
}

void cNullImpl::SetContext( eeWindowContex Context ) {
}

eeVector2i cNullImpl::Position() {
	return eeVector2i(0,0);
}

void cNullImpl::ShowMouseCursor() {
}

void cNullImpl::HideMouseCursor() {
}

cCursor * cNullImpl::CreateMouseCursor( cTexture * tex, const eeVector2i& hotspot, const std::string& name ) {
	return NULL;
}

cCursor * cNullImpl::CreateMouseCursor( cImage * img, const eeVector2i& hotspot, const std::string& name ) {
	return NULL;
}

cCursor * cNullImpl::CreateMouseCursor( const std::string& path, const eeVector2i& hotspot, const std::string& name ) {
	return NULL;
}

void cNullImpl::SetMouseCursor( cCursor * cursor ) {
}

void cNullImpl::SetSystemMouseCursor( Cursor::EE_SYSTEM_CURSOR syscursor ) {
}

void cNullImpl::RestoreCursor() {
}

}}}
