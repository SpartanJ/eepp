#include <eepp/window/backend/null/clipboardnull.hpp>
#include <eepp/window/backend/null/windownull.hpp>

namespace EE { namespace Window { namespace Backend { namespace Null {

ClipboardNull::ClipboardNull( EE::Window::Window * window ) :
	Clipboard( window )
{
}

ClipboardNull::~ClipboardNull() {
}

void ClipboardNull::init() {
}

void ClipboardNull::setText( const std::string& Text ) {
}

std::string ClipboardNull::getText() {
	return std::string();
}

String ClipboardNull::getWideText() {
	return String();
}

}}}}
