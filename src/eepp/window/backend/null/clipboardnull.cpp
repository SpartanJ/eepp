#include <eepp/window/backend/null/clipboardnull.hpp>
#include <eepp/window/backend/null/windownull.hpp>

namespace EE { namespace Window { namespace Backend { namespace Null {

ClipboardNull::ClipboardNull( EE::Window::Window * window ) :
	Clipboard( window )
{
}

ClipboardNull::~ClipboardNull() {
}

void ClipboardNull::Init() {
}

void ClipboardNull::SetText( const std::string& Text ) {
}

std::string ClipboardNull::GetText() {
	return std::string();
}

String ClipboardNull::GetWideText() {
	return String();
}

}}}}
