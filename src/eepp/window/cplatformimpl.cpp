#include <eepp/window/cplatformimpl.hpp>
#include <eepp/window/cwindow.hpp>

namespace EE { namespace Window { namespace Platform {

PlatformImpl::PlatformImpl( EE::Window::Window * window ) :
	mWindow( window )
{
}

PlatformImpl::~PlatformImpl() {
}

}}}
