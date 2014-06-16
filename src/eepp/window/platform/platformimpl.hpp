#ifndef EE_WINDOWPLATFORMIMPL_HPP
#define EE_WINDOWPLATFORMIMPL_HPP

#include <eepp/core.hpp>

#if defined( EE_X11_PLATFORM )
#include <eepp/window/platform/x11/x11impl.hpp>
#elif EE_PLATFORM == EE_PLATFORM_WIN
#include <eepp/window/platform/win/winimpl.hpp>
#elif EE_PLATFORM == EE_PLATFORM_MACOSX
#include <eepp/window/platform/osx/osximpl.hpp>
#else
#include <eepp/window/platform/null/nullimpl.hpp>
#endif

#endif
