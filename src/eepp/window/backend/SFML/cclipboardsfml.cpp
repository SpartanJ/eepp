#include <eepp/window/backend/SFML/cclipboardsfml.hpp>
#include <eepp/window/backend/SFML/cwindowsfml.hpp>

#ifdef EE_BACKEND_SFML_ACTIVE

namespace EE { namespace Window { namespace Backend { namespace SFML {

cClipboardSFML::cClipboardSFML( cWindow * window ) :
	cClipboard( window )
{
}

cClipboardSFML::~cClipboardSFML() {
}

void cClipboardSFML::Init() {
}

void cClipboardSFML::SetText( const std::string& Text ) {
}

std::string cClipboardSFML::GetText() {
	return std::string();
}

String cClipboardSFML::GetWideText() {
	return String();
}

}}}}

#endif
