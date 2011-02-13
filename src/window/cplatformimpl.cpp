#include "cplatformimpl.hpp"
#include "cwindow.hpp"

namespace EE { namespace Window { namespace Platform {

cPlatformImpl::cPlatformImpl( cWindow * window ) :
	mWindow( window )
{
}

cPlatformImpl::~cPlatformImpl() {
}

}}}
