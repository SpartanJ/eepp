#include <eepp/window/platformimpl.hpp>
#include <eepp/window/window.hpp>

namespace EE { namespace Window { namespace Platform {

PlatformImpl::PlatformImpl( EE::Window::Window * window ) :
	mWindow( window )
{
}

PlatformImpl::~PlatformImpl() {
}

}}}
