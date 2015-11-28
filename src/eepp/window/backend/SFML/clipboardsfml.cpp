#include <eepp/window/backend/SFML/clipboardsfml.hpp>
#include <eepp/window/backend/SFML/windowsfml.hpp>

#ifdef EE_BACKEND_SFML_ACTIVE

namespace EE { namespace Window { namespace Backend { namespace SFML {

ClipboardSFML::ClipboardSFML( EE::Window::Window * window ) :
	Clipboard( window )
{
}

ClipboardSFML::~ClipboardSFML() {
}

void ClipboardSFML::Init() {
}

void ClipboardSFML::SetText( const std::string& Text ) {
}

std::string ClipboardSFML::GetText() {
	return std::string();
}

String ClipboardSFML::GetWideText() {
	return String();
}

}}}}

#endif
