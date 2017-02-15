#include <eepp/window/backend/null/windownull.hpp>
#include <eepp/window/backend/null/clipboardnull.hpp>
#include <eepp/window/backend/null/inputnull.hpp>
#include <eepp/window/backend/null/cursormanagernull.hpp>

namespace EE { namespace Window { namespace Backend { namespace Null {

WindowNull::WindowNull( WindowSettings Settings, ContextSettings Context ) :
	Window( Settings, Context, eeNew( ClipboardNull, ( this ) ), eeNew( InputNull, ( this ) ), eeNew( CursorManagerNull, ( this ) ) )
{
	create( Settings, Context );
}

WindowNull::~WindowNull() {
}

bool WindowNull::create( WindowSettings Settings, ContextSettings Context ) {
	return false;
}

void WindowNull::toggleFullscreen() {
}

void WindowNull::caption( const std::string& Caption ) {
}

std::string WindowNull::caption() {
	return std::string();
}

bool WindowNull::icon( const std::string& Path ) {
	return false;
}

void WindowNull::minimize() {
}

void WindowNull::maximize() {
}

void WindowNull::hide() {
}

void WindowNull::raise() {
}

void WindowNull::show() {
}

void WindowNull::position( Int16 Left, Int16 Top ) {
}

bool WindowNull::active() {
	return true;
}

bool WindowNull::visible() {
	return true;
}

Vector2i WindowNull::position() {
	return Vector2i();
}

void WindowNull::size( Uint32 Width, Uint32 Height, bool Windowed ) {
}

void WindowNull::swapBuffers() {
}

std::vector<DisplayMode> WindowNull::getDisplayModes() const {
	return std::vector<DisplayMode>();
}

void WindowNull::setGamma( Float Red, Float Green, Float Blue ) {
}

eeWindowContex WindowNull::getContext() const {
	return 0;
}

void WindowNull::getMainContext() {
}

eeWindowHandle	WindowNull::getWindowHandler() {
	return 0;
}

void WindowNull::setDefaultContext() {
}

}}}}
