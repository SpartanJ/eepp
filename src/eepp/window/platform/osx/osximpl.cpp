#include <eepp/config.hpp>

#if EE_PLATFORM == EE_PLATFORM_MACOSX

#include <AGL/agl.h>

#include <eepp/window/platform/osx/osximpl.hpp>

namespace EE { namespace Window { namespace Platform {

cOSXImpl::cOSXImpl( EE::Window::Window * window ) :
	PlatformImpl( window )
{
}

cOSXImpl::~cOSXImpl() {
}

void cOSXImpl::MinimizeWindow() {
}

void cOSXImpl::MaximizeWindow() {
}

bool cOSXImpl::IsWindowMaximized() {
	return false;
}

void cOSXImpl::HideWindow() {
}

void cOSXImpl::RaiseWindow() {
}

void cOSXImpl::ShowWindow() {
}

void cOSXImpl::MoveWindow( int left, int top ) {
}

void cOSXImpl::SetContext( eeWindowContex Context ) {
	aglSetCurrentContext( Context );
}

Vector2i cOSXImpl::Position() {
	return Vector2i(0,0);
}

void cOSXImpl::ShowMouseCursor() {
}

void cOSXImpl::HideMouseCursor() {
}

Cursor * cOSXImpl::CreateMouseCursor( Texture * tex, const Vector2i& hotspot, const std::string& name ) {
	return NULL;
}

Cursor * cOSXImpl::CreateMouseCursor( Image * img, const Vector2i& hotspot, const std::string& name ) {
	return NULL;
}

Cursor * cOSXImpl::CreateMouseCursor( const std::string& path, const Vector2i& hotspot, const std::string& name ) {
	return NULL;
}

void cOSXImpl::SetMouseCursor( Cursor * cursor ) {
}

void cOSXImpl::SetSystemMouseCursor( EE_SYSTEM_CURSOR syscursor ) {
}

void cOSXImpl::RestoreCursor() {
}

eeWindowContex cOSXImpl::GetWindowContext() {
	return aglGetCurrentContext();
}

}}}

#endif
