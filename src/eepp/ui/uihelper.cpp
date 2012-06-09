#include <eepp/ui/uihelper.hpp>

namespace EE { namespace UI {

Uint32 HAlignGet( Uint32 Flags ) {
	return Flags & UI_HALIGN_MASK;
}

Uint32 VAlignGet( Uint32 Flags ) {
	return Flags & UI_VALIGN_MASK;
}

}} 
