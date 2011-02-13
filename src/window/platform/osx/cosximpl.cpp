#include "cosximpl.hpp"

#if EE_PLATFORM == EE_PLATFORM_MACOSX

namespace EE { namespace Window { namespace Platform {

cOSXImpl::cOSXImpl( cWindow * window ) :
	cPlatformImpl( window )
{
}

cOSXImpl::~cOSXImpl() {
}

void cOSXImpl::MinimizeWindow() {
}

void cOSXImpl::MaximizeWindow() {
}

void cOSXImpl::HideWindow() {
}

void cOSXImpl::RaiseWindow() {
}

void cOSXImpl::ShowWindow() {
}

void cOSXImpl::MoveWindow( int left, int top ) {
}

void cOSXImpl::SetContext( eeWindowContex Context ) {
	aglSetCurrentContext( Context );
}

eeVector2i cOSXImpl::Position() {
	return eeVector2i(0,0);
}

}}}

#endif
