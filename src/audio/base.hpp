#ifndef EE_AUDIO_BASE
#define EE_AUDIO_BASE

#include "../base.hpp"
#include "openal.hpp"

#include "../system/clog.hpp"
#include "../system/cpack.hpp"
#include "../system/cthread.hpp"
using namespace EE::System;

#include "../utils/vector3.hpp"
using namespace EE::Utils;

#if EE_PLATFORM == EE_PLATFORM_HAIKU
#define EE_NO_SNDFILE
#endif

/**
This module is based on the sfml-audio module.

SFML - Copyright (c) 2007-2009 Laurent Gomila - laurent.gom@gmail.com

This software is provided 'as-is', without any express or
implied warranty. In no event will the authors be held
liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute
it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented;
   you must not claim that you wrote the original software.
   If you use this software in a product, an acknowledgment
   in the product documentation would be appreciated but
   is not required.

2. Altered source versions must be plainly marked as such,
   and must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any
   source distribution.
*/
#endif
