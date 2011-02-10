#include "cclipboardal.hpp"
#include "cwindowal.hpp"

#ifdef EE_BACKEND_ALLEGRO_ACTIVE

namespace EE { namespace Window { namespace Backend { namespace Al {

cClipboardAl::cClipboardAl( cWindow * window ) :
	cClipboard( window )
{
}

cClipboardAl::~cClipboardAl() {
}

void cClipboardAl::Init() {
}

void cClipboardAl::SetText( const std::string& Text ) {
}

void cClipboardAl::SetText( const std::wstring& Text ) {
}

std::string cClipboardAl::GetText() {
	return std::string();
}

std::wstring cClipboardAl::GetTextWStr() {
	return std::wstring();
}

}}}}

#endif
