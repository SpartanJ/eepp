#include <eepp/window/backend/null/cclipboardnull.hpp>
#include <eepp/window/backend/null/cwindownull.hpp>

namespace EE { namespace Window { namespace Backend { namespace Null {

ClipboardNull::ClipboardNull( cWindow * window ) :
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
