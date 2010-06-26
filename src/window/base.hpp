#ifndef EE_WINDOW_BASE
#define EE_WINDOW_BASE

#include "../base.hpp"

#include <SDL/SDL_syswm.h>

#include "../helper/fastevents/fastevents.h"

#if EE_PLATFORM == EE_PLATFORM_LINUX
#include <X11/Xlib.h>
typedef Window X11Window;
#endif

#include "../utils/colors.hpp"
#include "../utils/rect.hpp"
#include "../utils/vector2.hpp"
#include "../utils/string.hpp"
#include "../utils/utils.hpp"
using namespace EE::Utils;

#include "../system/ctimeelapsed.hpp"
#include "../system/singleton.hpp"
#include "../system/clog.hpp"
using namespace EE::System;

#include "../graphics/renders.hpp"
using namespace EE::Graphics;

#endif
