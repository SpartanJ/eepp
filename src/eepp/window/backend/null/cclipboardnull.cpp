#include <eepp/window/backend/null/cclipboardnull.hpp>
#include <eepp/window/backend/null/cwindownull.hpp>

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

std::string cClipboardNull::GetText() {
	return std::string();
}

String cClipboardNull::GetWideText() {
	return String();
}

}}}}
