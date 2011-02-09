#include "cclipboardnull.hpp"
#include "cwindownull.hpp"

namespace EE { namespace Window { namespace Backend { namespace Null {

cClipboardNull::cClipboardNull( cWindow * window ) :
	cClipboard( window )
{
}

cClipboardNull::~cClipboardNull() {
}

void cClipboardNull::Init() {
}

void cClipboardNull::SetText( const std::string& Text ) {
}

void cClipboardNull::SetText( const std::wstring& Text ) {
}

std::string cClipboardNull::GetText() {
	return std::string();
}

std::wstring cClipboardNull::GetTextWStr() {
	return std::wstring();
}

}}}}
