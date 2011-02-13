#include "cnullimpl.hpp"

namespace EE { namespace Window { namespace Platform {

cNullImpl::cNullImpl( cWindow * window ) :
	cPlatformImpl( window )
{
}

cNullImpl::~cNullImpl() {
}

void cNullImpl::MinimizeWindow() {
}

void cNullImpl::MaximizeWindow() {
}

void cNullImpl::HideWindow() {
}

void cNullImpl::RaiseWindow() {
}

void cNullImpl::ShowWindow() {
}

void cNullImpl::MoveWindow( int left, int top ) {
}

void cNullImpl::SetContext( eeWindowContex Context ) {
}

eeVector2i cNullImpl::Position() {
	return eeVector2i(0,0);
}

}}}
