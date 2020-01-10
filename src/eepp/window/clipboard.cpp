#include <eepp/window/clipboard.hpp>

namespace EE { namespace Window {

Clipboard::Clipboard( EE::Window::Window* window ) : mWindow( window ) {}

Clipboard::~Clipboard() {}

EE::Window::Window* Clipboard::getWindow() const {
	return mWindow;
}

}} // namespace EE::Window
