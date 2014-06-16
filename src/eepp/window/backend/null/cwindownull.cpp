#include <eepp/window/backend/null/cwindownull.hpp>
#include <eepp/window/backend/null/cclipboardnull.hpp>
#include <eepp/window/backend/null/cinputnull.hpp>
#include <eepp/window/backend/null/ccursormanagernull.hpp>

namespace EE { namespace Window { namespace Backend { namespace Null {

WindowNull::WindowNull( WindowSettings Settings, ContextSettings Context ) :
	Window( Settings, Context, eeNew( ClipboardNull, ( this ) ), eeNew( InputNull, ( this ) ), eeNew( CursorManagerNull, ( this ) ) )
{
	Create( Settings, Context );
}

WindowNull::~WindowNull() {
}

bool WindowNull::Create( WindowSettings Settings, ContextSettings Context ) {
	return false;
}

void WindowNull::ToggleFullscreen() {
}

void WindowNull::Caption( const std::string& Caption ) {
}

std::string WindowNull::Caption() {
	return std::string();
}

bool WindowNull::Icon( const std::string& Path ) {
	return false;
}

void WindowNull::Minimize() {
}

void WindowNull::Maximize() {
}

void WindowNull::Hide() {
}

void WindowNull::Raise() {
}

void WindowNull::Show() {
}

void WindowNull::Position( Int16 Left, Int16 Top ) {
}

bool WindowNull::Active() {
	return true;
}

bool WindowNull::Visible() {
	return true;
}

Vector2i WindowNull::Position() {
	return Vector2i();
}

void WindowNull::Size( Uint32 Width, Uint32 Height, bool Windowed ) {
}

void WindowNull::SwapBuffers() {
}

std::vector<DisplayMode> WindowNull::GetDisplayModes() const {
	return std::vector<DisplayMode>();
}

void WindowNull::SetGamma( Float Red, Float Green, Float Blue ) {
}

eeWindowContex WindowNull::GetContext() const {
	return 0;
}

void WindowNull::GetMainContext() {
}

eeWindowHandle	WindowNull::GetWindowHandler() {
	return 0;
}

void WindowNull::SetDefaultContext() {
}

}}}}
