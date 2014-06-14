#include <eepp/window/cplatformimpl.hpp>
#include <eepp/window/cwindow.hpp>

namespace EE { namespace Window { namespace Platform {

PlatformImpl::PlatformImpl( cWindow * window ) :
	mWindow( window )
{
}

PlatformImpl::~PlatformImpl() {
}

}}}
