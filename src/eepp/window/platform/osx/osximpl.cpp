#include <eepp/config.hpp>

#if EE_PLATFORM == EE_PLATFORM_MACOSX

#include <eepp/window/platform/osx/osximpl.hpp>

namespace EE { namespace Window { namespace Platform {

OSXImpl::OSXImpl( EE::Window::Window * window ) :
	PlatformImpl( window )
{
}

OSXImpl::~OSXImpl() {
}

void OSXImpl::minimizeWindow() {
}

void OSXImpl::maximizeWindow() {
}

bool OSXImpl::isWindowMaximized() {
	return false;
}

void OSXImpl::hideWindow() {
}

void OSXImpl::raiseWindow() {
}

void OSXImpl::showWindow() {
}

void OSXImpl::moveWindow( int left, int top ) {
}

void OSXImpl::setContext( eeWindowContex Context ) {
}

Vector2i OSXImpl::getPosition() {
	return Vector2i(0,0);
}

void OSXImpl::showMouseCursor() {
}

void OSXImpl::hideMouseCursor() {
}

Cursor * OSXImpl::createMouseCursor( Texture * tex, const Vector2i& hotspot, const std::string& name ) {
	return NULL;
}

Cursor * OSXImpl::createMouseCursor( Image * img, const Vector2i& hotspot, const std::string& name ) {
	return NULL;
}

Cursor * OSXImpl::createMouseCursor( const std::string& path, const Vector2i& hotspot, const std::string& name ) {
	return NULL;
}

void OSXImpl::setMouseCursor( Cursor * cursor ) {
}

void OSXImpl::setSystemMouseCursor( Cursor::SysType syscursor ) {
}

void OSXImpl::restoreCursor() {
}

eeWindowContex OSXImpl::getWindowContext() {
	return NULL;
}

}}}

#endif
