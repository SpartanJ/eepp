#include <eepp/window/platform/null/nullimpl.hpp>

namespace EE { namespace Window { namespace Platform {

NullImpl::NullImpl( EE::Window::Window * window ) :
	PlatformImpl( window )
{
}

NullImpl::~NullImpl() {
}

void NullImpl::minimizeWindow() {
}

void NullImpl::maximizeWindow() {
}

bool NullImpl::isWindowMaximized() {
	return false;
}

void NullImpl::hideWindow() {
}

void NullImpl::raiseWindow() {
}

void NullImpl::showWindow() {
}

void NullImpl::moveWindow( int left, int top ) {
}

void NullImpl::setContext( eeWindowContex Context ) {
}

Vector2i NullImpl::getPosition() {
	return Vector2i(0,0);
}

void NullImpl::showMouseCursor() {
}

void NullImpl::hideMouseCursor() {
}

Cursor * NullImpl::createMouseCursor( Texture * tex, const Vector2i& hotspot, const std::string& name ) {
	return NULL;
}

Cursor * NullImpl::createMouseCursor( Image * img, const Vector2i& hotspot, const std::string& name ) {
	return NULL;
}

Cursor * NullImpl::createMouseCursor( const std::string& path, const Vector2i& hotspot, const std::string& name ) {
	return NULL;
}

void NullImpl::setMouseCursor( Cursor * cursor ) {
}

void NullImpl::setSystemMouseCursor( EE_SYSTEM_CURSOR syscursor ) {
}

void NullImpl::restoreCursor() {
}

eeWindowContex NullImpl::getWindowContext() {
	return 0;
}

}}}
