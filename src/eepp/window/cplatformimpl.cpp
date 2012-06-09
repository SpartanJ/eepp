#include <eepp/window/cplatformimpl.hpp>
#include <eepp/window/cwindow.hpp>

namespace EE { namespace Window { namespace Platform {

cPlatformImpl::cPlatformImpl( cWindow * window ) :
	mWindow( window )
{
}

cPlatformImpl::~cPlatformImpl() {
}

}}}
