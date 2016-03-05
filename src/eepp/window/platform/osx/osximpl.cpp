#include <eepp/config.hpp>

#if EE_PLATFORM == EE_PLATFORM_MACOSX

#include <AGL/agl.h>

#include <eepp/window/platform/osx/osximpl.hpp>

namespace EE { namespace Window { namespace Platform {

OSXImpl::OSXImpl( EE::Window::Window * window ) :
	PlatformImpl( window )
{
}

OSXImpl::~OSXImpl() {
}

void OSXImpl::MinimizeWindow() {
}

void OSXImpl::MaximizeWindow() {
}

bool OSXImpl::IsWindowMaximized() {
	return false;
}

void OSXImpl::HideWindow() {
}

void OSXImpl::RaiseWindow() {
}

void OSXImpl::ShowWindow() {
}

void OSXImpl::MoveWindow( int left, int top ) {
}

void OSXImpl::SetContext( eeWindowContex Context ) {
	aglSetCurrentContext( Context );
}

Vector2i OSXImpl::Position() {
	return Vector2i(0,0);
}

void OSXImpl::ShowMouseCursor() {
}

void OSXImpl::HideMouseCursor() {
}

Cursor * OSXImpl::CreateMouseCursor( Texture * tex, const Vector2i& hotspot, const std::string& name ) {
	return NULL;
}

Cursor * OSXImpl::CreateMouseCursor( Image * img, const Vector2i& hotspot, const std::string& name ) {
	return NULL;
}

Cursor * OSXImpl::CreateMouseCursor( const std::string& path, const Vector2i& hotspot, const std::string& name ) {
	return NULL;
}

void OSXImpl::SetMouseCursor( Cursor * cursor ) {
}

void OSXImpl::SetSystemMouseCursor( EE_SYSTEM_CURSOR syscursor ) {
}

void OSXImpl::RestoreCursor() {
}

eeWindowContex OSXImpl::GetWindowContext() {
	return aglGetCurrentContext();
}

}}}

#endif
