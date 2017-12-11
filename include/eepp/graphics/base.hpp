#ifndef EE_GRAPHICS_BASE
#define EE_GRAPHICS_BASE

#include <eepp/core.hpp>

#include <eepp/math/vector2.hpp>
#include <eepp/math/vector3.hpp>
#include <eepp/math/rect.hpp>
#include <eepp/math/math.hpp>
using namespace EE::Math;

#include <eepp/system/color.hpp>
#include <eepp/system/bitop.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/system/singleton.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/pack.hpp>
#include <eepp/system/resourcemanager.hpp>
#include <eepp/system/container.hpp>
#include <eepp/system/packmanager.hpp>
#include <eepp/system/filesystem.hpp>
using namespace EE::System;

#include <eepp/graphics/rendermode.hpp>
#include <eepp/graphics/blendmode.hpp>
#include <eepp/graphics/pixeldensity.hpp>

#ifndef EE_MAX_TEXTURE_UNITS
#define EE_MAX_TEXTURE_UNITS 4
#endif

#endif
