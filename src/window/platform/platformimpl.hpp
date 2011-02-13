#ifndef EE_WINDOWPLATFORMIMPL_HPP
#define EE_WINDOWPLATFORMIMPL_HPP

#include "../../base.hpp"

#if EE_PLATFORM == EE_PLATFORM_LINUX
#include "../platform/x11/cx11impl.hpp"
#elif EE_PLATFORM == EE_PLATFORM_WIN
#include "../platform/win/cwinimpl.hpp"
#elif EE_PLATFORM == EE_PLATFORM_MACOSX
#include "../platform/osx/cosximpl.hpp"
#else
#include "../platform/null/cnullimpl.hpp"
#endif

#endif
