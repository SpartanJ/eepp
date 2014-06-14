#include <eepp/window/cclipboard.hpp>

namespace EE { namespace Window {

Clipboard::Clipboard( cWindow * window ) :
	mWindow( window )
{}

Clipboard::~Clipboard() {}

cWindow * Clipboard::GetWindow() const {
	return mWindow;
}

}}
